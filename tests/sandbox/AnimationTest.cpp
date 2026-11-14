// SPDX-License-Identifier: MIT
#include "AnimationTest.h"

#include <ocf/core/Engine.h>
#include <ocf/core/Logger.h>
#include <ocf/resource/FontManager.h>
#include <ocf/resource/SpriteAnimation.h>
#include <ocf/resource/SpriteSheetLoader.h>
#include <ocf/resource/TextureManager.h>
#include <ocf/scene/2d/Text.h>
#include <ocf/scene/Camera.h>
#include <ocf/scene/Node.h>
#include <ocf/scene/SceneGraph.h>

#include <imgui.h>

#include <array>

using namespace ocf;

namespace {

struct EasingEntry {
    Ease curve;
    const char* name;
};

constexpr std::array<EasingEntry, 8> EASINGS = {{
    {Ease::Linear, "Linear"},
    {Ease::InOutQuad, "InOutQuad"},
    {Ease::OutCubic, "OutCubic"},
    {Ease::InOutSine, "InOutSine"},
    {Ease::InOutExpo, "InOutExpo"},
    {Ease::OutBack, "OutBack"},
    {Ease::OutElastic, "OutElastic"},
    {Ease::OutBounce, "OutBounce"},
}};

constexpr float EASING_LEFT = 180.0f;
constexpr float EASING_MARGIN = 80.0f;
constexpr float SPRITE_SCALE = 3.0f; // Screen pixels per sprite pixel

} // namespace

void AnimationTest::onEnter()
{
    initializeScene();

    const math::ivec2 size = getEngine().getWindowSize();
    m_width = static_cast<float>(size.x);
    m_height = static_cast<float>(size.y);
    m_camera->orthographic(0.0f, m_width, 0.0f, m_height);

    Texture* texture = getEngine().getTextureManager().addImage("textures/test-texture.png");

    // ---- Frame animation, moved by a tween ----
    auto result = SpriteSheetLoader::load(getEngine(), "spritesheets/sample.json");
    if (result.isErr()) {
        OCF_LOG_ERROR("AnimationTest: {}", result.unwrapErr());
    } else {
        m_sheet = result.unwrap();

        Node* node = m_world->getRoot()->createChild();
        node->setPosition(math::vec3(EASING_LEFT, m_height * 0.15f, 0.0f));
        m_walker = node->addComponent<Sprite>(getEngine());
        m_walker->setTextureFilter(TextureFilter::Nearest);
        m_walker->setPixelsPerUnit(1.0f / SPRITE_SCALE);

        m_walkerAnimator = node->addComponent<SpriteAnimator>(m_walker);
        m_walkerAnimator->setUseFramePivot(true);
        m_walkerAnimator->addAnimation(SpriteAnimation::fromSheet(
            "walk", *m_sheet.ptr(), "walk_", 8.0f, static_cast<AnimationLoop>(m_loop)));
        m_walkerAnimator->play("walk");

        m_walkerTweener = node->addComponent<Tweener>();
        runWalk();
    }

    // ---- One sprite per easing curve ----
    FontTrueTypeConfig config;
    config.fontPath = "fonts/NotoSansJP-Regular.ttf";
    config.fontSize = 16;
    Ref<Font> font = getEngine().getFontManager().loadFontTTF(config);

    const float top = m_height - 60.0f;
    const float rowHeight = (m_height * 0.55f) / static_cast<float>(EASINGS.size());
    for (size_t i = 0; i < EASINGS.size(); ++i) {
        const float y = top - rowHeight * static_cast<float>(i);

        Node* label = m_world->getRoot()->createChild();
        label->setPosition(math::vec3(20.0f, y, 0.0f));
        Text* text = label->addComponent<Text>(getEngine(), font);
        text->setPixelsPerUnit(1.0f);
        text->setAnchor(math::vec2(0.0f, 0.5f));
        text->setString(EASINGS[i].name);

        Node* node = m_world->getRoot()->createChild();
        node->setPosition(math::vec3(EASING_LEFT, y, 0.0f));
        Sprite* sprite = node->addComponent<Sprite>(getEngine(), texture);
        sprite->setPixelsPerUnit(128.0f / (rowHeight * 0.8f));
        m_easingNodes.push_back(node);
        m_easingTweeners.push_back(node->addComponent<Tweener>());
    }
    runEasing();

    // ---- Several tweens at once ----
    Node* node = m_world->getRoot()->createChild();
    node->setPosition(math::vec3(m_width * 0.75f, m_height * 0.15f, 0.0f));
    m_pulse = node->addComponent<Sprite>(getEngine(), texture);
    m_pulse->setPixelsPerUnit(1.0f);
    m_pulseTweener = node->addComponent<Tweener>();
    runPulse();
}

void AnimationTest::onExit()
{
    m_easingTweeners.clear();
    m_easingNodes.clear();
    m_walker = nullptr;
    m_walkerAnimator = nullptr;
    m_walkerTweener = nullptr;
    m_pulse = nullptr;
    m_pulseTweener = nullptr;
    m_sheet = Ref<SpriteSheet>();
}

