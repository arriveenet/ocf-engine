// SPDX-License-Identifier: MIT
#pragma once

#include "TestBaseScene.h"

#include <ocf/core/Reference.h>
#include <ocf/math/vec2.h>
#include <ocf/resource/TileMap.h>
#include <ocf/scene/2d/TileMapRenderer.h>

#include <string_view>
#include <vector>

/**
 * The sample map loaded from Tiled Map Editor files in both formats (JSON left, XML right),
 * and a map built in code that shows the letter F in all eight tile orientations.
 */
class TileMapTest : public TestBaseScene {
public:
    void onEnter() override;
    void onExit() override;
    void onImGui() override;

private:
    ocf::Ref<ocf::TileMap> loadMap(std::string_view path);
    ocf::Ref<ocf::TileMap> buildMap();
    void addTileMap(const ocf::Ref<ocf::TileMap>& map, const ocf::math::vec2& topLeft);
    void changeTile();

    ocf::Ref<ocf::TileMap> m_jsonMap;
    ocf::Ref<ocf::TileMap> m_xmlMap;
    ocf::Ref<ocf::TileMap> m_codeMap;
    std::vector<ocf::TileMapRenderer*> m_renderers;

    ocf::TileGid m_firstGid = 0;
    uint32_t m_changeCount = 0;
    float m_scale = 2.0f; // Screen pixels per map pixel
    bool m_linearFilter = false;
};
