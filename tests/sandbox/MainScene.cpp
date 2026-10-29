// SPDX-License-Identifier: MIT
#include "MainScene.h"

#include "AudioSystemTest.h"
#include "MeshInstanceTest.h"
#include "RenderTargetTest.h"
#include "SpriteTest.h"
#include "TileMapTest.h"

#include <ocf/scene/SceneManager.h>

#include <imgui.h>

using namespace ocf;

void MainScene::onEnter()
{
    initializeScene();
}

void MainScene::onImGui()
{
    ImGui::Begin("MainScene");

    if (ImGui::Button("AudioSystem Test")) {
        getSceneManager().change<AudioSystemTest>();
    }

    if (ImGui::Button("MeshInstance Test")) {
        getSceneManager().change<MeshInstanceTest>();
    }

    if (ImGui::Button("Sprite Test")) {
        getSceneManager().change<SpriteTest>();
    }

    if (ImGui::Button("RenderTarget Test")) {
        getSceneManager().change<RenderTargetTest>();
    }
  
    if (ImGui::Button("TileMap Test")) {
        getSceneManager().change<TileMapTest>();
    }

    ImGui::End();
}
