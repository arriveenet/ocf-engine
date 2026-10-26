// SPDX-License-Identifier: MIT
#include "MainScene.h"

#include <ocf/core/Engine.h>
#include <ocf/platform/Application.h>
#include <ocf/platform/FileSystem.h>
#include <ocf/scene/SceneManager.h>

#include <imgui.h>

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#ifdef _WIN32
#include <crtdbg.h>
#endif

#include <string>

using namespace ocf;



int main()
{
#ifdef _WIN32
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    Application::Config config;
    config.title = "SandBox Test";

    Application& app = Application::getInstance();
    app.run(
        config,
        [](Engine& engine) {
            // The built-in ImGui font has no Japanese glyphs
            engine.configureImGui([](ImGuiIO& io) {
                const std::string fontPath =
                    FileSystem::getInstance()->getAssetFullPath("fonts/NotoSansJP-Regular.ttf");
                if (!fontPath.empty()) {
                    io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f);
                }
            });
            engine.getSceneManager().change<MainScene>();
        },
        nullptr);
}
