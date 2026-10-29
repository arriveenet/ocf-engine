// SPDX-License-Identifier: MIT
#include "TileMapTest.h"

#include <ocf/core/Engine.h>
#include <ocf/core/Logger.h>
#include <ocf/resource/TextureManager.h>
#include <ocf/resource/TiledMapLoader.h>
#include <ocf/scene/Camera.h>
#include <ocf/scene/Node.h>
#include <ocf/scene/SceneGraph.h>
#include <ocf/scene/View.h>

#include <imgui.h>

using namespace ocf;

namespace {

const char* getShapeName(MapObject::Shape shape)
{
    switch (shape) {
    case MapObject::Shape::Rectangle:
        return "rectangle";
    case MapObject::Shape::Ellipse:
        return "ellipse";
    case MapObject::Shape::Point:
        return "point";
    case MapObject::Shape::Polygon:
        return "polygon";
    case MapObject::Shape::Polyline:
        return "polyline";
    case MapObject::Shape::Text:
        return "text";
    }
    return "";
}

} // namespace

void TileMapTest::onEnter()
{
    initializeScene();

    const math::ivec2 size = getEngine().getWindowSize();
    const float width = static_cast<float>(size.x);
    const float height = static_cast<float>(size.y);
    m_camera->orthographic(0.0f, width, 0.0f, height);

    // Layers are drawn in the order they are submitted
    m_view->setSortMode(View::SortMode::SubmitOrder);

    m_jsonMap = loadMap("tilemaps/sample.tmj");
    m_xmlMap = loadMap("tilemaps/sample.tmx");
    m_codeMap = buildMap();

    const float top = height - 40.0f;
    addTileMap(m_jsonMap, math::vec2(8.0f, top));
    addTileMap(m_xmlMap, math::vec2(width * 0.5f + 8.0f, top));
    addTileMap(m_codeMap, math::vec2(8.0f, top - 280.0f));
}

void TileMapTest::onExit()
{
}

void TileMapTest::onImGui()
{
    ImGui::Begin("TileMapTest");

    if (ImGui::SliderFloat("Scale", &m_scale, 0.5f, 4.0f)) {
        for (TileMapRenderer* renderer : m_renderers) {
            renderer->setPixelsPerUnit(1.0f / m_scale);
        }
    }

    if (ImGui::Checkbox("Linear Filter", &m_linearFilter)) {
        for (TileMapRenderer* renderer : m_renderers) {
            renderer->setTextureFilter(m_linearFilter ? TextureFilter::Linear
                                                      : TextureFilter::Nearest);
        }
    }

    if (ImGui::Button("Change a Tile")) {
        changeTile();
    }

    // Editing the shared map is picked up by the renderer in its next update
    if (m_jsonMap.ptr() != nullptr && ImGui::CollapsingHeader("Layers (sample.tmj)")) {
        for (const auto& layer : m_jsonMap->getLayers()) {
            ImGui::PushID(layer.get());

            bool visible = layer->isVisible();
            if (ImGui::Checkbox(layer->getName().c_str(), &visible)) {
                layer->setVisible(visible);
            }

            if (layer->getType() == TileMapLayer::Type::Tile) {
                float opacity = layer->getOpacity();
                ImGui::SameLine();
                ImGui::SetNextItemWidth(120.0f);
                if (ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f)) {
                    layer->setOpacity(opacity);
                }
            }

            ImGui::PopID();
        }
    }

    if (m_jsonMap.ptr() != nullptr && ImGui::CollapsingHeader("Objects (sample.tmj)")) {
        if (const ObjectLayer* objects = m_jsonMap->findObjectLayer("objects")) {
            for (const MapObject& object : objects->getObjects()) {
                ImGui::Text("%s (%s): (%.0f, %.0f) %.0fx%.0f", object.name.c_str(),
                            getShapeName(object.shape), object.position.x, object.position.y,
                            object.size.x, object.size.y);
            }
        }
    }

    ImGui::End();
}

Ref<TileMap> TileMapTest::loadMap(std::string_view path)
{
    auto result = TiledMapLoader::load(getEngine(), path);
    if (result.isErr()) {
        OCF_LOG_ERROR("{}", result.unwrapErr());
        return Ref<TileMap>();
    }
    return result.unwrap();
}

Ref<TileMap> TileMapTest::buildMap()
{
    Texture* texture = getEngine().getTextureManager().addImage("tilemaps/terrain.png");

    Ref<TileMap> map(new TileMap(8, 3, 16, 16));
    m_firstGid = map->addTileSet(Ref<TileSet>(new TileSet(texture, 16, 16)));

    TileLayer* ground = map->addTileLayer("ground");
    ground->fill(m_firstGid + 0); // Grass
    for (uint32_t x = 0; x < map->getWidth(); x++) {
        ground->setTile(x, 2, m_firstGid + 1); // Dirt
    }

    // The letter F in every combination of the flip flags
    TileLayer* letters = map->addTileLayer("letters");
    for (uint32_t i = 0; i < 8; i++) {
        letters->setTile(i, 1,
                         makeTileGid(m_firstGid + 4, (i & 1) != 0, (i & 2) != 0, (i & 4) != 0));
    }
    return map;
}

void TileMapTest::addTileMap(const Ref<TileMap>& map, const math::vec2& topLeft)
{
    if (map.ptr() == nullptr) {
        return;
    }

    Node* node = m_world->getRoot()->createChild();
    node->setPosition(math::vec3(topLeft.x, topLeft.y, 0.0f));

    TileMapRenderer* renderer = node->addComponent<TileMapRenderer>(getEngine(), map);
    renderer->setPixelsPerUnit(1.0f / m_scale);
    renderer->setAnchor(math::vec2(0.0f, 0.0f));
    m_renderers.push_back(renderer);
}

void TileMapTest::changeTile()
{
    // Cycle the top row of the code map through the first four tiles, one cell at a time
    TileLayer* ground = m_codeMap->findTileLayer("ground");
    const uint32_t x = m_changeCount % ground->getWidth();
    const uint32_t tile = (m_changeCount / ground->getWidth() + 1) % 4;
    ground->setTile(x, 0, m_firstGid + tile);
    m_changeCount++;
}
