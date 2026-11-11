// SPDX-License-Identifier: MIT
#include "ocf/scene/Node.h"
#include "ocf/scene/animation/Easing.h"
#include "ocf/scene/animation/Tween.h"
#include "ocf/scene/animation/Tweener.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace ocf;

namespace {

void expectVec3(const math::vec3& actual, const math::vec3& expected)
{
    EXPECT_NEAR(actual.x, expected.x, 1e-4f);
    EXPECT_NEAR(actual.y, expected.y, 1e-4f);
    EXPECT_NEAR(actual.z, expected.z, 1e-4f);
}

} // namespace

// ---- Easing ------------------------------------------------------------------------------------

TEST(EasingTest, EveryCurveStartsAtZeroAndEndsAtOne)
{
    for (int i = 0; i <= static_cast<int>(Ease::InOutBounce); ++i) {
        const Ease curve = static_cast<Ease>(i);
        EXPECT_EQ(ease(curve, 0.0f), 0.0f) << i;
        EXPECT_EQ(ease(curve, 1.0f), 1.0f) << i;
    }
}

TEST(EasingTest, ClampsTheProgress)
{
    EXPECT_EQ(ease(Ease::OutQuad, -0.5f), 0.0f);
    EXPECT_EQ(ease(Ease::OutQuad, 2.0f), 1.0f);
}

TEST(EasingTest, MatchesKnownValues)
{
    EXPECT_FLOAT_EQ(ease(Ease::Linear, 0.25f), 0.25f);
    EXPECT_FLOAT_EQ(ease(Ease::InQuad, 0.5f), 0.25f);
    EXPECT_FLOAT_EQ(ease(Ease::OutQuad, 0.5f), 0.75f);
    EXPECT_FLOAT_EQ(ease(Ease::InOutCubic, 0.5f), 0.5f);
    EXPECT_NEAR(ease(Ease::InOutSine, 0.5f), 0.5f, 1e-6f);
    EXPECT_LT(ease(Ease::InBack, 0.2f), 0.0f);
    EXPECT_GT(ease(Ease::OutBack, 0.8f), 1.0f);
}

// ---- Tween ---------------------------------------------------------------------------------------

TEST(TweenTest, MoveToInterpolatesFromThePositionAtStart)
{
    Node node;
    node.setPosition(math::vec3(10.0f, 0.0f, 0.0f));
    TweenPtr tween = Tween::moveTo(math::vec3(20.0f, 4.0f, 0.0f), 2.0f);

    tween->start(node);
    EXPECT_EQ(tween->update(node, 0.5f), 0.0f);
    expectVec3(node.getPosition(), math::vec3(12.5f, 1.0f, 0.0f));
    EXPECT_FALSE(tween->isFinished());

    EXPECT_FLOAT_EQ(tween->update(node, 2.0f), 0.5f);
    expectVec3(node.getPosition(), math::vec3(20.0f, 4.0f, 0.0f));
    EXPECT_TRUE(tween->isFinished());
}

TEST(TweenTest, AppliesTheEasingCurve)
{
    Node node;
    TweenPtr tween = Tween::moveTo(math::vec3(100.0f, 0.0f, 0.0f), 1.0f, Ease::InQuad);
    tween->start(node);
    tween->update(node, 0.5f);
    EXPECT_FLOAT_EQ(node.getPosition().x, 25.0f);
}

TEST(TweenTest, RelativeTweensReadTheStartValuesWhenTheyBegin)
{
    Node node;
    node.setPosition(math::vec3(1.0f, 1.0f, 1.0f));
    node.setRotation(math::vec3(0.0f, 0.0f, 90.0f));
    node.setScale(math::vec3(2.0f, 2.0f, 1.0f));

    TweenPtr move = Tween::moveBy(math::vec3(1.0f, 2.0f, 3.0f), 1.0f);
    TweenPtr rotate = Tween::rotateBy(math::vec3(0.0f, 0.0f, 360.0f), 1.0f);
    TweenPtr scale = Tween::scaleBy(math::vec3(3.0f, 0.5f, 1.0f), 1.0f);

    // Changed after the tweens were made, but before they begin
    node.setPosition(math::vec3(5.0f, 5.0f, 5.0f));

    for (TweenPtr* tween : {&move, &rotate, &scale}) {
        (*tween)->start(node);
        (*tween)->update(node, 1.0f);
    }
    expectVec3(node.getPosition(), math::vec3(6.0f, 7.0f, 8.0f));
    expectVec3(node.getRotation(), math::vec3(0.0f, 0.0f, 450.0f));
    expectVec3(node.getScale(), math::vec3(6.0f, 1.0f, 1.0f));
}

