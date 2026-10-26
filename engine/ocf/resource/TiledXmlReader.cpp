// SPDX-License-Identifier: MIT
#include "ocf/resource/TiledCommon.h"

#include "ocf/core/Logger.h"

#include <pugixml.hpp>

#include <charconv>
#include <cstdint>
#include <cstring>
#include <format>
#include <limits>
#include <memory>

namespace ocf::tiled {

namespace {

Result<std::unique_ptr<pugi::xml_document>, std::string> parseDocument(std::string_view xml)
{
    using DocumentResult = Result<std::unique_ptr<pugi::xml_document>, std::string>;

    auto document = std::make_unique<pugi::xml_document>();
    const pugi::xml_parse_result result = document->load_buffer(xml.data(), xml.size());
    if (!result) {
        return DocumentResult::Err(
            std::format("invalid XML at offset {}: {}", result.offset, result.description()));
    }
    return DocumentResult::Ok(std::move(document));
}

std::string childPath(const std::string& path, const pugi::xml_node& node, size_t index)
{
    return std::format("{}/{}[{}]", path, node.name(), index);
}

std::string attributePath(const std::string& path, const char* name)
{
    return std::format("{}/@{}", path, name);
}

template <typename T>
bool parseNumber(std::string_view text, T& result)
{
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), result);
    return !text.empty() && error == std::errc() && end == text.data() + text.size();
}

/** Reads a map or tile set, keeping the first error. Every function returns false on error. */
class XmlReader {
public:
    /** @param context Used for external tile sets; nullptr when reading a tile set file. */
    XmlReader(const ParseContext* context, std::filesystem::path baseDirectory)
        : m_context(context)
        , m_baseDirectory(std::move(baseDirectory))
    {
    }

    Ref<TileMap> readMap(const pugi::xml_node& node);
    Ref<TileSet> readTileSet(const pugi::xml_node& node, const std::string& path);

    const std::string& getError() const { return m_error; }

private:
    bool readTileSetEntry(const pugi::xml_node& node, const std::string& path, TileMap& map);
    bool readLayers(const pugi::xml_node& parent, const std::string& path, TileMap& map,
                    const LayerGroup& group);
    bool readTileLayer(const pugi::xml_node& node, const std::string& path, TileMap& map,
                       const LayerGroup& group);
    bool readTileData(const pugi::xml_node& node, const std::string& path, size_t expectedCount,
                      std::vector<TileGid>& tiles);
    bool readObjectLayer(const pugi::xml_node& node, const std::string& path, TileMap& map,
                         const LayerGroup& group);
    bool readLayerAttributes(const pugi::xml_node& node, const std::string& path,
                             TileMapLayer& layer, const LayerGroup& group);
    bool readGroupAttributes(const pugi::xml_node& node, const std::string& path, bool& visible,
                             float& opacity, math::vec2& offset);
    bool readObject(const pugi::xml_node& node, const std::string& path, MapObject& object);
    bool readPoints(const pugi::xml_node& node, const std::string& path,
                    std::vector<math::vec2>& result);
    bool readProperties(const pugi::xml_node& node, const std::string& path,
                        TileProperties& properties);

    bool fail(const std::string& path, std::string_view message);

    // The attributes are optional unless `required`. An absent one leaves `result` unchanged.
    bool readUint(const pugi::xml_node& node, const char* name, const std::string& path,
                  uint32_t& result, bool required = false);
    bool readFloat(const pugi::xml_node& node, const char* name, const std::string& path,
                   float& result);
    bool readString(const pugi::xml_node& node, const char* name, const std::string& path,
                    std::string& result, bool required = false);
    bool readBool(const pugi::xml_node& node, const char* name, const std::string& path,
                  bool& result);

    const ParseContext* m_context;
    std::filesystem::path m_baseDirectory;
    std::string m_error;
};

bool XmlReader::fail(const std::string& path, std::string_view message)
{
    if (m_error.empty()) {
        m_error = path.empty() ? std::string(message) : std::format("{}: {}", path, message);
    }
    return false;
}

bool XmlReader::readUint(const pugi::xml_node& node, const char* name, const std::string& path,
                         uint32_t& result, bool required)
{
    const pugi::xml_attribute attribute = node.attribute(name);
    if (!attribute) {
        return required ? fail(attributePath(path, name), "is required") : true;
    }
    if (!parseNumber(attribute.value(), result)) {
        return fail(attributePath(path, name), "must be an unsigned 32-bit integer");
    }
    return true;
}

