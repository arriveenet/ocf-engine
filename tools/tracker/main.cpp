// SPDX-License-Identifier: MIT
#include "TrackerScene.h"

#include <ocf/core/Engine.h>
#include <ocf/platform/Application.h>
#include <ocf/platform/FileSystem.h>
#include <ocf/scene/SceneManager.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <string>

using namespace ocf;

int main()
{
    Application::Config config;
    config.title = "OCF Tracker";

    Application& app = Application::getInstance();
    app.run(
        config,
        [](Engine& engine) {
            ImFont* monospaceFont = nullptr;
            engine.configureImGui([&monospaceFont](ImGuiIO& io) {
                // The UI is in English, but song titles and instrument names may be in
                // Japanese, which the built-in ImGui font has no glyphs for
                const std::string fontPath =
                    FileSystem::getInstance()->getAssetFullPath("fonts/NotoSansJP-Regular.ttf");
                if (!fontPath.empty()) {
                    io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f);
                }
                // Monospaced, for the pattern editor
                monospaceFont = io.Fonts->AddFontDefaultVector();

                // Tab moves the pattern cursor between channels, not the keyboard focus
                ImGui::GetCurrentContext()->ConfigNavEnableTabbing = false;
            });
            engine.getSceneManager().change<tracker_tool::TrackerScene>(monospaceFont);
        },
        nullptr, 1280, 800);
}