TEST(TweenTest, ZeroDurationFinishesAtOnce)
{
    Node node;
    TweenPtr tween = Tween::scaleTo(math::vec3(3.0f), 0.0f);
    tween->start(node);
    EXPECT_FLOAT_EQ(tween->update(node, 0.25f), 0.25f);
    EXPECT_TRUE(tween->isFinished());
    expectVec3(node.getScale(), math::vec3(3.0f));
}

TEST(TweenTest, CustomGetsTheEasedProgress)
{
    Node node;
    std::vector<float> progress;
    TweenPtr tween =
        Tween::custom([&progress](float p) { progress.push_back(p); }, 1.0f, Ease::InQuad);
    tween->start(node);
    tween->update(node, 0.5f);
    tween->update(node, 0.5f);
    EXPECT_EQ(progress, (std::vector<float>{0.25f, 1.0f}));
}

TEST(TweenTest, SequencePassesLeftoverTimeToTheNextTween)
{
    Node node;
    TweenPtr tween = Tween::sequence(Tween::moveTo(math::vec3(10.0f, 0.0f, 0.0f), 1.0f),
                                     Tween::moveBy(math::vec3(0.0f, 10.0f, 0.0f), 1.0f));
    tween->start(node);

    tween->update(node, 1.5f);
    expectVec3(node.getPosition(), math::vec3(10.0f, 5.0f, 0.0f));

    EXPECT_FLOAT_EQ(tween->update(node, 1.0f), 0.5f);
    expectVec3(node.getPosition(), math::vec3(10.0f, 10.0f, 0.0f));
    EXPECT_TRUE(tween->isFinished());
}

TEST(TweenTest, SequenceRunsDelaysAndCallsInOrder)
{
    Node node;
    std::string log;
    TweenPtr tween = Tween::sequence(Tween::call([&log] { log += "a"; }), Tween::delay(1.0f),
                                     Tween::call([&log] { log += "b"; }));
    tween->start(node);

    tween->update(node, 0.0f);
    EXPECT_EQ(log, "a");
    tween->update(node, 0.9f);
    EXPECT_EQ(log, "a");
    tween->update(node, 0.1f);
    EXPECT_EQ(log, "ab");
    EXPECT_TRUE(tween->isFinished());
}

TEST(TweenTest, EmptyCompositesFinishAtOnce)
{
    Node node;
    for (TweenPtr& tween : [] {
             std::vector<TweenPtr> tweens;
             tweens.push_back(Tween::sequence(std::vector<TweenPtr>()));
             tweens.push_back(Tween::parallel(std::vector<TweenPtr>()));
             return tweens;
         }()) {
        tween->start(node);
        EXPECT_FLOAT_EQ(tween->update(node, 0.5f), 0.5f);
        EXPECT_TRUE(tween->isFinished());
    }
}

TEST(TweenTest, ParallelFinishesWithTheLongestTween)
{
    Node node;
    TweenPtr tween = Tween::parallel(Tween::moveTo(math::vec3(10.0f, 0.0f, 0.0f), 1.0f),
                                     Tween::scaleTo(math::vec3(3.0f), 2.0f));
    tween->start(node);

    tween->update(node, 1.5f);
    EXPECT_FALSE(tween->isFinished());
    expectVec3(node.getPosition(), math::vec3(10.0f, 0.0f, 0.0f));
    expectVec3(node.getScale(), math::vec3(2.5f));

    EXPECT_FLOAT_EQ(tween->update(node, 1.0f), 0.5f);
    EXPECT_TRUE(tween->isFinished());
    expectVec3(node.getScale(), math::vec3(3.0f));
}

TEST(TweenTest, RepeatRunsTheTweenTheGivenNumberOfTimes)
{
    Node node;
    TweenPtr tween = Tween::repeat(Tween::moveBy(math::vec3(1.0f, 0.0f, 0.0f), 1.0f), 3);
    tween->start(node);

    // One long update covers two runs and half of the third
    tween->update(node, 2.5f);
    EXPECT_FALSE(tween->isFinished());
    EXPECT_FLOAT_EQ(node.getPosition().x, 2.5f);

    EXPECT_FLOAT_EQ(tween->update(node, 1.0f), 0.5f);
    EXPECT_TRUE(tween->isFinished());
    EXPECT_FLOAT_EQ(node.getPosition().x, 3.0f);
}

