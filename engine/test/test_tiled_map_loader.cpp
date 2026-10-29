// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include <ocf/resource/TiledMapLoader.h>

#include <format>
#include <fstream>
#include <iterator>
#include <map>
#include <string>

using namespace ocf;

namespace {

// The same map in both formats. Each refers to the external tile set in the other format.
// Tiles of "deco" (base64 with zlib): 0, 5, flipX(3), 0, flipY|diagonal(6), 0

constexpr const char* SAMPLE_JSON = R"({
  "type": "map", "version": "1.10", "orientation": "orthogonal", "renderorder": "right-down",
  "width": 3, "height": 2, "tilewidth": 16, "tileheight": 16, "infinite": false,
  "backgroundcolor": "#80112233",
  "properties": [
    { "name": "title", "type": "string", "value": "Level 1" },
    { "name": "hard", "type": "bool", "value": true },
    { "name": "lives", "type": "int", "value": 3 },
    { "name": "gravity", "type": "float", "value": 9.5 },
    { "name": "tint", "type": "color", "value": "#ff00ff00" },
    { "name": "music", "type": "file", "value": "../audio/theme.mp3" }
  ],
  "tilesets": [
    { "firstgid": 1, "name": "terrain", "tilewidth": 16, "tileheight": 16, "tilecount": 8,
      "columns": 4, "margin": 0, "spacing": 0, "tileoffset": { "x": 1, "y": 2 },
      "image": "../images/terrain.png", "imagewidth": 64, "imageheight": 32,
      "tiles": [ { "id": 1, "properties": [ { "name": "solid", "type": "bool", "value": true } ] } ] },
    { "firstgid": 9, "source": "tiles/props.tsx" }
  ],
  "layers": [
    { "id": 1, "name": "ground", "type": "tilelayer", "width": 3, "height": 2,
      "data": [1, 2, 9, 4, 5, 2147483650], "opacity": 1, "visible": true, "x": 0, "y": 0 },
    { "id": 2, "name": "decor", "type": "group", "offsetx": 4, "offsety": 8, "opacity": 0.5,
      "layers": [
        { "id": 3, "name": "deco", "type": "tilelayer", "width": 3, "height": 2, "opacity": 0.5,
          "encoding": "base64", "compression": "zlib",
          "data": "eJxjYGBgYAViZgaGBiDFwMbAkACiAQk8AO8=" },
        { "id": 4, "name": "hidden", "type": "tilelayer", "width": 3, "height": 2,
          "visible": false, "data": [0, 0, 0, 0, 0, 0] }
      ] },
    { "id": 6, "name": "background", "type": "imagelayer", "image": "bg.png" },
    { "id": 5, "name": "objects", "type": "objectgroup", "objects": [
        { "id": 1, "name": "spawn", "x": 24, "y": 8, "point": true },
        { "id": 2, "name": "area", "type": "trigger", "x": 0, "y": 0, "width": 32, "height": 16,
          "properties": [ { "name": "damage", "type": "int", "value": 3 } ] },
        { "id": 3, "name": "pond", "x": 8, "y": 8, "width": 10, "height": 6, "ellipse": true },
        { "id": 4, "name": "path", "x": 0, "y": 16,
          "polyline": [ { "x": 0, "y": 0 }, { "x": 16, "y": 4.5 } ] },
        { "id": 5, "name": "sign", "x": 32, "y": 0, "width": 16, "height": 16,
          "text": { "text": "Hello", "wrap": true } },
        { "id": 6, "name": "crate", "gid": 9, "x": 16, "y": 32, "width": 16, "height": 32 }
      ] }
  ]
})";

