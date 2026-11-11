// SPDX-License-Identifier: MIT
#include "ocf/resource/SpriteAnimation.h"
#include "ocf/resource/SpriteSheet.h"
#include "ocf/scene/2d/SpriteAnimator.h"
#include "ocf/scene/Node.h"

#include <gtest/gtest.h>

#include <limits>
#include <string>
#include <vector>

using namespace ocf;

namespace {

SpriteFrame makeFrame(std::string name)
{
    SpriteFrame frame = SpriteFrame::fromRect(math::Rect(0.0f, 0.0f, 16.0f, 16.0f));
    frame.name = std::move(name);
    return frame;
}

/** Frames "<name>0" to "<name><count - 1>", each shown for 1 second. */
SpriteAnimation makeAnimation(std::string name, int count, AnimationLoop loop)
{
    SpriteAnimation animation;
    animation.loop = loop;
    for (int i = 0; i < count; ++i) {
        animation.frames.push_back(SpriteAnimationFrame{makeFrame(name + std::to_string(i)), 1.0f});
    }
    animation.name = std::move(name);
    return animation;
}

/** The frame index after each of `steps` updates of `deltaTime`. */
std::vector<size_t> run(SpriteAnimator& animator, int steps, float deltaTime = 1.0f)
{
    std::vector<size_t> indices;
    for (int i = 0; i < steps; ++i) {
        animator.update(deltaTime);
        indices.push_back(animator.getFrameIndex());
    }
    return indices;
}

} // namespace

// ---- SpriteAnimation -----------------------------------------------------------------------------

TEST(SpriteAnimationTest, FromSheetTakesThePrefixedFramesInNameOrder)
{
    SpriteSheet sheet;
    sheet.addFrame(makeFrame("walk_02.png"));
    sheet.addFrame(makeFrame("idle_01.png"));
    sheet.addFrame(makeFrame("walk_01.png"));

    const SpriteAnimation animation =
        SpriteAnimation::fromSheet("walk", sheet, "walk_", 4.0f, AnimationLoop::PingPong);

    ASSERT_EQ(animation.frames.size(), 2u);
    EXPECT_EQ(animation.name, "walk");
    EXPECT_EQ(animation.loop, AnimationLoop::PingPong);
    EXPECT_EQ(animation.frames[0].frame.name, "walk_01.png");
    EXPECT_EQ(animation.frames[1].frame.name, "walk_02.png");
    EXPECT_FLOAT_EQ(animation.frames[0].duration, 0.25f);
    EXPECT_FLOAT_EQ(animation.getDuration(), 0.5f);
    EXPECT_TRUE(animation.isValid());
}

TEST(SpriteAnimationTest, IsNotValidWithoutFramesOrPositiveDurations)
{
    SpriteSheet sheet;
    sheet.addFrame(makeFrame("walk_01.png"));

    EXPECT_FALSE(SpriteAnimation::fromSheet("none", sheet, "run_", 4.0f).isValid());
    EXPECT_FALSE(SpriteAnimation::fromSheet("still", sheet, "walk_", 0.0f).isValid());

    SpriteAnimation nan = makeAnimation("nan", 2, AnimationLoop::Loop);
    nan.frames[1].duration = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(nan.isValid());
}

// ---- SpriteAnimator ------------------------------------------------------------------------------

TEST(SpriteAnimatorTest, LoopGoesBackToTheFirstFrame)
{
    SpriteAnimator animator(nullptr);
    ASSERT_TRUE(animator.addAnimation(makeAnimation("a", 3, AnimationLoop::Loop)));
    ASSERT_TRUE(animator.play("a"));

    EXPECT_EQ(animator.getFrameIndex(), 0u);
    EXPECT_EQ(run(animator, 4), (std::vector<size_t>{1, 2, 0, 1}));
    EXPECT_TRUE(animator.isPlaying());
}

TEST(SpriteAnimatorTest, OnceStopsOnTheLastFrame)
{
    SpriteAnimator animator(nullptr);
    animator.addAnimation(makeAnimation("a", 3, AnimationLoop::Once));
    int finished = 0;
    animator.setOnFinished([&finished](const SpriteAnimation& animation) {
        EXPECT_EQ(animation.name, "a");
        ++finished;
    });
    animator.play("a");

    EXPECT_EQ(run(animator, 4), (std::vector<size_t>{1, 2, 2, 2}));
    EXPECT_TRUE(animator.isFinished());
    EXPECT_FALSE(animator.isPlaying());
    EXPECT_EQ(finished, 1);
}

TEST(SpriteAnimatorTest, PingPongShowsTheEndFramesOncePerTurn)
{
    SpriteAnimator animator(nullptr);
    animator.addAnimation(makeAnimation("a", 3, AnimationLoop::PingPong));
    animator.play("a");
    EXPECT_EQ(run(animator, 6), (std::vector<size_t>{1, 2, 1, 0, 1, 2}));
}

TEST(SpriteAnimatorTest, SingleFrameAnimationsStayOnTheirFrame)
{
    for (AnimationLoop loop : {AnimationLoop::Once, AnimationLoop::Loop, AnimationLoop::PingPong}) {
        SpriteAnimator animator(nullptr);
        animator.addAnimation(makeAnimation("a", 1, loop));
        animator.play("a");
        EXPECT_EQ(run(animator, 3), (std::vector<size_t>{0, 0, 0}));
    }
}

TEST(SpriteAnimatorTest, LongUpdatesSkipFramesAndKeepTheRemainder)
{
    SpriteAnimator animator(nullptr);
    animator.addAnimation(makeAnimation("a", 5, AnimationLoop::Loop));
    animator.play("a");

    animator.update(2.5f);
    EXPECT_EQ(animator.getFrameIndex(), 2u);
    animator.update(0.5f); // Completes the half second left over
    EXPECT_EQ(animator.getFrameIndex(), 3u);
}

