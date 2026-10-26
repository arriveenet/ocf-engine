// SPDX-License-Identifier: MIT
#pragma once

#include <ocf/math/vec2.h>
#include <ocf/scene/2d/Text.h>
#include <ocf/scene/Scene.h>
#include <ocf/scene/View.h>

#include <string_view>

class ocf::SceneGraph;
class ocf::Camera;
class ocf::View;

class UIScene : public ocf::Scene {
protected:
    void setupUI(ocf::View::ClearFlags clearFlags);

    ocf::Text* addText(const ocf::math::vec2& position, const ocf::math::vec2& anchor,
                       std::string_view string, int fontSize = 32);

    ocf::math::vec2 getScreenSize() const;

protected:
    ocf::SceneGraph* m_ui = nullptr;
};


class DebugHudScene : public UIScene {
public:
    void onEnter() override;
    void onUpdate(float deltaTime) override;

protected:
    ocf::Text* m_fpsText = nullptr;
};

class TestBaseScene : public ocf::Scene {
public:
    static constexpr int DEBUG_HUD_LAYER = 1000;

    TestBaseScene();
    virtual ~TestBaseScene() = default;

    void initializeScene();

protected:
    ocf::SceneGraph* m_world = nullptr;
    ocf::Camera* m_camera = nullptr;
    ocf::View* m_view = nullptr;
};
