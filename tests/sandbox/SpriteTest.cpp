#include "SpriteTest.h"

#include <ocf/core/Engine.h>
#include <ocf/resource/TextureManager.h>
#include <ocf/scene/Camera.h>
#include <ocf/scene/Node.h>
#include <ocf/scene/SceneGraph.h>

#include <imgui.h>

using namespace ocf;

void SpriteTest::onEnter()
{
    initializeScene();

    const math::ivec2 size = getEngine().getWindowSize();
    m_camera->orthographic(0.0f, static_cast<float>(size.x), 0.0f, static_cast<float>(size.y));

    TextureManager& textureManager = getEngine().getTextureManager();
    Texture* texture = textureManager.addImage("textures/test-texture.png");

    m_spriteNode = m_world->getRoot()->createChild();
    m_sprite = m_spriteNode->addComponent<Sprite>(getEngine(), texture);

    m_spriteNode->setPosition(math::vec3(static_cast<float>(size.x) / 2.0f,
                                         static_cast<float>(size.y) / 2.0f, 0.0f));
    m_sprite->setPixelsPerUnit(1.0f);
}

void SpriteTest::onExit()
{
}

void SpriteTest::onImGui()
{
    ImGui::Begin("SpriteTest");

    bool flipX = m_sprite->isFlipX();
    if (ImGui::Checkbox("Flip Horizontally",&flipX)) {
        m_sprite->setFlipX(flipX);
    }

    bool flipY = m_sprite->isFlipY();
    if (ImGui::Checkbox("Flip Vertically", &flipY)) {
        m_sprite->setFlipY(flipY);
    }
    
    float anchor[2] = {m_sprite->getAnchor().x, m_sprite->getAnchor().y};
    if (ImGui::SliderFloat2("Anchor", anchor, 0.0f, 1.0f)) {
        m_sprite->setAnchor(math::vec2(anchor[0], anchor[1]));
    }

    ImGui::End();
}