TEST(SpriteAnimatorTest, FrameChangedIsCalledOncePerUpdateWithTheLastFrame)
{
    SpriteAnimator animator(nullptr);
    animator.addAnimation(makeAnimation("a", 5, AnimationLoop::Loop));
    std::vector<size_t> changes;
    animator.setOnFrameChanged(
        [&changes](const SpriteAnimation&, size_t frameIndex) { changes.push_back(frameIndex); });

    animator.play("a");
    animator.update(0.5f); // No change
    animator.update(2.5f);
    EXPECT_EQ(changes, (std::vector<size_t>{0, 3}));
}

TEST(SpriteAnimatorTest, PlayingTheSameAnimationGoesOnUnlessRestarted)
{
    SpriteAnimator animator(nullptr);
    animator.addAnimation(makeAnimation("a", 3, AnimationLoop::Loop));
    animator.play("a");
    animator.update(1.0f);

    animator.play("a");
    EXPECT_EQ(animator.getFrameIndex(), 1u);

    animator.play("a", true);
    EXPECT_EQ(animator.getFrameIndex(), 0u);
}

TEST(SpriteAnimatorTest, PlayingAFinishedAnimationStartsItAgain)
{
    SpriteAnimator animator(nullptr);
    animator.addAnimation(makeAnimation("a", 2, AnimationLoop::Once));
    animator.play("a");
    run(animator, 2);
    ASSERT_TRUE(animator.isFinished());

    animator.play("a");
    EXPECT_FALSE(animator.isFinished());
    EXPECT_EQ(animator.getFrameIndex(), 0u);
}

TEST(SpriteAnimatorTest, SwitchesAnimations)
{
    SpriteAnimator animator(nullptr);
    animator.addAnimation(makeAnimation("idle", 2, AnimationLoop::Loop));
    animator.addAnimation(makeAnimation("walk", 4, AnimationLoop::Loop));
    animator.play("idle");
    animator.update(1.0f);

    animator.play("walk");
    ASSERT_NE(animator.getCurrentAnimation(), nullptr);
    EXPECT_EQ(animator.getCurrentAnimation()->name, "walk");
    EXPECT_EQ(animator.getFrameIndex(), 0u);
}

TEST(SpriteAnimatorTest, RejectsUnknownAndInvalidAnimations)
{
    SpriteAnimator animator(nullptr);
    EXPECT_FALSE(animator.play("missing"));
    EXPECT_EQ(animator.getCurrentAnimation(), nullptr);
    EXPECT_FALSE(animator.isPlaying());

    EXPECT_FALSE(animator.addAnimation(makeAnimation("empty", 0, AnimationLoop::Loop)));
    EXPECT_EQ(animator.getAnimation("empty"), nullptr);
}

TEST(SpriteAnimatorTest, ReplacingThePlayingAnimationStartsItAgain)
{
    SpriteAnimator animator(nullptr);
    animator.addAnimation(makeAnimation("a", 5, AnimationLoop::Loop));
    animator.play("a");
    animator.update(4.0f);
    ASSERT_EQ(animator.getFrameIndex(), 4u);

    animator.addAnimation(makeAnimation("a", 2, AnimationLoop::Loop));
    EXPECT_EQ(animator.getFrameIndex(), 0u);
    EXPECT_EQ(run(animator, 2), (std::vector<size_t>{1, 0}));
}

TEST(SpriteAnimatorTest, StopPauseAndSpeed)
{
    SpriteAnimator animator(nullptr);
    animator.addAnimation(makeAnimation("a", 4, AnimationLoop::Loop));
    animator.play("a");

    animator.setPaused(true);
    animator.update(1.0f);
    EXPECT_EQ(animator.getFrameIndex(), 0u);
    animator.setPaused(false);

    animator.setSpeed(2.0f);
    animator.update(1.0f);
    EXPECT_EQ(animator.getFrameIndex(), 2u);

    animator.stop();
    EXPECT_FALSE(animator.isPlaying());
    animator.update(1.0f);
    EXPECT_EQ(animator.getFrameIndex(), 2u);
}

TEST(SpriteAnimatorTest, SetFrameIndexJumpsWithinTheAnimation)
{
    SpriteAnimator animator(nullptr);
    animator.addAnimation(makeAnimation("a", 4, AnimationLoop::Loop));
    animator.play("a");
    animator.update(0.5f);

    animator.setFrameIndex(3);
    EXPECT_EQ(animator.getFrameIndex(), 3u);
    animator.update(0.5f); // The frame shows from its beginning
    EXPECT_EQ(animator.getFrameIndex(), 3u);

    animator.setFrameIndex(10);
    EXPECT_EQ(animator.getFrameIndex(), 3u);
}

TEST(SpriteAnimatorTest, CallbacksCanPlayAnotherAnimation)
{
    Node node;
    SpriteAnimator* animator = node.addComponent<SpriteAnimator>(nullptr);
    animator->addAnimation(makeAnimation("attack", 2, AnimationLoop::Once));
    animator->addAnimation(makeAnimation("idle", 2, AnimationLoop::Loop));
    animator->setOnFinished([animator](const SpriteAnimation&) { animator->play("idle"); });

    animator->play("attack");
    node.update(1.0f);
    node.update(1.0f);
    EXPECT_EQ(animator->getCurrentAnimation()->name, "idle");
    EXPECT_TRUE(animator->isPlaying());
    EXPECT_EQ(animator->getFrameIndex(), 0u);
}
