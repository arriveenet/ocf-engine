// SPDX-License-Identifier: MIT
#pragma once

#include "TestBaseScene.h"

#include <ocf/core/Reference.h>
#include <ocf/resource/SpriteSheet.h>
#include <ocf/scene/2d/Sprite.h>

#include <vector>

/**
 * Frames of a TexturePacker sprite sheet (some trimmed and rotated): every frame in a row at
 * the top, and an animation of them below with the pivot of the frames as its anchor.
 */
class SpriteSheetTest : public TestBaseScene {
public:
    void onEnter() override;
    void onExit() override;
    void onUpdate(float deltaTime) override;
    void onImGui() override;

private:
    void applyFrame();

    ocf::Ref<ocf::SpriteSheet> m_sheet;
    std::vector<const ocf::SpriteFrame*> m_frames;
    std::vector<ocf::Sprite*> m_sprites;
    ocf::Sprite* m_animated = nullptr;

    int m_frameIndex = 0;
    float m_elapsed = 0.0f;
    float m_framesPerSecond = 6.0f;
    float m_scale = 3.0f; // Screen pixels per sprite pixel
    bool m_playing = true;
    bool m_flipX = false;
    bool m_flipY = false;
};
