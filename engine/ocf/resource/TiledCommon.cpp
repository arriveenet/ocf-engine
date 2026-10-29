// SPDX-License-Identifier: MIT
#include "ocf/resource/TiledCommon.h"

#include "ocf/core/base64.h"

#include <stb_image.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <format>
#include <limits>

namespace ocf::tiled {

namespace {

bool isSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

std::string_view trim(std::string_view text)
{
    while (!text.empty() && isSpace(text.front())) {
        text.remove_prefix(1);
    }
    while (!text.empty() && isSpace(text.back())) {
        text.remove_suffix(1);
    }
    return text;
}

/** The deflate stream of a gzip member (RFC 1952), without the header and the trailer. */
std::optional<std::string_view> getGzipDeflateStream(std::string_view data)
{
    constexpr size_t HEADER_SIZE = 10;
    constexpr size_t TRAILER_SIZE = 8; // CRC-32 and size
    constexpr uint8_t FLAG_HEADER_CRC = 0x02;
    constexpr uint8_t FLAG_EXTRA = 0x04;
    constexpr uint8_t FLAG_NAME = 0x08;
    constexpr uint8_t FLAG_COMMENT = 0x10;

    const auto byteAt = [&data](size_t index) { return static_cast<uint8_t>(data[index]); };

    if (data.size() < HEADER_SIZE + TRAILER_SIZE || byteAt(0) != 0x1f || byteAt(1) != 0x8b ||
        byteAt(2) != 8) {
        return std::nullopt;
    }
    const uint8_t flags = byteAt(3);
    size_t position = HEADER_SIZE;
    const size_t end = data.size() - TRAILER_SIZE;

    if (flags & FLAG_EXTRA) {
        if (position + 2 > end) {
            return std::nullopt;
        }
        position += 2 + (byteAt(position) | (byteAt(position + 1) << 8));
    }
    for (uint8_t flag : {FLAG_NAME, FLAG_COMMENT}) {
        if (flags & flag) {
            // Zero-terminated string
            while (position < end && data[position] != '\0') {
                position++;
            }
            position++;
        }
    }
    if (flags & FLAG_HEADER_CRC) {
        position += 2;
    }
    if (position > end) {
        return std::nullopt;
    }
    return data.substr(position, end - position);
}

} // namespace

Result<Ref<TileSet>, std::string> readExternalTileSet(std::string_view source,
                                                      const ParseContext& context)
{
    using TileSetResult = Result<Ref<TileSet>, std::string>;

    const std::filesystem::path path = resolvePath(context.baseDirectory, source);
    const std::optional<std::string> text = context.reader(path);
    if (!text) {
        return TileSetResult::Err(std::format("cannot read tile set {}", toUtf8(path)));
    }

    std::string extension = toUtf8(path.extension());
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    TileSetResult result = extension == ".tsx" ? readTileSetXml(*text, path.parent_path())
                                               : readTileSetJson(*text, path.parent_path());
    if (result.isErr()) {
        return TileSetResult::Err(std::format("{}: {}", toUtf8(path), result.unwrapErr()));
    }
    return result;
}

std::string toUtf8(const std::filesystem::path& path)
{
    const std::u8string text = path.generic_u8string();
    return std::string(text.begin(), text.end());
}

std::filesystem::path resolvePath(const std::filesystem::path& baseDirectory,
                                  std::string_view relativePath)
{
    const std::filesystem::path path(
        std::u8string(reinterpret_cast<const char8_t*>(relativePath.data()), relativePath.size()));
    return (baseDirectory / path).lexically_normal();
}

std::optional<math::vec4> parseColor(std::string_view text)
{
    if (!text.empty() && text.front() == '#') {
        text.remove_prefix(1);
    }
    if (text.size() != 6 && text.size() != 8) {
        return std::nullopt;
    }

    uint32_t value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value, 16);
    if (error != std::errc() || end != text.data() + text.size()) {
        return std::nullopt;
    }

    const auto channel = [value](int shift) {
        return static_cast<float>((value >> shift) & 0xff) / 255.0f;
    };
    const float alpha = text.size() == 8 ? channel(24) : 1.0f;
    return math::vec4(channel(16), channel(8), channel(0), alpha);
}

