// SPDX-License-Identifier: MIT
#include "ocf/resource/TiledCommon.h"

#include "ocf/core/Logger.h"

#include <yyjson.h>

#include <cstdint>
#include <format>
#include <limits>
#include <memory>

namespace ocf::tiled {

namespace {

struct DocumentDeleter {
    void operator()(yyjson_doc* document) const { yyjson_doc_free(document); }
};

using Document = std::unique_ptr<yyjson_doc, DocumentDeleter>;

Result<Document, std::string> parseDocument(std::string_view json)
{
    // A UTF-8 BOM is accepted, since some editors add one
    const yyjson_read_flag flags = YYJSON_READ_ALLOW_BOM | YYJSON_READ_ALLOW_INVALID_UNICODE;
    yyjson_read_err error{};
    // The text is not modified without YYJSON_READ_INSITU
    Document document(
        yyjson_read_opts(const_cast<char*>(json.data()), json.size(), flags, nullptr, &error));
    if (!document) {
        size_t line = 0;
        size_t column = 0;
        size_t character = 0;
        if (yyjson_locate_pos(json.data(), json.size(), error.pos, &line, &column, &character)) {
            return Result<Document, std::string>::Err(
                std::format("invalid JSON at line {}, column {}: {}", line, column, error.msg));
        }
        return Result<Document, std::string>::Err(std::format("invalid JSON: {}", error.msg));
    }
    return Result<Document, std::string>::Ok(std::move(document));
}

std::string memberPath(const std::string& path, std::string_view key)
{
    return std::format("{}/{}", path, key);
}

std::string itemPath(const std::string& path, size_t index)
{
    return std::format("{}/{}", path, index);
}

std::string_view getString(yyjson_val* value)
{
    return std::string_view(yyjson_get_str(value), yyjson_get_len(value));
}

/** Reads a map or tile set, keeping the first error. Every function returns false on error. */
class JsonReader {
public:
    /** @param context Used for external tile sets; nullptr when reading a tile set file. */
    JsonReader(const ParseContext* context, std::filesystem::path baseDirectory)
        : m_context(context)
        , m_baseDirectory(std::move(baseDirectory))
    {
    }

    Ref<TileMap> readMap(yyjson_val* root);
    Ref<TileSet> readTileSet(yyjson_val* object, const std::string& path);

    const std::string& getError() const { return m_error; }

private:
    bool readTileSetEntry(yyjson_val* value, const std::string& path, TileMap& map);
    bool readLayers(yyjson_val* layers, const std::string& path, TileMap& map,
                    const LayerGroup& group);
    bool readTileLayer(yyjson_val* value, const std::string& path, TileMap& map,
                       const LayerGroup& group);
    bool readObjectLayer(yyjson_val* value, const std::string& path, TileMap& map,
                         const LayerGroup& group);
    bool readLayerAttributes(yyjson_val* value, const std::string& path, TileMapLayer& layer,
                             const LayerGroup& group);
    bool readObject(yyjson_val* value, const std::string& path, MapObject& object);
    bool readPoints(yyjson_val* points, const std::string& path, std::vector<math::vec2>& result);
    bool readProperties(yyjson_val* object, const std::string& path, TileProperties& properties);

    bool fail(const std::string& path, std::string_view message);

    // The members are optional unless `required`. An absent member leaves `result` unchanged.
    bool readUint(yyjson_val* object, const char* key, const std::string& path, uint32_t& result,
                  bool required = false);
    bool readFloat(yyjson_val* object, const char* key, const std::string& path, float& result);
    bool readString(yyjson_val* object, const char* key, const std::string& path,
                    std::string& result, bool required = false);
    bool readBool(yyjson_val* object, const char* key, const std::string& path, bool& result);
    bool getArray(yyjson_val* object, const char* key, const std::string& path,
                  yyjson_val*& result);