constexpr const char* SAMPLE_XML = R"(<?xml version="1.0" encoding="UTF-8"?>
<map version="1.10" orientation="orthogonal" renderorder="right-down" width="3" height="2"
     tilewidth="16" tileheight="16" infinite="0" backgroundcolor="#80112233">
 <properties>
  <property name="title" value="Level 1"/>
  <property name="hard" type="bool" value="true"/>
  <property name="lives" type="int" value="3"/>
  <property name="gravity" type="float" value="9.5"/>
  <property name="tint" type="color" value="#ff00ff00"/>
  <property name="music" type="file" value="../audio/theme.mp3"/>
 </properties>
 <tileset firstgid="1" name="terrain" tilewidth="16" tileheight="16" tilecount="8" columns="4">
  <tileoffset x="1" y="2"/>
  <image source="../images/terrain.png" width="64" height="32"/>
  <tile id="1">
   <properties><property name="solid" type="bool" value="true"/></properties>
  </tile>
 </tileset>
 <tileset firstgid="9" source="tiles/props.tsj"/>
 <layer id="1" name="ground" width="3" height="2">
  <data encoding="csv">
1,2,9,
4,5,2147483650
</data>
 </layer>
 <group id="2" name="decor" offsetx="4" offsety="8" opacity="0.5">
  <layer id="3" name="deco" width="3" height="2" opacity="0.5">
   <data encoding="base64" compression="zlib">
    eJxjYGBgYAViZgaGBiDFwMbAkACiAQk8AO8=
   </data>
  </layer>
  <layer id="4" name="hidden" width="3" height="2" visible="0">
   <data encoding="csv">0,0,0,0,0,0</data>
  </layer>
 </group>
 <imagelayer id="6" name="background"><image source="bg.png"/></imagelayer>
 <objectgroup id="5" name="objects">
  <object id="1" name="spawn" x="24" y="8"><point/></object>
  <object id="2" name="area" class="trigger" x="0" y="0" width="32" height="16">
   <properties><property name="damage" type="int" value="3"/></properties>
  </object>
  <object id="3" name="pond" x="8" y="8" width="10" height="6"><ellipse/></object>
  <object id="4" name="path" x="0" y="16"><polyline points="0,0 16,4.5"/></object>
  <object id="5" name="sign" x="32" y="0" width="16" height="16"><text wrap="1">Hello</text></object>
  <object id="6" name="crate" gid="9" x="16" y="32" width="16" height="32"/>
 </objectgroup>
</map>
)";

constexpr const char* PROPS_TSJ = R"({
  "type": "tileset", "name": "props", "tilewidth": 16, "tileheight": 32, "tilecount": 2,
  "columns": 2, "image": "props.png", "imagewidth": 32, "imageheight": 32
})";

constexpr const char* PROPS_TSX = R"(<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.10" name="props" tilewidth="16" tileheight="32" tilecount="2" columns="2">
 <image source="props.png" width="32" height="32"/>
</tileset>
)";

/** Serves files from memory, recording the paths that were asked for. */
class FakeFiles {
public:
    FakeFiles()
    {
        m_files["maps/level1/tiles/props.tsj"] = PROPS_TSJ;
        m_files["maps/level1/tiles/props.tsx"] = PROPS_TSX;
    }

    TiledMapLoader::FileReader reader()
    {
        return [this](const std::filesystem::path& path) -> std::optional<std::string> {
            auto iter = m_files.find(path.generic_string());
            if (iter == m_files.end()) {
                return std::nullopt;
            }
            return iter->second;
        };
    }

private:
    std::map<std::string, std::string> m_files;
};

Ref<TileMap> loadSample(TiledMapLoader::Format format)
{
    FakeFiles files;
    auto result = TiledMapLoader::loadFromString(
        format == TiledMapLoader::Format::Json ? SAMPLE_JSON : SAMPLE_XML, format, "maps/level1",
        files.reader());
    EXPECT_TRUE(result.isOk()) << (result.isErr() ? result.unwrapErr() : "");
    return result.isOk() ? result.unwrap() : Ref<TileMap>();
}

std::string loadError(std::string_view text, TiledMapLoader::Format format)
{
    FakeFiles files;
    auto result = TiledMapLoader::loadFromString(text, format, "maps/level1", files.reader());
    EXPECT_TRUE(result.isErr());
    return result.isErr() ? result.unwrapErr() : std::string();
}