void AnimationTest::runWalk()
{
    // Walk right, turn around, walk left, turn around, and so on
    const float distance = m_width * 0.5f - EASING_LEFT;
    Sprite* walker = m_walker;
    m_walkerTweener->stopAll();
    m_walkerTweener->getOwner()->setPosition(math::vec3(EASING_LEFT, m_height * 0.15f, 0.0f));
    m_walker->setFlipX(false);
    m_walkerTweener->run(Tween::repeat(Tween::sequence(
        Tween::moveBy(math::vec3(distance, 0.0f, 0.0f), 3.0f),
        Tween::call([walker] { walker->setFlipX(true); }),
        Tween::moveBy(math::vec3(-distance, 0.0f, 0.0f), 3.0f),
        Tween::call([walker] { walker->setFlipX(false); }))));
}

void AnimationTest::runEasing()
{
    const float right = m_width - EASING_MARGIN;
    for (size_t i = 0; i < m_easingTweeners.size(); ++i) {
        Tweener* tweener = m_easingTweeners[i];
        Node* node = m_easingNodes[i];
        const Ease curve = EASINGS[i].curve;
        const float y = node->getPosition().y;

        tweener->stopAll();
        node->setPosition(math::vec3(EASING_LEFT, y, 0.0f));
        tweener->run(Tween::repeat(Tween::sequence(
            Tween::moveTo(math::vec3(right, y, 0.0f), 1.5f, curve), Tween::delay(0.5f),
            Tween::moveTo(math::vec3(EASING_LEFT, y, 0.0f), 1.5f, curve), Tween::delay(0.5f))));
    }
}

void AnimationTest::runPulse()
{
    // Grow while turning and turning red, then shrink back while fading out and in
    Node* node = m_pulseTweener->getOwner();
    m_pulseTweener->stopAll();
    node->setScale(math::vec3(1.0f));
    node->setRotation(math::vec3(0.0f));
    m_pulse->setColor(math::vec4(1.0f));
    m_pulseTweener->run(Tween::repeat(Tween::sequence(
        Tween::parallel(Tween::scaleTo(math::vec3(1.5f, 1.5f, 1.0f), 0.8f, Ease::OutBack),
                        Tween::rotateBy(math::vec3(0.0f, 0.0f, 180.0f), 0.8f, Ease::InOutSine),
                        Tween::colorTo(m_pulse, math::vec4(1.0f, 0.4f, 0.4f, 1.0f), 0.8f)),
        Tween::parallel(Tween::scaleTo(math::vec3(1.0f, 1.0f, 1.0f), 0.8f, Ease::InOutQuad),
                        Tween::colorTo(m_pulse, math::vec4(1.0f), 0.8f),
                        Tween::sequence(Tween::fadeTo(m_pulse, 0.2f, 0.4f),
                                        Tween::fadeTo(m_pulse, 1.0f, 0.4f))),
        Tween::delay(0.3f))));
}

void AnimationTest::onImGui()
{
    ImGui::Begin("AnimationTest");

    bool changed = ImGui::Checkbox("Pause tweens", &m_paused);
    changed |= ImGui::SliderFloat("Tween time scale", &m_timeScale, 0.0f, 3.0f);
    if (changed) {
        for (Tweener* tweener : m_easingTweeners) {
            tweener->setPaused(m_paused);
            tweener->setTimeScale(m_timeScale);
        }
        for (Tweener* tweener : {m_walkerTweener, m_pulseTweener}) {
            if (tweener != nullptr) {
                tweener->setPaused(m_paused);
                tweener->setTimeScale(m_timeScale);
            }
        }
    }
    if (ImGui::Button("Restart tweens")) {
        runEasing();
        runPulse();
        if (m_walkerTweener != nullptr) {
            runWalk();
        }
    }

    ImGui::SeparatorText("Walk animation");
    if (m_walkerAnimator == nullptr) {
        ImGui::TextUnformatted("spritesheets/sample.json could not be loaded");
        ImGui::End();
        return;
    }

    bool paused = m_walkerAnimator->isPaused();
    if (ImGui::Checkbox("Pause animation", &paused)) {
        m_walkerAnimator->setPaused(paused);
    }
    if (ImGui::SliderFloat("Animation speed", &m_animationSpeed, 0.0f, 3.0f)) {
        m_walkerAnimator->setSpeed(m_animationSpeed);
    }
    if (ImGui::Combo("Loop", &m_loop, "Once\0Loop\0PingPong\0")) {
        // Replacing the animation that is playing starts it again
        m_walkerAnimator->addAnimation(SpriteAnimation::fromSheet(
            "walk", *m_sheet.ptr(), "walk_", 8.0f, static_cast<AnimationLoop>(m_loop)));
    }
    if (ImGui::Button("Play from start")) {
        m_walkerAnimator->play("walk", true);
    }
    ImGui::Text("Frame %zu, %s", m_walkerAnimator->getFrameIndex(),
                m_walkerAnimator->isFinished() ? "finished"
                : m_walkerAnimator->isPlaying() ? "playing"
                                                : "stopped");

    ImGui::End();
}