bool XmlReader::readFloat(const pugi::xml_node& node, const char* name, const std::string& path,
                          float& result)
{
    const pugi::xml_attribute attribute = node.attribute(name);
    if (!attribute) {
        return true;
    }
    if (!parseNumber(attribute.value(), result)) {
        return fail(attributePath(path, name), "must be a number");
    }
    return true;
}

bool XmlReader::readString(const pugi::xml_node& node, const char* name, const std::string& path,
                           std::string& result, bool required)
{
    const pugi::xml_attribute attribute = node.attribute(name);
    if (!attribute) {
        return required ? fail(attributePath(path, name), "is required") : true;
    }
    result = attribute.value();
    return true;
}

bool XmlReader::readBool(const pugi::xml_node& node, const char* name, const std::string& path,
                         bool& result)
{
    const pugi::xml_attribute attribute = node.attribute(name);
    if (!attribute) {
        return true;
    }
    // Attributes are written as 0 and 1, property values as false and true
    const std::string_view value = attribute.value();
    if (value == "1" || value == "true") {
        result = true;
    }
    else if (value == "0" || value == "false") {
        result = false;
    }
    else {
        return fail(attributePath(path, name), "must be 0, 1, false or true");
    }
    return true;
}

Ref<TileMap> XmlReader::readMap(const pugi::xml_node& node)
{
    const std::string path = "/map";
    if (!node) {
        fail("", "the root element must be <map>");
        return Ref<TileMap>();
    }

    std::string orientation = "orthogonal";
    bool infinite = false;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t tileWidth = 0;
    uint32_t tileHeight = 0;
    std::string backgroundColor;
    if (!readString(node, "orientation", path, orientation) ||
        !readBool(node, "infinite", path, infinite) ||
        !readUint(node, "width", path, width, true) ||
        !readUint(node, "height", path, height, true) ||
        !readUint(node, "tilewidth", path, tileWidth, true) ||
        !readUint(node, "tileheight", path, tileHeight, true) ||
        !readString(node, "backgroundcolor", path, backgroundColor)) {
        return Ref<TileMap>();
    }
    if (orientation != "orthogonal") {
        fail(attributePath(path, "orientation"),
             std::format("\"{}\" maps are not supported", orientation));
        return Ref<TileMap>();
    }
    if (infinite) {
        fail(attributePath(path, "infinite"), "infinite maps are not supported");
        return Ref<TileMap>();
    }

    Ref<TileMap> map(new TileMap(width, height, tileWidth, tileHeight));
    if (!backgroundColor.empty()) {
        const std::optional<math::vec4> color = parseColor(backgroundColor);
        if (!color) {
            fail(attributePath(path, "backgroundcolor"),
                 "must be a color like #RRGGBB or #AARRGGBB");
            return Ref<TileMap>();
        }
        map->setBackgroundColor(*color);
    }
    if (!readProperties(node, path, map->getProperties())) {
        return Ref<TileMap>();
    }

    size_t index = 0;
    for (const pugi::xml_node& tileSet : node.children("tileset")) {
        if (!readTileSetEntry(tileSet, childPath(path, tileSet, index++), *map.ptr())) {
            return Ref<TileMap>();
        }
    }

    if (!readLayers(node, path, *map.ptr(), LayerGroup())) {
        return Ref<TileMap>();
    }
    return map;
}

bool XmlReader::readTileSetEntry(const pugi::xml_node& node, const std::string& path, TileMap& map)
{
    uint32_t firstGid = 0;
    std::string source;
    if (!readUint(node, "firstgid", path, firstGid, true) ||
        !readString(node, "source", path, source)) {
        return false;
    }

    Ref<TileSet> tileSet;
    if (!source.empty()) {
        Result<Ref<TileSet>, std::string> result = readExternalTileSet(source, *m_context);
        if (result.isErr()) {
            return fail(attributePath(path, "source"), result.unwrapErr());
        }
        tileSet = result.unwrap();
    }
    else {
        tileSet = readTileSet(node, path);
        if (tileSet.ptr() == nullptr) {
            return false;
        }
    }

    if (!map.addTileSet(tileSet, firstGid)) {
        return fail(attributePath(path, "firstgid"), "overlaps another tile set");
    }
    return true;
}