    const ParseContext* m_context;
    std::filesystem::path m_baseDirectory;
    std::string m_error;
};

bool JsonReader::fail(const std::string& path, std::string_view message)
{
    if (m_error.empty()) {
        m_error = path.empty() ? std::string(message) : std::format("{}: {}", path, message);
    }
    return false;
}

bool JsonReader::readUint(yyjson_val* object, const char* key, const std::string& path,
                          uint32_t& result, bool required)
{
    yyjson_val* member = yyjson_obj_get(object, key);
    if (member == nullptr) {
        return required ? fail(memberPath(path, key), "is required") : true;
    }
    if (!yyjson_is_uint(member) || yyjson_get_uint(member) > std::numeric_limits<uint32_t>::max()) {
        return fail(memberPath(path, key), "must be an unsigned 32-bit integer");
    }
    result = static_cast<uint32_t>(yyjson_get_uint(member));
    return true;
}

bool JsonReader::readFloat(yyjson_val* object, const char* key, const std::string& path,
                           float& result)
{
    yyjson_val* member = yyjson_obj_get(object, key);
    if (member == nullptr) {
        return true;
    }
    if (!yyjson_is_num(member)) {
        return fail(memberPath(path, key), "must be a number");
    }
    result = static_cast<float>(yyjson_get_num(member));
    return true;
}

bool JsonReader::readString(yyjson_val* object, const char* key, const std::string& path,
                            std::string& result, bool required)
{
    yyjson_val* member = yyjson_obj_get(object, key);
    if (member == nullptr) {
        return required ? fail(memberPath(path, key), "is required") : true;
    }
    if (!yyjson_is_str(member)) {
        return fail(memberPath(path, key), "must be a string");
    }
    result = getString(member);
    return true;
}

bool JsonReader::readBool(yyjson_val* object, const char* key, const std::string& path,
                          bool& result)
{
    yyjson_val* member = yyjson_obj_get(object, key);
    if (member == nullptr) {
        return true;
    }
    if (!yyjson_is_bool(member)) {
        return fail(memberPath(path, key), "must be true or false");
    }
    result = yyjson_get_bool(member);
    return true;
}

bool JsonReader::getArray(yyjson_val* object, const char* key, const std::string& path,
                          yyjson_val*& result)
{
    result = yyjson_obj_get(object, key);
    if (result != nullptr && !yyjson_is_arr(result)) {
        return fail(memberPath(path, key), "must be an array");
    }
    return true;
}

Ref<TileMap> JsonReader::readMap(yyjson_val* root)
{
    const std::string path;
    if (!yyjson_is_obj(root)) {
        fail(path, "the root must be an object");
        return Ref<TileMap>();
    }

    std::string orientation = "orthogonal";
    bool infinite = false;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t tileWidth = 0;
    uint32_t tileHeight = 0;
    std::string backgroundColor;
    if (!readString(root, "orientation", path, orientation) ||
        !readBool(root, "infinite", path, infinite) ||
        !readUint(root, "width", path, width, true) ||
        !readUint(root, "height", path, height, true) ||
        !readUint(root, "tilewidth", path, tileWidth, true) ||
        !readUint(root, "tileheight", path, tileHeight, true) ||
        !readString(root, "backgroundcolor", path, backgroundColor)) {
        return Ref<TileMap>();
    }
    if (orientation != "orthogonal") {
        fail("/orientation", std::format("\"{}\" maps are not supported", orientation));
        return Ref<TileMap>();
    }
    if (infinite) {
        fail("/infinite", "infinite maps are not supported");
        return Ref<TileMap>();
    }

    Ref<TileMap> map(new TileMap(width, height, tileWidth, tileHeight));
    if (!backgroundColor.empty()) {
        const std::optional<math::vec4> color = parseColor(backgroundColor);
        if (!color) {
            fail("/backgroundcolor", "must be a color like #RRGGBB or #AARRGGBB");
            return Ref<TileMap>();
        }
        map->setBackgroundColor(*color);
    }
    if (!readProperties(root, path, map->getProperties())) {
        return Ref<TileMap>();
    }

    yyjson_val* tileSets = nullptr;
    if (!getArray(root, "tilesets", path, tileSets)) {
        return Ref<TileMap>();
    }
    size_t index = 0;
    size_t max = 0;
    yyjson_val* tileSet = nullptr;
    yyjson_arr_foreach(tileSets, index, max, tileSet)
    {
        if (!readTileSetEntry(tileSet, itemPath("/tilesets", index), *map.ptr())) {
            return Ref<TileMap>();
        }
    }

    yyjson_val* layers = nullptr;
    if (!getArray(root, "layers", path, layers) ||
        !readLayers(layers, "/layers", *map.ptr(), LayerGroup())) {
        return Ref<TileMap>();
    }
    return map;
}

bool JsonReader::readTileSetEntry(yyjson_val* value, const std::string& path, TileMap& map)
{
    if (!yyjson_is_obj(value)) {
        return fail(path, "must be an object");
    }

    uint32_t firstGid = 0;
    std::string source;
    if (!readUint(value, "firstgid", path, firstGid, true) ||
        !readString(value, "source", path, source)) {
        return false;
    }

    Ref<TileSet> tileSet;
    if (!source.empty()) {
        Result<Ref<TileSet>, std::string> result = readExternalTileSet(source, *m_context);
        if (result.isErr()) {
            return fail(memberPath(path, "source"), result.unwrapErr());
        }
        tileSet = result.unwrap();
    }
    else {
        tileSet = readTileSet(value, path);
        if (tileSet.ptr() == nullptr) {
            return false;
        }
    }

    if (!map.addTileSet(tileSet, firstGid)) {
        return fail(memberPath(path, "firstgid"), "overlaps another tile set");
    }
    return true;
}

Ref<TileSet> JsonReader::readTileSet(yyjson_val* object, const std::string& path)
{
    if (!yyjson_is_obj(object)) {
        fail(path, "must be an object");
        return Ref<TileSet>();
    }

    std::string name;
    uint32_t tileWidth = 0;
    uint32_t tileHeight = 0;
    uint32_t margin = 0;
    uint32_t spacing = 0;
    std::string image;
    uint32_t imageWidth = 0;
    uint32_t imageHeight = 0;
    if (!readString(object, "name", path, name) ||
        !readUint(object, "tilewidth", path, tileWidth, true) ||
        !readUint(object, "tileheight", path, tileHeight, true) ||
        !readUint(object, "margin", path, margin) || !readUint(object, "spacing", path, spacing) ||
        !readString(object, "image", path, image) ||
        !readUint(object, "imagewidth", path, imageWidth) ||
        !readUint(object, "imageheight", path, imageHeight)) {
        return Ref<TileSet>();
    }
    if (image.empty()) {
        fail(memberPath(path, "image"), "tile sets without a single image are not supported");
        return Ref<TileSet>();
    }

    Ref<TileSet> tileSet(new TileSet(tileWidth, tileHeight, margin, spacing));
    tileSet->setName(std::move(name));
    tileSet->setImage(toUtf8(resolvePath(m_baseDirectory, image)), imageWidth, imageHeight);

    uint32_t columns = tileSet->getColumns();
    uint32_t tileCount = tileSet->getTileCount();
    if (!readUint(object, "columns", path, columns) ||
        !readUint(object, "tilecount", path, tileCount)) {
        return Ref<TileSet>();
    }
    tileSet->setColumns(columns);
    tileSet->setTileCount(tileCount);

    if (yyjson_val* offset = yyjson_obj_get(object, "tileoffset")) {
        math::vec2 tileOffset(0.0f, 0.0f);
        const std::string offsetPath = memberPath(path, "tileoffset");
        if (!readFloat(offset, "x", offsetPath, tileOffset.x) ||
            !readFloat(offset, "y", offsetPath, tileOffset.y)) {
            return Ref<TileSet>();
        }
        tileSet->setTileOffset(tileOffset);
    }

    if (!readProperties(object, path, tileSet->getProperties())) {
        return Ref<TileSet>();
    }

    yyjson_val* tiles = nullptr;
    if (!getArray(object, "tiles", path, tiles)) {
        return Ref<TileSet>();
    }
    size_t index = 0;
    size_t max = 0;
    yyjson_val* tile = nullptr;
    yyjson_arr_foreach(tiles, index, max, tile)
    {
        const std::string tilePath = itemPath(memberPath(path, "tiles"), index);
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

bool JsonReader::readLayers(yyjson_val* layers, const std::string& path, TileMap& map,
                            const LayerGroup& group)
{
    size_t index = 0;
    size_t max = 0;
    yyjson_val* layer = nullptr;
    yyjson_arr_foreach(layers, index, max, layer)
    {
        const std::string layerPath = itemPath(path, index);
        if (!yyjson_is_obj(layer)) {
            return fail(layerPath, "must be an object");
        }

        std::string type;
        if (!readString(layer, "type", layerPath, type, true)) {
            return false;
        }

        if (type == "tilelayer") {
            if (!readTileLayer(layer, layerPath, map, group)) {
                return false;
            }
        }
        else if (type == "objectgroup") {
            if (!readObjectLayer(layer, layerPath, map, group)) {
                return false;
            }
        }
        else if (type == "group") {
            LayerGroup childGroup = group;
            bool visible = true;
            float opacity = 1.0f;
            math::vec2 offset(0.0f, 0.0f);
            yyjson_val* children = nullptr;
            if (!readBool(layer, "visible", layerPath, visible) ||
                !readFloat(layer, "opacity", layerPath, opacity) ||
                !readFloat(layer, "offsetx", layerPath, offset.x) ||
                !readFloat(layer, "offsety", layerPath, offset.y) ||
                !getArray(layer, "layers", layerPath, children)) {
                return false;
            }
            childGroup.visible = group.visible && visible;
            childGroup.opacity = group.opacity * opacity;
            childGroup.offset = group.offset + offset;
            if (!readLayers(children, memberPath(layerPath, "layers"), map, childGroup)) {
                return false;
            }
        }
        else if (type == "imagelayer") {
            OCF_LOG_WARN("TiledMapLoader: {}: image layers are not supported", layerPath);
        }
        else {
            return fail(memberPath(layerPath, "type"), std::format("unknown type \"{}\"", type));
        }
    }
    return true;
}

bool JsonReader::readLayerAttributes(yyjson_val* value, const std::string& path,
                                     TileMapLayer& layer, const LayerGroup& group)
{
    uint32_t id = 0;
    bool visible = true;
    float opacity = 1.0f;
    math::vec2 offset(0.0f, 0.0f);
    if (!readUint(value, "id", path, id) || !readBool(value, "visible", path, visible) ||
        !readFloat(value, "opacity", path, opacity) ||
        !readFloat(value, "offsetx", path, offset.x) ||
        !readFloat(value, "offsety", path, offset.y) ||
        !readProperties(value, path, layer.getProperties())) {
        return false;
    }
    applyLayerAttributes(layer, group, id, visible, opacity, offset);
    return true;
}

bool JsonReader::readTileLayer(yyjson_val* value, const std::string& path, TileMap& map,
                               const LayerGroup& group)
{
    std::string name;
    uint32_t width = map.getWidth();
    uint32_t height = map.getHeight();
    std::string encoding = "csv";
    std::string compression;
    if (!readString(value, "name", path, name) || !readUint(value, "width", path, width) ||
        !readUint(value, "height", path, height) ||
        !readString(value, "encoding", path, encoding) ||
        !readString(value, "compression", path, compression)) {
        return false;
    }
    if (yyjson_obj_get(value, "chunks") != nullptr) {
        return fail(memberPath(path, "chunks"), "infinite maps are not supported");
    }
    if (width != map.getWidth() || height != map.getHeight()) {
        return fail(path, "the layer must be as large as the map");
    }

    TileLayer* layer = map.addTileLayer(std::move(name));
    if (!readLayerAttributes(value, path, *layer, group)) {
        return false;
    }

    const std::string dataPath = memberPath(path, "data");
    yyjson_val* data = yyjson_obj_get(value, "data");
    if (data == nullptr) {
        return fail(dataPath, "is required");
    }

    const size_t expectedCount = static_cast<size_t>(width) * height;
    std::vector<TileGid> tiles;
    if (encoding == "csv") {
        if (!yyjson_is_arr(data)) {
            return fail(dataPath, "must be an array");
        }
        tiles.reserve(yyjson_arr_size(data));
        size_t index = 0;
        size_t max = 0;
        yyjson_val* tile = nullptr;
        yyjson_arr_foreach(data, index, max, tile)
        {
            if (!yyjson_is_uint(tile) ||
                yyjson_get_uint(tile) > std::numeric_limits<uint32_t>::max()) {
                return fail(itemPath(dataPath, index), "must be an unsigned 32-bit integer");
            }
            tiles.push_back(static_cast<TileGid>(yyjson_get_uint(tile)));
        }
    }
    else if (encoding == "base64") {
        if (!yyjson_is_str(data)) {
            return fail(dataPath, "must be a string");
        }
        Result<std::vector<TileGid>, std::string> result =
            decodeBase64Tiles(getString(data), compression, expectedCount);
        if (result.isErr()) {
            return fail(dataPath, result.unwrapErr());
        }
        tiles = std::move(result.unwrap());
    }
    else {
        return fail(memberPath(path, "encoding"), std::format("unknown encoding \"{}\"", encoding));
    }

    Result<void, std::string> result = setLayerTiles(map, *layer, std::move(tiles));
    if (result.isErr()) {
        return fail(dataPath, result.unwrapErr());
    }
    return true;
}

bool JsonReader::readObjectLayer(yyjson_val* value, const std::string& path, TileMap& map,
                                 const LayerGroup& group)
{
    std::string name;
    yyjson_val* objects = nullptr;
    if (!readString(value, "name", path, name) || !getArray(value, "objects", path, objects)) {
        return false;
    }

    ObjectLayer* layer = map.addObjectLayer(std::move(name));
    if (!readLayerAttributes(value, path, *layer, group)) {
        return false;
    }

    size_t index = 0;
    size_t max = 0;
    yyjson_val* item = nullptr;
    yyjson_arr_foreach(objects, index, max, item)
    {
        MapObject object;
        if (!readObject(item, itemPath(memberPath(path, "objects"), index), object)) {
            return false;
        }
        layer->addObject(std::move(object));
    }
    return true;
}

bool JsonReader::readObject(yyjson_val* value, const std::string& path, MapObject& object)
{
    if (!yyjson_is_obj(value)) {
        return fail(path, "must be an object");
    }
    if (yyjson_obj_get(value, "template") != nullptr) {
        return fail(memberPath(path, "template"), "object templates are not supported");
    }

    bool isEllipse = false;
    bool isPoint = false;
    if (!readUint(value, "id", path, object.id) || !readString(value, "name", path, object.name) ||
        !readString(value, "type", path, object.className) ||
        !readString(value, "class", path, object.className) ||
        !readFloat(value, "x", path, object.position.x) ||
        !readFloat(value, "y", path, object.position.y) ||
        !readFloat(value, "width", path, object.size.x) ||
        !readFloat(value, "height", path, object.size.y) ||
        !readFloat(value, "rotation", path, object.rotation) ||
        !readUint(value, "gid", path, object.gid) ||
        !readBool(value, "visible", path, object.visible) ||
        !readBool(value, "ellipse", path, isEllipse) || !readBool(value, "point", path, isPoint) ||
        !readProperties(value, path, object.properties)) {
        return false;
    }

    yyjson_val* polygon = nullptr;
    yyjson_val* polyline = nullptr;
    if (!getArray(value, "polygon", path, polygon) ||
        !getArray(value, "polyline", path, polyline)) {
        return false;
    }

    if (isEllipse) {
        object.shape = MapObject::Shape::Ellipse;
    }
    else if (isPoint) {
        object.shape = MapObject::Shape::Point;
    }
    else if (polygon != nullptr) {
        object.shape = MapObject::Shape::Polygon;
        return readPoints(polygon, memberPath(path, "polygon"), object.points);
    }
    else if (polyline != nullptr) {
        object.shape = MapObject::Shape::Polyline;
        return readPoints(polyline, memberPath(path, "polyline"), object.points);
    }
    else if (yyjson_val* text = yyjson_obj_get(value, "text")) {
        object.shape = MapObject::Shape::Text;
        return readString(text, "text", memberPath(path, "text"), object.text);
    }
    return true;
}

bool JsonReader::readPoints(yyjson_val* points, const std::string& path,
                            std::vector<math::vec2>& result)
{
    size_t index = 0;
    size_t max = 0;
    yyjson_val* point = nullptr;
    yyjson_arr_foreach(points, index, max, point)
    {
        const std::string pointPath = itemPath(path, index);
        if (!yyjson_is_obj(point)) {
            return fail(pointPath, "must be an object");
        }
        math::vec2 position(0.0f, 0.0f);
        if (!readFloat(point, "x", pointPath, position.x) ||
            !readFloat(point, "y", pointPath, position.y)) {
            return false;
        }
        result.push_back(position);
    }
    return true;
}

bool JsonReader::readProperties(yyjson_val* object, const std::string& path,
                                TileProperties& properties)
{
    const std::string propertiesPath = memberPath(path, "properties");
    yyjson_val* array = nullptr;
    if (!getArray(object, "properties", path, array)) {
        return false;
    }
    if (array == nullptr) {
        return true;
    }

    size_t index = 0;
    size_t max = 0;
    yyjson_val* property = nullptr;
    yyjson_arr_foreach(array, index, max, property)
    {
        const std::string propertyPath = itemPath(propertiesPath, index);
        if (!yyjson_is_obj(property)) {
            return fail(propertyPath, "must be an object");
        }

        std::string name;
        std::string type = "string";
        if (!readString(property, "name", propertyPath, name, true) ||
            !readString(property, "type", propertyPath, type)) {
            return false;
        }

        const std::string valuePath = memberPath(propertyPath, "value");
        yyjson_val* value = yyjson_obj_get(property, "value");
        if (value == nullptr) {
            return fail(valuePath, "is required");
        }

        if (type == "bool") {
            if (!yyjson_is_bool(value)) {
                return fail(valuePath, "must be true or false");
            }
            properties[name] = yyjson_get_bool(value);
        }
        else if (type == "int" || type == "object") {
            if (!yyjson_is_int(value) || yyjson_get_sint(value) < std::numeric_limits<int>::min() ||
                yyjson_get_sint(value) > std::numeric_limits<int>::max()) {
                return fail(valuePath, "must be a 32-bit integer");
            }
            properties[name] = static_cast<int>(yyjson_get_sint(value));
        }
        else if (type == "float") {
            if (!yyjson_is_num(value)) {
                return fail(valuePath, "must be a number");
            }
            properties[name] = static_cast<float>(yyjson_get_num(value));
        }
        else if (type == "string" || type == "color" || type == "file") {
            if (!yyjson_is_str(value)) {
                return fail(valuePath, "must be a string");
            }
            const std::string_view text = getString(value);
            properties[name] = type == "file" && !text.empty()
                                   ? toUtf8(resolvePath(m_baseDirectory, text))
                                   : std::string(text);
        }
        else if (type == "class") {
            // Nested properties are not represented
            continue;
        }
        else {
            return fail(memberPath(propertyPath, "type"), std::format("unknown type \"{}\"", type));
        }
    }
    return true;
}

} // namespace

Result<Ref<TileMap>, std::string> readMapJson(std::string_view text, const ParseContext& context)
{
    using MapResult = Result<Ref<TileMap>, std::string>;

    Result<Document, std::string> document = parseDocument(text);
    if (document.isErr()) {
        return MapResult::Err(document.unwrapErr());
    }

    JsonReader reader(&context, context.baseDirectory);
    Ref<TileMap> map = reader.readMap(yyjson_doc_get_root(document.unwrap().get()));
    if (map.ptr() == nullptr) {
        return MapResult::Err(reader.getError());
    }
    return MapResult::Ok(std::move(map));
}

Result<Ref<TileSet>, std::string> readTileSetJson(std::string_view text,
                                                  const std::filesystem::path& baseDirectory)
{
    using TileSetResult = Result<Ref<TileSet>, std::string>;

    Result<Document, std::string> document = parseDocument(text);
    if (document.isErr()) {
        return TileSetResult::Err(document.unwrapErr());
    }

    JsonReader reader(nullptr, baseDirectory);
    Ref<TileSet> tileSet = reader.readTileSet(yyjson_doc_get_root(document.unwrap().get()), "");
    if (tileSet.ptr() == nullptr) {
        return TileSetResult::Err(reader.getError());
    }
    return TileSetResult::Ok(std::move(tileSet));
}

} // namespace ocf::tiled