Result<std::vector<TileGid>, std::string> decodeCsvTiles(std::string_view text,
                                                         size_t expectedCount)
{
    using TilesResult = Result<std::vector<TileGid>, std::string>;

    std::vector<TileGid> tiles;
    tiles.reserve(expectedCount);

    text = trim(text);
    while (!text.empty()) {
        const size_t comma = text.find(',');
        const std::string_view token = trim(text.substr(0, comma));

        TileGid tile = 0;
        const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), tile);
        if (token.empty() || error != std::errc() || end != token.data() + token.size()) {
            return TilesResult::Err(
                std::format("invalid tile \"{}\" at index {}", token, tiles.size()));
        }
        tiles.push_back(tile);

        if (comma == std::string_view::npos) {
            break;
        }
        text.remove_prefix(comma + 1);
    }

    if (tiles.size() != expectedCount) {
        return TilesResult::Err(
            std::format("expected {} tiles, got {}", expectedCount, tiles.size()));
    }
    return TilesResult::Ok(std::move(tiles));
}

Result<std::vector<TileGid>, std::string>
decodeBase64Tiles(std::string_view text, std::string_view compression, size_t expectedCount)
{
    using TilesResult = Result<std::vector<TileGid>, std::string>;

    std::string compact;
    compact.reserve(text.size());
    std::copy_if(text.begin(), text.end(), std::back_inserter(compact),
                 [](char c) { return !isSpace(c); });

    const std::optional<std::vector<uint8_t>> decoded = decodeBase64(compact);
    if (!decoded) {
        return TilesResult::Err("invalid base64");
    }

    const size_t expectedSize = expectedCount * sizeof(TileGid);
    if (expectedSize > static_cast<size_t>(std::numeric_limits<int>::max()) ||
        decoded->size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
        return TilesResult::Err("the layer is too large");
    }

    std::vector<uint8_t> bytes;
    if (compression.empty()) {
        bytes = std::move(*decoded);
    }
    else if (compression == "zlib" || compression == "gzip") {
        std::string_view stream(reinterpret_cast<const char*>(decoded->data()), decoded->size());
        if (compression == "gzip") {
            const std::optional<std::string_view> deflateStream = getGzipDeflateStream(stream);
            if (!deflateStream) {
                return TilesResult::Err("invalid gzip header");
            }
            stream = *deflateStream;
        }

        // One byte more than expected, so that too long data is detected
        bytes.resize(expectedSize + 1);
        const auto decode =
            compression == "zlib" ? &stbi_zlib_decode_buffer : &stbi_zlib_decode_noheader_buffer;
        const int size =
            decode(reinterpret_cast<char*>(bytes.data()), static_cast<int>(bytes.size()),
                   stream.data(), static_cast<int>(stream.size()));
        if (size < 0) {
            return TilesResult::Err(std::format("invalid or too long {} data", compression));
        }
        bytes.resize(static_cast<size_t>(size));
    }
    else {
        return TilesResult::Err(std::format("unsupported compression \"{}\"", compression));
    }

    if (bytes.size() != expectedSize) {
        return TilesResult::Err(
            std::format("expected {} tiles, got {} bytes", expectedCount, bytes.size()));
    }

    std::vector<TileGid> tiles(expectedCount);
    for (size_t i = 0; i < expectedCount; i++) {
        const uint8_t* tile = &bytes[i * 4];
        tiles[i] = static_cast<TileGid>(tile[0]) | (static_cast<TileGid>(tile[1]) << 8) |
                   (static_cast<TileGid>(tile[2]) << 16) | (static_cast<TileGid>(tile[3]) << 24);
    }
    return TilesResult::Ok(std::move(tiles));
}

Result<void, std::string> setLayerTiles(const TileMap& map, TileLayer& layer,
                                        std::vector<TileGid> tiles)
{
    using VoidResult = Result<void, std::string>;

    if (tiles.size() != layer.getTiles().size()) {
        return VoidResult::Err(
            std::format("expected {} tiles, got {}", layer.getTiles().size(), tiles.size()));
    }

    // Tile sets come before the layers in both formats, so they are all known here
    for (size_t i = 0; i < tiles.size(); i++) {
        if (getTileGid(tiles[i]) != 0 && map.findTileSet(tiles[i]) == nullptr) {
            return VoidResult::Err(
                std::format("tile {} at index {} is in no tile set", getTileGid(tiles[i]), i));
        }
    }

    layer.setTiles(std::move(tiles));
    return VoidResult::Ok();
}

void applyLayerAttributes(TileMapLayer& layer, const LayerGroup& group, uint32_t id, bool visible,
                          float opacity, const math::vec2& offset)
{
    layer.setId(id);
    layer.setVisible(group.visible && visible);
    layer.setOpacity(group.opacity * opacity);
    layer.setOffset(group.offset + offset);
}

} // namespace ocf::tiled
