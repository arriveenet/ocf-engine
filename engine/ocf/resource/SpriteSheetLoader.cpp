// SPDX-License-Identifier: MIT
#include "ocf/resource/SpriteSheetLoader.h"

#include "ocf/core/Engine.h"
#include "ocf/resource/AssetUtility.h"
#include "ocf/resource/SpriteSheetCommon.h"
#include "ocf/resource/TextureManager.h"

#include <format>

namespace ocf {

using spritesheet::SheetResult;

Result<Ref<SpriteSheet>, std::string> SpriteSheetLoader::load(Engine& engine,
                                                              std::string_view assetPath)
{
    const std::filesystem::path path = asset::resolvePath(std::filesystem::path(), assetPath);
    const std::optional<std::string> text = asset::readFile(path);
    if (!text) {
        return SheetResult::Err(std::format("cannot read {}", assetPath));
    }

    SheetResult result = loadFromString(*text, Format::Auto, path.parent_path());
    if (result.isErr()) {
        return SheetResult::Err(std::format("{}: {}", assetPath, result.unwrapErr()));
    }

    Ref<SpriteSheet> sheet = result.unwrap();
    sheet->setName(asset::toUtf8(path));

    Result<void, std::string> texture = resolveTexture(*sheet.ptr(), engine.getTextureManager());
    if (texture.isErr()) {
        return SheetResult::Err(std::format("{}: {}", assetPath, texture.unwrapErr()));
    }
    return SheetResult::Ok(std::move(sheet));
}

Result<Ref<SpriteSheet>, std::string>
SpriteSheetLoader::loadFromString(std::string_view text, Format format,
                                  const std::filesystem::path& baseDirectory)
{
    if (format == Format::Auto) {
        const std::optional<Format> detected = detectFormat(text);
        if (!detected) {
            return SheetResult::Err("unknown sprite sheet format");
        }
        format = *detected;
    }

    switch (format) {
    case Format::Json:
        return spritesheet::readJson(text, baseDirectory);
    case Format::Auto:
        break;
    }
    return SheetResult::Err("unknown sprite sheet format");
}

std::optional<SpriteSheetLoader::Format> SpriteSheetLoader::detectFormat(std::string_view text)
{
    constexpr std::string_view BOM = "\xEF\xBB\xBF";
    if (text.starts_with(BOM)) {
        text.remove_prefix(BOM.size());
    }
    const size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) {
        return std::nullopt;
    }

    // XML formats start with '<' and are told apart by their root element
    if (text[first] == '{') {
        return Format::Json;
    }
    return std::nullopt;
}

Result<void, std::string> SpriteSheetLoader::resolveTexture(SpriteSheet& sheet,
                                                            TextureManager& textureManager)
{
    if (sheet.getTexture() != nullptr) {
        return Result<void, std::string>::Ok();
    }

    Texture* texture = textureManager.addImage(sheet.getImagePath());
    if (texture == nullptr) {
        return Result<void, std::string>::Err(
            std::format("cannot load image {}", sheet.getImagePath()));
    }
    sheet.setTexture(texture);
    return Result<void, std::string>::Ok();
}

} // namespace ocf
