// SPDX-License-Identifier: MIT
#include "TestBaseScene.h"

#include "ocf/core/Engine.h"
#include "ocf/resource/Font.h"
#include "ocf/resource/FontManager.h"
#include "ocf/scene/Camera.h"
#include "ocf/scene/Node.h"
#include "ocf/scene/SceneGraph.h"
#include "ocf/scene/View.h"

using namespace ocf;

// ===== UIScene =====
void UIScene::setupUI(View::ClearFlags clearFlags)
{
    const math::vec2 size = getScreenSize();

    m_ui = createSceneGraph();
    Camera* camera = m_ui->getRoot()->createChild()->addComponent<Camera>();
    camera->orthographic(0.0f, size.x, 0.0f, size.y);

    View* view = createView();
    view->setScene(m_ui);
    view->setCamera(camera);
    view->setClearFlags(clearFlags);
    view->setClearColor(math::vec4(0.1f, 0.1f, 0.15f, 1.0f));
    view->setSortMode(View::SortMode::SubmitOrder);
}

ocf::Text* UIScene::addText(const ocf::math::vec2& position, const ocf::math::vec2& anchor,
                                  std::string_view string, int fontSize)
{
    FontTrueTypeConfig config;
    config.fontPath = "fonts/NotoSansJP-Regular.ttf";
    config.fontSize = fontSize;
    Ref<Font> font = getEngine().getFontManager().loadFontTTF(config);

    Node* node = m_ui->getRoot()->createChild();
    node->setPosition(math::vec3(position.x, position.y, 0.0f));

    Text* text = node->addComponent<Text>(getEngine(), font);
    text->setPixelsPerUnit(1.0f);
    text->setAnchor(anchor);
    text->setString(string);
    return text;
}

math::vec2 UIScene::getScreenSize() const
{
    const math::ivec2 size = getEngine().getWindowSize();
    return math::vec2(static_cast<float>(size.x), static_cast<float>(size.y));
}

// ===== DebugHudScene =====
void DebugHudScene::onEnter()
{
    setupUI(View::ClearFlags::Depth);

    const math::vec2 size = getScreenSize();
    m_fpsText = addText(math::vec2(16.0f, size.y), math::vec2(0.0f, 0.0f), "", 16);
}

// ===== TestBaseScene =====
TestBaseScene::TestBaseScene()
{
    m_world = createSceneGraph();
    m_camera = m_world->getRoot()->createChild()->addComponent<Camera>();

    m_view = createView();
    m_view->setScene(m_world);
    m_view->setCamera(m_camera);
}

void DebugHudScene::onUpdate(float deltaTime)
{
    m_fpsText->setString(std::format("FPS: {:.1f}", getEngine().getFrameCounter().getFrameRate()));
}

void TestBaseScene::initializeScene()
{
    addChild<DebugHudScene>(DEBUG_HUD_LAYER);
}
