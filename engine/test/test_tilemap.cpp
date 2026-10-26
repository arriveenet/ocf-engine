// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include <ocf/core/Reference.h>
#include <ocf/resource/TileMap.h>

using namespace ocf;

TEST(TileGidTest, FlagsRoundTrip)
{
    const TileGid tile = makeTileGid(42, true, false, true);

    EXPECT_EQ(getTileGid(tile), 42u);
    EXPECT_TRUE(isTileFlippedX(tile));
    EXPECT_FALSE(isTileFlippedY(tile));
    EXPECT_TRUE(isTileFlippedDiagonally(tile));
    EXPECT_EQ(tile, 42u | TILE_FLIPPED_HORIZONTALLY | TILE_FLIPPED_DIAGONALLY);
}

TEST(TileGidTest, MakeDropsFlagBitsOfTheId)
{
    EXPECT_EQ(makeTileGid(TILE_FLIPPED_VERTICALLY | 7), 7u);
}

TEST(TileSetTest, GridFromImage)
{
    // 2px margin, 1px spacing: 2 + 16 + 1 + 16 + 1 + 16 + 2 = 54
    TileSet tileSet(16, 16, 2, 1);
    tileSet.setImage("tiles.png", 54, 37);

    EXPECT_EQ(tileSet.getColumns(), 3u);
    EXPECT_EQ(tileSet.getTileCount(), 6u);
}

TEST(TileSetTest, PartialTilesAreNotCounted)
{
    TileSet tileSet(16, 16);
    tileSet.setImage("tiles.png", 40, 15);

    EXPECT_EQ(tileSet.getColumns(), 2u);
    EXPECT_EQ(tileSet.getTileCount(), 0u);
}

TEST(TileSetTest, TextureRectWithMarginAndSpacing)
{
    TileSet tileSet(16, 8, 2, 1);
    tileSet.setImage("tiles.png", 54, 30);

    const math::Rect first = tileSet.getTextureRect(0);
    EXPECT_FLOAT_EQ(first.getMinX(), 2.0f);
    EXPECT_FLOAT_EQ(first.getMinY(), 2.0f);
    EXPECT_FLOAT_EQ(first.m_size.x, 16.0f);
    EXPECT_FLOAT_EQ(first.m_size.y, 8.0f);

    // Second row, third column
    const math::Rect rect = tileSet.getTextureRect(5);
    EXPECT_FLOAT_EQ(rect.getMinX(), 2.0f + 2.0f * 17.0f);
    EXPECT_FLOAT_EQ(rect.getMinY(), 2.0f + 9.0f);
}

TEST(TileSetTest, TextureRectOutOfRangeIsEmpty)
{
    TileSet tileSet(16, 16);
    tileSet.setImage("tiles.png", 32, 32);

    const math::Rect rect = tileSet.getTextureRect(4);
    EXPECT_FLOAT_EQ(rect.m_size.x, 0.0f);
    EXPECT_FLOAT_EQ(rect.m_size.y, 0.0f);
}

TEST(TileSetTest, TileProperties)
{
    TileSet tileSet(16, 16);
    EXPECT_EQ(tileSet.getTileProperties(3), nullptr);

    tileSet.setTileProperties(3, {{"solid", true}});
    const TileProperties* properties = tileSet.getTileProperties(3);
    ASSERT_NE(properties, nullptr);
    EXPECT_EQ(std::get<bool>(properties->at("solid")), true);
}

namespace {

Ref<TileSet> makeTileSet(uint32_t columns, uint32_t rows)
{
    Ref<TileSet> tileSet(new TileSet(16, 16));
    tileSet->setImage("tiles.png", columns * 16, rows * 16);
    return tileSet;
}

} // namespace

TEST(TileMapTest, AddTileSetAssignsConsecutiveRanges)
{
    TileMap map(4, 3, 16, 16);

    EXPECT_EQ(map.addTileSet(makeTileSet(2, 2)), 1u);
    EXPECT_EQ(map.addTileSet(makeTileSet(3, 1)), 5u);
    EXPECT_EQ(map.addTileSet(makeTileSet(1, 1)), 8u);
}

TEST(TileMapTest, AddTileSetAtGidRejectsOverlap)
{
    TileMap map(4, 3, 16, 16);

    EXPECT_FALSE(map.addTileSet(makeTileSet(1, 1), 0));
    EXPECT_TRUE(map.addTileSet(makeTileSet(2, 2), 10)); // 10..13
    EXPECT_FALSE(map.addTileSet(makeTileSet(1, 1), 13));
    EXPECT_FALSE(map.addTileSet(makeTileSet(2, 1), 9)); // 9..10
    EXPECT_TRUE(map.addTileSet(makeTileSet(3, 3), 1));  // 1..9, before the first one
    EXPECT_TRUE(map.addTileSet(makeTileSet(1, 1), 14));

    ASSERT_EQ(map.getTileSets().size(), 3u);
    EXPECT_EQ(map.getTileSets()[0].firstGid, 1u);
    EXPECT_EQ(map.getTileSets()[1].firstGid, 10u);
    EXPECT_EQ(map.getTileSets()[2].firstGid, 14u);
}