TEST(TweenTest, RepeatForeverGoesBackAndForth)
{
    Node node;
    TweenPtr tween = Tween::repeat(Tween::sequence(Tween::moveBy(math::vec3(1.0f, 0.0f, 0.0f), 1.0f),
                                                   Tween::moveBy(math::vec3(-1.0f, 0.0f, 0.0f), 1.0f)));
    tween->start(node);
    for (int i = 0; i < 10; ++i) {
        tween->update(node, 0.5f);
    }
    EXPECT_FALSE(tween->isFinished());
    EXPECT_NEAR(node.getPosition().x, 1.0f, 1e-4f); // 5 seconds: 2 full runs and 1 second
}

TEST(TweenTest, RepeatForeverOfAnInstantTweenRunsOncePerUpdate)
{
    Node node;
    int calls = 0;
    TweenPtr tween = Tween::repeat(Tween::call([&calls] { ++calls; }));
    tween->start(node);
    tween->update(node, 1.0f);
    tween->update(node, 1.0f);
    EXPECT_EQ(calls, 2);
}

// ---- Tweener -------------------------------------------------------------------------------------

TEST(TweenerTest, BeginsTweensAtTheNextUpdate)
{
    Node node;
    Tweener* tweener = node.addComponent<Tweener>();
    const Tweener::TweenId id = tweener->run(Tween::moveBy(math::vec3(10.0f, 0.0f, 0.0f), 1.0f));
    EXPECT_NE(id, 0u);
    EXPECT_TRUE(tweener->isRunning(id));

    // Set up after run(): the move starts from here
    node.setPosition(math::vec3(5.0f, 0.0f, 0.0f));

    node.update(0.5f);
    EXPECT_FLOAT_EQ(node.getPosition().x, 10.0f);
    node.update(0.5f);
    EXPECT_FLOAT_EQ(node.getPosition().x, 15.0f);
    EXPECT_FALSE(tweener->isRunning(id));
    EXPECT_FALSE(tweener->isRunning());
}

TEST(TweenerTest, StopLeavesTheNodeWhereItIs)
{
    Node node;
    Tweener* tweener = node.addComponent<Tweener>();
    const Tweener::TweenId id = tweener->run(Tween::moveTo(math::vec3(10.0f, 0.0f, 0.0f), 1.0f));
    node.update(0.5f);
    tweener->stop(id);
    node.update(0.5f);
    EXPECT_FLOAT_EQ(node.getPosition().x, 5.0f);
    EXPECT_FALSE(tweener->isRunning(id));
}

TEST(TweenerTest, PauseAndTimeScale)
{
    Node node;
    Tweener* tweener = node.addComponent<Tweener>();
    tweener->run(Tween::moveTo(math::vec3(10.0f, 0.0f, 0.0f), 1.0f));

    tweener->setPaused(true);
    node.update(0.5f);
    EXPECT_FLOAT_EQ(node.getPosition().x, 0.0f);

    tweener->setPaused(false);
    tweener->setTimeScale(0.5f);
    node.update(0.5f);
    EXPECT_FLOAT_EQ(node.getPosition().x, 2.5f);
}

TEST(TweenerTest, CallbacksCanRunAndStopTweens)
{
    Node node;
    Tweener* tweener = node.addComponent<Tweener>();
    Tweener::TweenId spin = tweener->run(Tween::rotateBy(math::vec3(0.0f, 0.0f, 90.0f), 10.0f));
    Tweener::TweenId chained = 0;
    tweener->run(Tween::call([&] {
        tweener->stop(spin);
        chained = tweener->run(Tween::moveTo(math::vec3(1.0f, 0.0f, 0.0f), 1.0f));
    }));

    node.update(1.0f);
    EXPECT_FALSE(tweener->isRunning(spin));
    EXPECT_TRUE(tweener->isRunning(chained));
    EXPECT_FLOAT_EQ(node.getPosition().x, 0.0f); // Begins at the next update

    node.update(1.0f);
    EXPECT_FLOAT_EQ(node.getPosition().x, 1.0f);
    EXPECT_FALSE(tweener->isRunning());
}

TEST(TweenerTest, IgnoresNullTweens)
{
    Node node;
    Tweener* tweener = node.addComponent<Tweener>();
    EXPECT_EQ(tweener->run(nullptr), 0u);
    EXPECT_FALSE(tweener->isRunning());
}