class TiledMapLoaderTest : public ::testing::TestWithParam<TiledMapLoader::Format> {};

} // namespace

INSTANTIATE_TEST_SUITE_P(Formats, TiledMapLoaderTest,
                         ::testing::Values(TiledMapLoader::Format::Json,
                                           TiledMapLoader::Format::Xml),
                         [](const auto& info) {
                             return info.param == TiledMapLoader::Format::Json ? "Json" : "Xml";
                         });

TEST_P(TiledMapLoaderTest, MapAttributes)
{
    Ref<TileMap> map = loadSample(GetParam());
    ASSERT_NE(map.ptr(), nullptr);

    EXPECT_EQ(map->getWidth(), 3u);
    EXPECT_EQ(map->getHeight(), 2u);
    EXPECT_EQ(map->getTileWidth(), 16u);
    EXPECT_EQ(map->getTileHeight(), 16u);

    const math::vec4& color = map->getBackgroundColor();
    EXPECT_FLOAT_EQ(color.x, 0x11 / 255.0f);
    EXPECT_FLOAT_EQ(color.y, 0x22 / 255.0f);
    EXPECT_FLOAT_EQ(color.z, 0x33 / 255.0f);
    EXPECT_FLOAT_EQ(color.w, 0x80 / 255.0f);
}

TEST_P(TiledMapLoaderTest, Properties)
{
    Ref<TileMap> map = loadSample(GetParam());
    ASSERT_NE(map.ptr(), nullptr);

    const TileProperties& properties = map->getProperties();
    ASSERT_EQ(properties.size(), 6u);
    EXPECT_EQ(std::get<std::string>(properties.at("title")), "Level 1");
    EXPECT_EQ(std::get<bool>(properties.at("hard")), true);
    EXPECT_EQ(std::get<int>(properties.at("lives")), 3);
    EXPECT_FLOAT_EQ(std::get<float>(properties.at("gravity")), 9.5f);
    EXPECT_EQ(std::get<std::string>(properties.at("tint")), "#ff00ff00");
    EXPECT_EQ(std::get<std::string>(properties.at("music")), "maps/audio/theme.mp3");
}

TEST_P(TiledMapLoaderTest, TileSets)
{
    Ref<TileMap> map = loadSample(GetParam());
    ASSERT_NE(map.ptr(), nullptr);

    const auto& tileSets = map->getTileSets();
    ASSERT_EQ(tileSets.size(), 2u);

    const TileSet& terrain = *tileSets[0].tileSet.ptr();
    EXPECT_EQ(tileSets[0].firstGid, 1u);
    EXPECT_EQ(terrain.getName(), "terrain");
    EXPECT_EQ(terrain.getImagePath(), "maps/images/terrain.png");
    EXPECT_EQ(terrain.getImageWidth(), 64u);
    EXPECT_EQ(terrain.getColumns(), 4u);
    EXPECT_EQ(terrain.getTileCount(), 8u);
    EXPECT_FLOAT_EQ(terrain.getTileOffset().x, 1.0f);
    EXPECT_FLOAT_EQ(terrain.getTileOffset().y, 2.0f);
    EXPECT_EQ(terrain.getTexture(), nullptr);
    ASSERT_NE(terrain.getTileProperties(1), nullptr);
    EXPECT_EQ(std::get<bool>(terrain.getTileProperties(1)->at("solid")), true);
    EXPECT_EQ(terrain.getTileProperties(0), nullptr);

    // External, resolved against its own directory
    const TileSet& props = *tileSets[1].tileSet.ptr();
    EXPECT_EQ(tileSets[1].firstGid, 9u);
    EXPECT_EQ(props.getName(), "props");
    EXPECT_EQ(props.getTileHeight(), 32u);
    EXPECT_EQ(props.getImagePath(), "maps/level1/tiles/props.png");
    EXPECT_EQ(props.getTileCount(), 2u);
}

