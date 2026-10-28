// SPDX-License-Identifier: MIT
#pragma once

#include "TestBaseScene.h"

#include <ocf/core/Reference.h>
#include <ocf/math/vec4.h>
#include <ocf/resource/Mesh.h>
#include <ocf/scene/2d/Sprite.h>
#include <ocf/scene/Node.h>

namespace ocf {
class RenderTarget;
class Texture;
} // namespace ocf

/** Draws a model into a texture, then draws the texture on screen as a sprite */
class RenderTargetTest : public TestBaseScene {
public:
    static constexpr uint32_t TARGET_SIZE = 512;

    void onLoad() override;
    void onEnter() override;
    void onExit() override;
    void onUpdate(float deltaTime) override;
    void onImGui() override;

private:
    ocf::Ref<ocf::Mesh> m_mesh;
    ocf::Node* m_modelNode = nullptr;

    ocf::Texture* m_colorTexture = nullptr;
    ocf::Texture* m_depthTexture = nullptr;
    ocf::RenderTarget* m_renderTarget = nullptr;

    ocf::SceneGraph* m_screen = nullptr;
    ocf::Sprite* m_sprite = nullptr;

    ocf::math::vec4 m_targetClearColor = ocf::math::vec4(0.2f, 0.3f, 0.4f, 1.0f);
    float m_rotationSpeed = 45.0f; // Degrees per second
    float m_rotation = 0.0f;
};