TEST(TileMapTest, FindTileSet)
{
    TileMap map(4, 3, 16, 16);
    Ref<TileSet> first = makeTileSet(2, 2);
    Ref<TileSet> second = makeTileSet(1, 1);
    map.addTileSet(first, 1);  // 1..4
    map.addTileSet(second, 8); // 8 only

    EXPECT_EQ(map.findTileSet(0), nullptr);
    ASSERT_NE(map.findTileSet(1), nullptr);
    EXPECT_EQ(map.findTileSet(1)->tileSet, first);
    EXPECT_EQ(map.findTileSet(4)->tileSet, first);
    EXPECT_EQ(map.findTileSet(5), nullptr); // In the gap
    EXPECT_EQ(map.findTileSet(8)->tileSet, second);
    EXPECT_EQ(map.findTileSet(makeTileGid(8, true, true, true))->tileSet, second);
    EXPECT_EQ(map.findTileSet(9), nullptr);
}

TEST(TileMapTest, TileLayerAccess)
{
    TileMap map(4, 3, 16, 16);
    TileLayer* layer = map.addTileLayer("ground");

    EXPECT_EQ(layer->getWidth(), 4u);
    EXPECT_EQ(layer->getHeight(), 3u);
    EXPECT_EQ(layer->getTiles().size(), 12u);

    layer->fill(1);
    layer->setTile(3, 2, makeTileGid(2, true));
    EXPECT_EQ(layer->getTile(0, 0), 1u);
    EXPECT_EQ(layer->getTile(3, 2), makeTileGid(2, true));
    EXPECT_EQ(layer->getTiles()[11], makeTileGid(2, true));

    // Outside the layer
    layer->setTile(4, 0, 5);
    EXPECT_EQ(layer->getTile(4, 0), 0u);

    EXPECT_FALSE(layer->setTiles(std::vector<TileGid>(11, 1)));
    EXPECT_TRUE(layer->setTiles(std::vector<TileGid>(12, 3)));
    EXPECT_EQ(layer->getTile(2, 1), 3u);
}

TEST(TileMapTest, RevisionAdvancesOnDrawingChanges)
{
    TileMap map(4, 3, 16, 16);
    TileLayer* layer = map.addTileLayer("ground");

    uint32_t revision = map.getRevision();
    layer->setTile(1, 1, 2);
    EXPECT_GT(map.getRevision(), revision);

    revision = map.getRevision();
    layer->setTile(1, 1, 2); // Unchanged
    EXPECT_EQ(map.getRevision(), revision);

    layer->setVisible(false);
    EXPECT_GT(map.getRevision(), revision);

    revision = map.getRevision();
    layer->setOpacity(0.5f);
    EXPECT_GT(map.getRevision(), revision);

    revision = map.getRevision();
    map.addTileSet(makeTileSet(1, 1));
    EXPECT_GT(map.getRevision(), revision);
}

TEST(TileMapTest, FindLayers)
{
    TileMap map(4, 3, 16, 16);
    TileLayer* ground = map.addTileLayer("ground");
    ObjectLayer* objects = map.addObjectLayer("objects");

    EXPECT_EQ(map.findLayer("ground"), ground);
    EXPECT_EQ(map.findTileLayer("ground"), ground);
    EXPECT_EQ(map.findTileLayer("objects"), nullptr);
    EXPECT_EQ(map.findObjectLayer("objects"), objects);
    EXPECT_EQ(map.findLayer("missing"), nullptr);
    ASSERT_EQ(map.getLayers().size(), 2u);
    EXPECT_EQ(map.getLayers()[1].get(), objects);
}

TEST(TileMapTest, ObjectLayer)
{
    TileMap map(4, 3, 16, 16);
    ObjectLayer* layer = map.addObjectLayer("objects");

    MapObject spawn;
    spawn.name = "spawn";
    spawn.shape = MapObject::Shape::Point;
    spawn.position = math::vec2(24.0f, 8.0f);
    layer->addObject(spawn);

    const MapObject* found = layer->findObject("spawn");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->shape, MapObject::Shape::Point);
    EXPECT_FLOAT_EQ(found->position.x, 24.0f);
    EXPECT_EQ(layer->findObject("missing"), nullptr);
}
