// SPDX-License-Identifier: MIT
#pragma once

#include "TestBaseScene.h"

#include <ocf/core/Reference.h>
#include <ocf/resource/SpriteSheet.h>
#include <ocf/scene/2d/Sprite.h>
#include <ocf/scene/2d/SpriteAnimator.h>
#include <ocf/scene/animation/Tweener.h>

#include <vector>

/**
 * SpriteAnimator and Tweener: a character walking back and forth with a frame animation, one
 * sprite per easing curve moving across the screen, and a sprite combining several tweens.
 */
class AnimationTest : public TestBaseScene {
public:
    void onEnter() override;
    void onExit() override;
    void onImGui() override;

private:
    void runWalk();
    void runEasing();
    void runPulse();

    ocf::Ref<ocf::SpriteSheet> m_sheet;

    ocf::Sprite* m_walker = nullptr;
    ocf::SpriteAnimator* m_walkerAnimator = nullptr;
    ocf::Tweener* m_walkerTweener = nullptr;

    std::vector<ocf::Tweener*> m_easingTweeners;
    std::vector<ocf::Node*> m_easingNodes;

    ocf::Sprite* m_pulse = nullptr;
    ocf::Tweener* m_pulseTweener = nullptr;

    float m_width = 0.0f;
    float m_height = 0.0f;
    float m_timeScale = 1.0f;
    float m_animationSpeed = 1.0f;
    int m_loop = 1; // AnimationLoop of the walk animation
    bool m_paused = false;
};