Ref<TileSet> XmlReader::readTileSet(const pugi::xml_node& node, const std::string& path)
{
    if (!node) {
        fail("", "the root element must be <tileset>");
        return Ref<TileSet>();
    }

    std::string name;
    uint32_t tileWidth = 0;
    uint32_t tileHeight = 0;
    uint32_t margin = 0;
    uint32_t spacing = 0;
    if (!readString(node, "name", path, name) ||
        !readUint(node, "tilewidth", path, tileWidth, true) ||
        !readUint(node, "tileheight", path, tileHeight, true) ||
        !readUint(node, "margin", path, margin) || !readUint(node, "spacing", path, spacing)) {
        return Ref<TileSet>();
    }

    const pugi::xml_node imageNode = node.child("image");
    const std::string imagePath = std::format("{}/image", path);
    std::string image;
    uint32_t imageWidth = 0;
    uint32_t imageHeight = 0;
    if (!imageNode) {
        fail(imagePath, "tile sets without a single image are not supported");
        return Ref<TileSet>();
    }
    if (!readString(imageNode, "source", imagePath, image, true) ||
        !readUint(imageNode, "width", imagePath, imageWidth) ||
        !readUint(imageNode, "height", imagePath, imageHeight)) {
        return Ref<TileSet>();
    }

    Ref<TileSet> tileSet(new TileSet(tileWidth, tileHeight, margin, spacing));
    tileSet->setName(std::move(name));
    tileSet->setImage(toUtf8(resolvePath(m_baseDirectory, image)), imageWidth, imageHeight);

    uint32_t columns = tileSet->getColumns();
    uint32_t tileCount = tileSet->getTileCount();
    if (!readUint(node, "columns", path, columns) ||
        !readUint(node, "tilecount", path, tileCount)) {
        return Ref<TileSet>();
    }
    tileSet->setColumns(columns);
    tileSet->setTileCount(tileCount);

    if (const pugi::xml_node offset = node.child("tileoffset")) {
        math::vec2 tileOffset(0.0f, 0.0f);
        const std::string offsetPath = std::format("{}/tileoffset", path);
        if (!readFloat(offset, "x", offsetPath, tileOffset.x) ||
            !readFloat(offset, "y", offsetPath, tileOffset.y)) {
            return Ref<TileSet>();
        }
        tileSet->setTileOffset(tileOffset);
    }

    if (!readProperties(node, path, tileSet->getProperties())) {
        return Ref<TileSet>();
    }

    size_t index = 0;
    for (const pugi::xml_node& tile : node.children("tile")) {
        const std::string tilePath = childPath(path, tile, index++);
        uint32_t id = 0;
        TileProperties properties;
        if (!readUint(tile, "id", tilePath, id, true) ||
            !readProperties(tile, tilePath, properties)) {
            return Ref<TileSet>();
        }
        if (!properties.empty()) {
            tileSet->setTileProperties(id, std::move(properties));
        }
    }
    return tileSet;
}

bool XmlReader::readLayers(const pugi::xml_node& parent, const std::string& path, TileMap& map,
                           const LayerGroup& group)
{
    // Layers of all types share one order, so the children are counted together
    size_t index = 0;
    for (const pugi::xml_node& node : parent.children()) {
        const std::string_view type = node.name();
        if (type == "layer") {
            if (!readTileLayer(node, childPath(path, node, index++), map, group)) {
                return false;
            }
        }
        else if (type == "objectgroup") {
            if (!readObjectLayer(node, childPath(path, node, index++), map, group)) {
                return false;
            }
        }
        else if (type == "group") {
            const std::string groupPath = childPath(path, node, index++);
            bool visible = true;
            float opacity = 1.0f;
            math::vec2 offset(0.0f, 0.0f);
            if (!readGroupAttributes(node, groupPath, visible, opacity, offset)) {
                return false;
            }
            LayerGroup childGroup;
            childGroup.visible = group.visible && visible;
            childGroup.opacity = group.opacity * opacity;
            childGroup.offset = group.offset + offset;
            if (!readLayers(node, groupPath, map, childGroup)) {
                return false;
            }
        }
        else if (type == "imagelayer") {
            OCF_LOG_WARN("TiledMapLoader: {}: image layers are not supported",
                         childPath(path, node, index++));
        }
    }
    return true;
}

bool XmlReader::readGroupAttributes(const pugi::xml_node& node, const std::string& path,
                                    bool& visible, float& opacity, math::vec2& offset)
{
    return readBool(node, "visible", path, visible) && readFloat(node, "opacity", path, opacity) &&
           readFloat(node, "offsetx", path, offset.x) && readFloat(node, "offsety", path, offset.y);
}

bool XmlReader::readLayerAttributes(const pugi::xml_node& node, const std::string& path,
                                    TileMapLayer& layer, const LayerGroup& group)
{
    uint32_t id = 0;
    bool visible = true;
    float opacity = 1.0f;
    math::vec2 offset(0.0f, 0.0f);
    if (!readUint(node, "id", path, id) ||
        !readGroupAttributes(node, path, visible, opacity, offset) ||
        !readProperties(node, path, layer.getProperties())) {
        return false;
    }
    applyLayerAttributes(layer, group, id, visible, opacity, offset);
    return true;
}