TEST_P(TiledMapLoaderTest, TileLayers)
{
    Ref<TileMap> map = loadSample(GetParam());
    ASSERT_NE(map.ptr(), nullptr);

    // The group is flattened and the image layer skipped
    const auto& layers = map->getLayers();
    ASSERT_EQ(layers.size(), 4u);
    EXPECT_EQ(layers[0]->getName(), "ground");
    EXPECT_EQ(layers[1]->getName(), "deco");
    EXPECT_EQ(layers[2]->getName(), "hidden");
    EXPECT_EQ(layers[3]->getName(), "objects");

    const TileLayer* ground = map->findTileLayer("ground");
    ASSERT_NE(ground, nullptr);
    EXPECT_EQ(ground->getId(), 1u);
    EXPECT_EQ(ground->getTiles(),
              (std::vector<TileGid>{1, 2, 9, 4, 5, makeTileGid(2, true, false, false)}));

    const TileLayer* deco = map->findTileLayer("deco");
    ASSERT_NE(deco, nullptr);
    EXPECT_EQ(deco->getTiles(), (std::vector<TileGid>{0, 5, makeTileGid(3, true), 0,
                                                      makeTileGid(6, false, true, true), 0}));
    EXPECT_FLOAT_EQ(deco->getOpacity(), 0.25f);
    EXPECT_FLOAT_EQ(deco->getOffset().x, 4.0f);
    EXPECT_FLOAT_EQ(deco->getOffset().y, 8.0f);
    EXPECT_TRUE(deco->isVisible());

    EXPECT_FALSE(map->findTileLayer("hidden")->isVisible());
}

TEST_P(TiledMapLoaderTest, Objects)
{
    Ref<TileMap> map = loadSample(GetParam());
    ASSERT_NE(map.ptr(), nullptr);

    const ObjectLayer* layer = map->findObjectLayer("objects");
    ASSERT_NE(layer, nullptr);
    EXPECT_EQ(layer->getId(), 5u);
    ASSERT_EQ(layer->getObjects().size(), 6u);

    const MapObject* spawn = layer->findObject("spawn");
    ASSERT_NE(spawn, nullptr);
    EXPECT_EQ(spawn->shape, MapObject::Shape::Point);
    EXPECT_FLOAT_EQ(spawn->position.x, 24.0f);
    EXPECT_FLOAT_EQ(spawn->position.y, 8.0f);

    const MapObject* area = layer->findObject("area");
    EXPECT_EQ(area->shape, MapObject::Shape::Rectangle);
    EXPECT_EQ(area->className, "trigger");
    EXPECT_FLOAT_EQ(area->size.x, 32.0f);
    EXPECT_EQ(std::get<int>(area->properties.at("damage")), 3);

    EXPECT_EQ(layer->findObject("pond")->shape, MapObject::Shape::Ellipse);

    const MapObject* path = layer->findObject("path");
    EXPECT_EQ(path->shape, MapObject::Shape::Polyline);
    ASSERT_EQ(path->points.size(), 2u);
    EXPECT_FLOAT_EQ(path->points[1].x, 16.0f);
    EXPECT_FLOAT_EQ(path->points[1].y, 4.5f);

    const MapObject* sign = layer->findObject("sign");
    EXPECT_EQ(sign->shape, MapObject::Shape::Text);
    EXPECT_EQ(sign->text, "Hello");

    const MapObject* crate = layer->findObject("crate");
    EXPECT_EQ(crate->gid, 9u);
    EXPECT_EQ(crate->id, 6u);
}

TEST(TiledMapLoaderJsonTest, InvalidJsonReportsPosition)
{
    const std::string error = loadError("{\n  \"width\": }", TiledMapLoader::Format::Json);
    EXPECT_NE(error.find("line 2"), std::string::npos) << error;
}

