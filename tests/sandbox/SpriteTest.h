// SPDX-License-Identifier: MIT
#pragma once

#include "TestBaseScene.h"

#include <ocf/scene/2d/Sprite.h>
#include <ocf/scene/Node.h>

class SpriteTest : public TestBaseScene {
public:
    void onEnter() override;
    void onExit() override;
    void onImGui() override;

private:
    ocf::Node* m_spriteNode = nullptr;
    ocf::Sprite* m_sprite = nullptr;
};