bool XmlReader::readTileLayer(const pugi::xml_node& node, const std::string& path, TileMap& map,
                              const LayerGroup& group)
{
    std::string name;
    uint32_t width = map.getWidth();
    uint32_t height = map.getHeight();
    if (!readString(node, "name", path, name) || !readUint(node, "width", path, width) ||
        !readUint(node, "height", path, height)) {
        return false;
    }
    if (width != map.getWidth() || height != map.getHeight()) {
        return fail(path, "the layer must be as large as the map");
    }

    TileLayer* layer = map.addTileLayer(std::move(name));
    if (!readLayerAttributes(node, path, *layer, group)) {
        return false;
    }

    const std::string dataPath = std::format("{}/data", path);
    std::vector<TileGid> tiles;
    if (!readTileData(node.child("data"), dataPath, static_cast<size_t>(width) * height, tiles)) {
        return false;
    }

    Result<void, std::string> result = setLayerTiles(map, *layer, std::move(tiles));
    if (result.isErr()) {
        return fail(dataPath, result.unwrapErr());
    }
    return true;
}

bool XmlReader::readTileData(const pugi::xml_node& node, const std::string& path,
                             size_t expectedCount, std::vector<TileGid>& tiles)
{
    if (!node) {
        return fail(path, "is required");
    }
    if (node.child("chunk")) {
        return fail(path, "infinite maps are not supported");
    }

    std::string encoding;
    std::string compression;
    if (!readString(node, "encoding", path, encoding) ||
        !readString(node, "compression", path, compression)) {
        return false;
    }

    if (encoding.empty()) {
        // One <tile> element per cell, an old format that Tiled Map Editor can still write
        size_t index = 0;
        for (const pugi::xml_node& tile : node.children("tile")) {
            TileGid gid = 0;
            if (!readUint(tile, "gid", childPath(path, tile, index++), gid)) {
                return false;
            }
            tiles.push_back(gid);
        }
        return true;
    }

    Result<std::vector<TileGid>, std::string> result =
        encoding == "csv"      ? decodeCsvTiles(node.child_value(), expectedCount)
        : encoding == "base64" ? decodeBase64Tiles(node.child_value(), compression, expectedCount)
                               : Result<std::vector<TileGid>, std::string>::Err(
                                     std::format("unknown encoding \"{}\"", encoding));
    if (result.isErr()) {
        return fail(path, result.unwrapErr());
    }
    tiles = std::move(result.unwrap());
    return true;
}

bool XmlReader::readObjectLayer(const pugi::xml_node& node, const std::string& path, TileMap& map,
                                const LayerGroup& group)
{
    std::string name;
    if (!readString(node, "name", path, name)) {
        return false;
    }

    ObjectLayer* layer = map.addObjectLayer(std::move(name));
    if (!readLayerAttributes(node, path, *layer, group)) {
        return false;
    }

    size_t index = 0;
    for (const pugi::xml_node& item : node.children("object")) {
        MapObject object;
        if (!readObject(item, childPath(path, item, index++), object)) {
            return false;
        }
        layer->addObject(std::move(object));
    }
    return true;
}

bool XmlReader::readObject(const pugi::xml_node& node, const std::string& path, MapObject& object)
{
    if (node.attribute("template")) {
        return fail(attributePath(path, "template"), "object templates are not supported");
    }

    if (!readUint(node, "id", path, object.id) || !readString(node, "name", path, object.name) ||
        !readString(node, "type", path, object.className) ||
        !readString(node, "class", path, object.className) ||
        !readFloat(node, "x", path, object.position.x) ||
        !readFloat(node, "y", path, object.position.y) ||
        !readFloat(node, "width", path, object.size.x) ||
        !readFloat(node, "height", path, object.size.y) ||
        !readFloat(node, "rotation", path, object.rotation) ||
        !readUint(node, "gid", path, object.gid) ||
        !readBool(node, "visible", path, object.visible) ||
        !readProperties(node, path, object.properties)) {
        return false;
    }

    if (node.child("ellipse")) {
        object.shape = MapObject::Shape::Ellipse;
    }
    else if (node.child("point")) {
        object.shape = MapObject::Shape::Point;
    }
    else if (const pugi::xml_node polygon = node.child("polygon")) {
        object.shape = MapObject::Shape::Polygon;
        return readPoints(polygon, std::format("{}/polygon", path), object.points);
    }
    else if (const pugi::xml_node polyline = node.child("polyline")) {
        object.shape = MapObject::Shape::Polyline;
        return readPoints(polyline, std::format("{}/polyline", path), object.points);
    }
    else if (const pugi::xml_node text = node.child("text")) {
        object.shape = MapObject::Shape::Text;
        object.text = text.child_value();
    }
    return true;
}