TEST(TiledMapLoaderJsonTest, RequiredMember)
{
    const std::string error =
        loadError(R"({ "width": 3, "height": 2, "tilewidth": 16 })", TiledMapLoader::Format::Json);
    EXPECT_EQ(error, "/tileheight: is required");
}

TEST(TiledMapLoaderJsonTest, UnsupportedMaps)
{
    EXPECT_EQ(loadError(R"({ "orientation": "isometric", "width": 3, "height": 2,
                             "tilewidth": 16, "tileheight": 16 })",
                        TiledMapLoader::Format::Json),
              "/orientation: \"isometric\" maps are not supported");
    EXPECT_EQ(loadError(R"({ "infinite": true, "width": 3, "height": 2,
                             "tilewidth": 16, "tileheight": 16 })",
                        TiledMapLoader::Format::Json),
              "/infinite: infinite maps are not supported");
}

TEST(TiledMapLoaderJsonTest, WrongTileCount)
{
    const std::string error = loadError(
        R"({ "width": 2, "height": 1, "tilewidth": 16, "tileheight": 16,
             "layers": [ { "type": "tilelayer", "name": "a", "data": [0, 0, 0] } ] })",
        TiledMapLoader::Format::Json);
    EXPECT_EQ(error, "/layers/0/data: expected 2 tiles, got 3");
}

TEST(TiledMapLoaderJsonTest, TileWithoutTileSet)
{
    const std::string error = loadError(
        R"({ "width": 2, "height": 1, "tilewidth": 16, "tileheight": 16,
             "layers": [ { "type": "tilelayer", "name": "a", "data": [0, 4] } ] })",
        TiledMapLoader::Format::Json);
    EXPECT_EQ(error, "/layers/0/data: tile 4 at index 1 is in no tile set");
}

TEST(TiledMapLoaderJsonTest, MissingExternalTileSet)
{
    const std::string error = loadError(
        R"({ "width": 2, "height": 1, "tilewidth": 16, "tileheight": 16,
             "tilesets": [ { "firstgid": 1, "source": "missing.tsj" } ] })",
        TiledMapLoader::Format::Json);
    EXPECT_EQ(error, "/tilesets/0/source: cannot read tile set maps/level1/missing.tsj");
}

TEST(TiledMapLoaderJsonTest, ImageCollectionTileSet)
{
    const std::string error = loadError(
        R"({ "width": 2, "height": 1, "tilewidth": 16, "tileheight": 16,
             "tilesets": [ { "firstgid": 1, "tilewidth": 16, "tileheight": 16, "tiles": [] } ] })",
        TiledMapLoader::Format::Json);
    EXPECT_EQ(error, "/tilesets/0/image: tile sets without a single image are not supported");
}

TEST(TiledMapLoaderXmlTest, InvalidXml)
{
    const std::string error = loadError("<map width=\"3\"", TiledMapLoader::Format::Xml);
    EXPECT_EQ(error.rfind("invalid XML", 0), 0u) << error;
}

TEST(TiledMapLoaderXmlTest, WrongRoot)
{
    EXPECT_EQ(loadError("<tileset/>", TiledMapLoader::Format::Xml),
              "the root element must be <map>");
}

TEST(TiledMapLoaderXmlTest, InvalidNumber)
{
    EXPECT_EQ(loadError(R"(<map width="3x" height="2" tilewidth="16" tileheight="16"/>)",
                        TiledMapLoader::Format::Xml),
              "/map/@width: must be an unsigned 32-bit integer");
}

TEST(TiledMapLoaderXmlTest, TileElements)
{
    FakeFiles files;
    auto result = TiledMapLoader::loadFromString(
        R"(<map width="2" height="1" tilewidth="16" tileheight="16">
            <tileset firstgid="1" name="t" tilewidth="16" tileheight="16">
             <image source="t.png" width="32" height="16"/>
            </tileset>
            <layer name="a" width="2" height="1"><data><tile/><tile gid="2"/></data></layer>
           </map>)",
        TiledMapLoader::Format::Xml, "", files.reader());
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();
    EXPECT_EQ(result.unwrap()->findTileLayer("a")->getTiles(), (std::vector<TileGid>{0, 2}));
}

