// SPDX-License-Identifier: MIT
#include "ocf/resource/TiledMapLoader.h"

#include "ocf/core/Engine.h"
#include "ocf/platform/FileSystem.h"
#include "ocf/resource/TextureManager.h"
#include "ocf/resource/TiledCommon.h"

#include <format>
#include <fstream>
#include <iterator>

namespace ocf {

Result<Ref<TileMap>, std::string> TiledMapLoader::load(Engine& engine, std::string_view assetPath)
{
    using MapResult = Result<Ref<TileMap>, std::string>;

    const std::filesystem::path path = tiled::resolvePath(std::filesystem::path(), assetPath);
    const std::optional<std::string> text = readAssetFile(path);
    if (!text) {
        return MapResult::Err(std::format("cannot read {}", assetPath));
    }

    const Format format = path.extension() == ".tmx" ? Format::Xml : Format::Json;
    MapResult result = loadFromString(*text, format, path.parent_path());
    if (result.isErr()) {
        return MapResult::Err(std::format("{}: {}", assetPath, result.unwrapErr()));
    }

    Ref<TileMap> map = result.unwrap();
    map->setName(tiled::toUtf8(path));

    Result<void, std::string> textures = resolveTextures(*map.ptr(), engine.getTextureManager());
    if (textures.isErr()) {
        return MapResult::Err(std::format("{}: {}", assetPath, textures.unwrapErr()));
    }
    return MapResult::Ok(std::move(map));
}

Result<Ref<TileMap>, std::string>
TiledMapLoader::loadFromString(std::string_view text, Format format,
                               const std::filesystem::path& baseDirectory, const FileReader& reader)
{
    tiled::ParseContext context;
    context.baseDirectory = baseDirectory;
    context.reader = reader ? reader : FileReader(&TiledMapLoader::readAssetFile);

    return format == Format::Xml ? tiled::readMapXml(text, context)
                                 : tiled::readMapJson(text, context);
}

Result<void, std::string> TiledMapLoader::resolveTextures(TileMap& map,
                                                          TextureManager& textureManager)
{
    for (const TileMap::TileSetEntry& entry : map.getTileSets()) {
        TileSet& tileSet = *entry.tileSet.ptr();
        if (tileSet.getTexture() != nullptr) {
            continue;
        }

        Texture* texture = textureManager.addImage(tileSet.getImagePath());
        if (texture == nullptr) {
            return Result<void, std::string>::Err(
                std::format("cannot load image {} of tile set \"{}\"", tileSet.getImagePath(),
                            tileSet.getName()));
        }
        tileSet.setTexture(texture);
    }
    return Result<void, std::string>::Ok();
}

std::optional<std::string> TiledMapLoader::readAssetFile(const std::filesystem::path& assetPath)
{
    const std::string fullPath =
        FileSystem::getInstance()->getAssetFullPath(tiled::toUtf8(assetPath));
    if (fullPath.empty()) {
        return std::nullopt;
    }

    std::ifstream file(fullPath, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (file.bad()) {
        return std::nullopt;
    }
    return text;
}

} // namespace ocf