bool XmlReader::readPoints(const pugi::xml_node& node, const std::string& path,
                           std::vector<math::vec2>& result)
{
    // "x,y x,y ..."
    std::string_view points = node.attribute("points").value();
    while (!points.empty()) {
        const size_t space = points.find(' ');
        const std::string_view point = points.substr(0, space);
        if (!point.empty()) {
            const size_t comma = point.find(',');
            math::vec2 position(0.0f, 0.0f);
            if (comma == std::string_view::npos ||
                !parseNumber(point.substr(0, comma), position.x) ||
                !parseNumber(point.substr(comma + 1), position.y)) {
                return fail(attributePath(path, "points"),
                            std::format("invalid point \"{}\"", point));
            }
            result.push_back(position);
        }
        if (space == std::string_view::npos) {
            break;
        }
        points.remove_prefix(space + 1);
    }
    return true;
}

bool XmlReader::readProperties(const pugi::xml_node& node, const std::string& path,
                               TileProperties& properties)
{
    const std::string propertiesPath = std::format("{}/properties", path);
    size_t index = 0;
    for (const pugi::xml_node& property : node.child("properties").children("property")) {
        const std::string propertyPath = childPath(propertiesPath, property, index++);

        std::string name;
        std::string type = "string";
        if (!readString(property, "name", propertyPath, name, true) ||
            !readString(property, "type", propertyPath, type)) {
            return false;
        }

        // Strings with line breaks are written as the text of the element
        const pugi::xml_attribute valueAttribute = property.attribute("value");
        const std::string_view value =
            valueAttribute ? valueAttribute.value() : property.child_value();
        const std::string valuePath = attributePath(propertyPath, "value");

        if (type == "bool") {
            bool result = false;
            if (!readBool(property, "value", propertyPath, result)) {
                return false;
            }
            properties[name] = result;
        }
        else if (type == "int" || type == "object") {
            int result = 0;
            if (!parseNumber(value, result)) {
                return fail(valuePath, "must be a 32-bit integer");
            }
            properties[name] = result;
        }
        else if (type == "float") {
            float result = 0.0f;
            if (!parseNumber(value, result)) {
                return fail(valuePath, "must be a number");
            }
            properties[name] = result;
        }
        else if (type == "string" || type == "color" || type == "file") {
            properties[name] = type == "file" && !value.empty()
                                   ? toUtf8(resolvePath(m_baseDirectory, value))
                                   : std::string(value);
        }
        else if (type == "class") {
            // Nested properties are not represented
            continue;
        }
        else {
            return fail(attributePath(propertyPath, "type"),
                        std::format("unknown type \"{}\"", type));
        }
    }
    return true;
}

} // namespace

Result<Ref<TileMap>, std::string> readMapXml(std::string_view text, const ParseContext& context)
{
    using MapResult = Result<Ref<TileMap>, std::string>;

    Result<std::unique_ptr<pugi::xml_document>, std::string> document = parseDocument(text);
    if (document.isErr()) {
        return MapResult::Err(document.unwrapErr());
    }

    XmlReader reader(&context, context.baseDirectory);
    Ref<TileMap> map = reader.readMap(document.unwrap()->child("map"));
    if (map.ptr() == nullptr) {
        return MapResult::Err(reader.getError());
    }
    return MapResult::Ok(std::move(map));
}

Result<Ref<TileSet>, std::string> readTileSetXml(std::string_view text,
                                                 const std::filesystem::path& baseDirectory)
{
    using TileSetResult = Result<Ref<TileSet>, std::string>;

    Result<std::unique_ptr<pugi::xml_document>, std::string> document = parseDocument(text);
    if (document.isErr()) {
        return TileSetResult::Err(document.unwrapErr());
    }

    XmlReader reader(nullptr, baseDirectory);
    Ref<TileSet> tileSet = reader.readTileSet(document.unwrap()->child("tileset"), "/tileset");
    if (tileSet.ptr() == nullptr) {
        return TileSetResult::Err(reader.getError());
    }
    return TileSetResult::Ok(std::move(tileSet));
}

} // namespace ocf::tiled