TEST(TiledMapLoaderXmlTest, InfiniteLayerChunks)
{
    EXPECT_EQ(loadError(R"(<map width="2" height="1" tilewidth="16" tileheight="16">
                            <layer name="a"><data encoding="csv"><chunk/></data></layer>
                           </map>)",
                        TiledMapLoader::Format::Xml),
              "/map/layer[0]/data: infinite maps are not supported");
}

TEST(TiledMapLoaderXmlTest, ObjectTemplatesAreRejected)
{
    EXPECT_EQ(loadError(R"(<map width="2" height="1" tilewidth="16" tileheight="16">
                            <objectgroup name="o"><object id="1" template="t.tx"/></objectgroup>
                           </map>)",
                        TiledMapLoader::Format::Xml),
              "/map/objectgroup[0]/object[0]/@template: object templates are not supported");
}

namespace {

/** Loads a 3x2 JSON map with one tile set of 8 tiles and one base64 layer. */
Result<Ref<TileMap>, std::string> loadBase64Layer(std::string_view compression,
                                                  std::string_view data)
{
    const std::string json = std::format(
        R"({{ "width": 3, "height": 2, "tilewidth": 16, "tileheight": 16,
              "tilesets": [ {{ "firstgid": 1, "tilewidth": 16, "tileheight": 16,
                               "image": "t.png", "imagewidth": 64, "imageheight": 32 }} ],
              "layers": [ {{ "type": "tilelayer", "name": "a", "encoding": "base64",
                             "compression": "{}", "data": "{}" }} ] }})",
        compression, data);
    return TiledMapLoader::loadFromString(json, TiledMapLoader::Format::Json, "",
                                          FakeFiles().reader());
}

const std::vector<TileGid> BASE64_TILES = {
    0, 5, makeTileGid(3, true), 0, makeTileGid(6, false, true, true), 0};

} // namespace

TEST(TiledTileDataTest, Base64Uncompressed)
{
    auto result = loadBase64Layer("", "AAAAAAUAAAADAACAAAAAAAYAAGAAAAAA");
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();
    EXPECT_EQ(result.unwrap()->findTileLayer("a")->getTiles(), BASE64_TILES);
}

TEST(TiledTileDataTest, Base64Gzip)
{
    // With and without a file name in the gzip header
    for (const char* data : {"H4sICAAAAAAC/2xheWVyLmJpbgBjYGBgYAViZgaGBiDFwMbAkACiARD1tF4YAAAA",
                             "H4sIAAAAAAAC/2NgYGBgBWJmBoYGIMXAxsCQAKIBEPW0XhgAAAA="}) {
        auto result = loadBase64Layer("gzip", data);
        ASSERT_TRUE(result.isOk()) << result.unwrapErr();
        EXPECT_EQ(result.unwrap()->findTileLayer("a")->getTiles(), BASE64_TILES);
    }
}

TEST(TiledTileDataTest, Base64Errors)
{
    EXPECT_EQ(loadBase64Layer("zstd", "AAAA").unwrapErr(),
              "/layers/0/data: unsupported compression \"zstd\"");
    EXPECT_EQ(loadBase64Layer("", "AAAAAA==").unwrapErr(),
              "/layers/0/data: expected 6 tiles, got 4 bytes");
    EXPECT_EQ(loadBase64Layer("", "A*AA").unwrapErr(), "/layers/0/data: invalid base64");
    EXPECT_EQ(loadBase64Layer("zlib", "AAAA").unwrapErr(),
              "/layers/0/data: invalid or too long zlib data");
    EXPECT_EQ(loadBase64Layer("gzip", "AAAA").unwrapErr(), "/layers/0/data: invalid gzip header");
}

