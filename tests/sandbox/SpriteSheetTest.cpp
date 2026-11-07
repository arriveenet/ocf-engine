// SPDX-License-Identifier: MIT
#include "SpriteSheetTest.h"

#include <ocf/core/Engine.h>
#include <ocf/core/Logger.h>
#include <ocf/resource/SpriteSheetLoader.h>
#include <ocf/scene/Camera.h>
#include <ocf/scene/Node.h>
#include <ocf/scene/SceneGraph.h>

#include <imgui.h>

using namespace ocf;

void SpriteSheetTest::onEnter()
{
    initializeScene();

    const math::ivec2 size = getEngine().getWindowSize();
    const float width = static_cast<float>(size.x);
    const float height = static_cast<float>(size.y);
    m_camera->orthographic(0.0f, width, 0.0f, height);

    auto result = SpriteSheetLoader::load(getEngine(), "spritesheets/sample.json");
    if (result.isErr()) {
        OCF_LOG_ERROR("SpriteSheetTest: {}", result.unwrapErr());
        return;
    }
    m_sheet = result.unwrap();
    m_frames = m_sheet->getFramesWithPrefix("walk_");

    // Every frame at its original size, top-left anchored, so trimming is visible
    float x = 40.0f;
    for (const SpriteFrame* frame : m_frames) {
        Node* node = m_world->getRoot()->createChild();
        node->setPosition(math::vec3(x, height - 60.0f, 0.0f));
        Sprite* sprite = node->addComponent<Sprite>(getEngine(), *frame, m_sheet->getTexture());
        sprite->setAnchor(math::vec2(0.0f, 0.0f));
        sprite->setTextureFilter(TextureFilter::Nearest);
        m_sprites.push_back(sprite);
        x += frame->sourceSize.x * m_scale + 20.0f;
    }

    Node* node = m_world->getRoot()->createChild();
    node->setPosition(math::vec3(width * 0.5f, height * 0.25f, 0.0f));
    m_animated = node->addComponent<Sprite>(getEngine());
    m_animated->setTextureFilter(TextureFilter::Nearest);
    m_sprites.push_back(m_animated);

    for (Sprite* sprite : m_sprites) {
        sprite->setPixelsPerUnit(1.0f / m_scale);
    }
    applyFrame();
}

void SpriteSheetTest::onExit()
{
    m_sprites.clear();
    m_frames.clear();
    m_animated = nullptr;
    m_sheet = Ref<SpriteSheet>();
}

void SpriteSheetTest::onUpdate(float deltaTime)
{
    if (!m_playing || m_frames.empty() || m_framesPerSecond <= 0.0f) {
        return;
    }

    m_elapsed += deltaTime;
    const float frameTime = 1.0f / m_framesPerSecond;
    if (m_elapsed >= frameTime) {
        m_elapsed -= frameTime;
        m_frameIndex = (m_frameIndex + 1) % static_cast<int>(m_frames.size());
        applyFrame();
    }
}

void SpriteSheetTest::applyFrame()
{
    if (m_animated == nullptr || m_frames.empty()) {
        return;
    }
    const SpriteFrame& frame = *m_frames[m_frameIndex];
    m_animated->setSpriteFrame(frame, m_sheet->getTexture());
    m_animated->setAnchor(frame.pivot);
}

void SpriteSheetTest::onImGui()
{
    ImGui::Begin("SpriteSheetTest");

    if (m_sheet == nullptr) {
        ImGui::TextUnformatted("spritesheets/sample.json could not be loaded");
        ImGui::End();
        return;
    }

    ImGui::Text("%s: %zu frames", m_sheet->getImagePath().c_str(), m_sheet->getFrameCount());
    ImGui::Checkbox("Play", &m_playing);
    ImGui::SliderFloat("FPS", &m_framesPerSecond, 1.0f, 30.0f);

    const SpriteFrame& current = *m_frames[m_frameIndex];
    if (ImGui::BeginCombo("Frame", current.name.c_str())) {
        for (int i = 0; i < static_cast<int>(m_frames.size()); ++i) {
            if (ImGui::Selectable(m_frames[i]->name.c_str(), i == m_frameIndex)) {
                m_frameIndex = i;
                m_playing = false;
                applyFrame();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::Text("rotated: %s, offset: (%.0f, %.0f)", current.rotated ? "yes" : "no",
                current.offset.x, current.offset.y);

    bool changed = ImGui::Checkbox("Flip Horizontally", &m_flipX);
    changed |= ImGui::Checkbox("Flip Vertically", &m_flipY);
    if (changed) {
        for (Sprite* sprite : m_sprites) {
            sprite->setFlipX(m_flipX);
            sprite->setFlipY(m_flipY);
        }
    }

    if (ImGui::SliderFloat("Scale", &m_scale, 1.0f, 6.0f)) {
        for (Sprite* sprite : m_sprites) {
            sprite->setPixelsPerUnit(1.0f / m_scale);
        }
    }

    ImGui::End();
}