TEST(TiledTileDataTest, CsvErrors)
{
    const auto load = [](std::string_view csv) {
        const std::string xml = std::format(
            R"(<map width="3" height="1" tilewidth="16" tileheight="16">
                <layer name="a" width="3" height="1"><data encoding="csv">{}</data></layer>
               </map>)",
            csv);
        return TiledMapLoader::loadFromString(xml, TiledMapLoader::Format::Xml, "",
                                              FakeFiles().reader());
    };

    EXPECT_TRUE(load(" 0,\n0 ,0\n").isOk());
    EXPECT_EQ(load("0,x,0").unwrapErr(), "/map/layer[0]/data: invalid tile \"x\" at index 1");
    EXPECT_EQ(load("0,,0").unwrapErr(), "/map/layer[0]/data: invalid tile \"\" at index 1");
    EXPECT_EQ(load("0,0").unwrapErr(), "/map/layer[0]/data: expected 3 tiles, got 2");
}

TEST(TiledMapLoaderSampleTest, JsonAndXmlSamplesMatch)
{
    // The sample maps in assets/tilemaps, as Tiled Map Editor would save them
    const std::filesystem::path assets(OCF_TEST_ASSETS_DIR);
    const auto reader = [&assets](const std::filesystem::path& path) -> std::optional<std::string> {
        std::ifstream file(assets / path, std::ios::binary);
        if (!file) {
            return std::nullopt;
        }
        return std::string((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    };

    const auto load = [&reader](const char* name, TiledMapLoader::Format format) {
        auto result = TiledMapLoader::loadFromString(
            *reader(std::filesystem::path("tilemaps") / name), format, "tilemaps", reader);
        EXPECT_TRUE(result.isOk()) << name << ": " << (result.isErr() ? result.unwrapErr() : "");
        return result.isOk() ? result.unwrap() : Ref<TileMap>();
    };

    Ref<TileMap> json = load("sample.tmj", TiledMapLoader::Format::Json);
    Ref<TileMap> xml = load("sample.tmx", TiledMapLoader::Format::Xml);
    ASSERT_NE(json.ptr(), nullptr);
    ASSERT_NE(xml.ptr(), nullptr);

    ASSERT_EQ(json->getTileSets().size(), 2u);
    ASSERT_EQ(xml->getTileSets().size(), 2u);
    for (size_t i = 0; i < 2; i++) {
        const TileSet& a = *json->getTileSets()[i].tileSet.ptr();
        const TileSet& b = *xml->getTileSets()[i].tileSet.ptr();
        EXPECT_EQ(json->getTileSets()[i].firstGid, xml->getTileSets()[i].firstGid);
        EXPECT_EQ(a.getImagePath(), b.getImagePath());
        EXPECT_EQ(a.getTileCount(), b.getTileCount());
        EXPECT_EQ(a.getTileHeight(), b.getTileHeight());
    }
    EXPECT_EQ(json->getTileSets()[1].tileSet->getImagePath(), "tilemaps/props.png");

    ASSERT_EQ(json->getLayers().size(), xml->getLayers().size());
    for (size_t i = 0; i < json->getLayers().size(); i++) {
        const TileMapLayer& a = *json->getLayers()[i];
        const TileMapLayer& b = *xml->getLayers()[i];
        EXPECT_EQ(a.getName(), b.getName());
        EXPECT_FLOAT_EQ(a.getOpacity(), b.getOpacity());
        ASSERT_EQ(a.getType(), b.getType());
        if (a.getType() == TileMapLayer::Type::Tile) {
            EXPECT_EQ(static_cast<const TileLayer&>(a).getTiles(),
                      static_cast<const TileLayer&>(b).getTiles())
                << a.getName();
        }
    }

    const MapObject* spawn = xml->findObjectLayer("objects")->findObject("spawn");
    ASSERT_NE(spawn, nullptr);
    EXPECT_EQ(spawn->shape, MapObject::Shape::Point);
}
