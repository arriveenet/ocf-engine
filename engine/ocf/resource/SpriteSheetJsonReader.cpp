// SPDX-License-Identifier: MIT
#include "ocf/resource/SpriteSheetCommon.h"

#include "ocf/platform/FileSystem.h"
#include "ocf/resource/JsonUtility.h"

#include <yyjson.h>

#include <cstdint>
#include <format>

namespace ocf::spritesheet {

namespace {

using json::getString;
using json::memberPath;

/** Reads one sprite sheet, keeping the first error. Every function returns false on error. */
class JsonReader {
public:
    explicit JsonReader(std::filesystem::path baseDirectory)
        : m_baseDirectory(std::move(baseDirectory))
    {
    }

    Ref<SpriteSheet> read(yyjson_val* root);

    const std::string& getError() const { return m_error; }

private:
    bool fail(const std::string& path, std::string_view message);

    yyjson_val* getObject(yyjson_val* object, const char* key, const std::string& path,
                          bool required);
    bool readNumber(yyjson_val* object, const char* key, const std::string& path, float& result,
                    bool required);
    bool readBool(yyjson_val* object, const char* key, const std::string& path, bool& result);
    /** Reads members x and y (or w and h) of an optional object. */
    bool readPair(yyjson_val* object, const char* key, const char* first, const char* second,
                  const std::string& path, math::vec2& result);

    bool readMeta(yyjson_val* root, SpriteSheet& sheet);
    bool readFrame(yyjson_val* value, std::string name, const std::string& path,
                   SpriteSheet& sheet);

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

yyjson_val* JsonReader::getObject(yyjson_val* object, const char* key, const std::string& path,
                                  bool required)
{
    yyjson_val* member = yyjson_obj_get(object, key);
    if (member == nullptr) {
        if (required) {
            fail(memberPath(path, key), "is required");
        }
        return nullptr;
    }
    if (!yyjson_is_obj(member)) {
        fail(memberPath(path, key), "must be an object");
        return nullptr;
    }
    return member;
}

bool JsonReader::readNumber(yyjson_val* object, const char* key, const std::string& path,
                            float& result, bool required)
{
    yyjson_val* member = yyjson_obj_get(object, key);
    if (member == nullptr) {
        return required ? fail(memberPath(path, key), "is required") : true;
    }
    if (!yyjson_is_num(member)) {
        return fail(memberPath(path, key), "must be a number");
    }
    result = static_cast<float>(yyjson_get_num(member));
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
        return fail(memberPath(path, key), "must be a boolean");
    }
    result = yyjson_get_bool(member);
    return true;
}

bool JsonReader::readPair(yyjson_val* object, const char* key, const char* first,
                          const char* second, const std::string& path, math::vec2& result)
{
    yyjson_val* member = getObject(object, key, path, false);
    if (member == nullptr) {
        return m_error.empty();
    }
    const std::string pairPath = memberPath(path, key);
    return readNumber(member, first, pairPath, result.x, true) &&
           readNumber(member, second, pairPath, result.y, true);
}

Ref<SpriteSheet> JsonReader::read(yyjson_val* root)
{
    if (!yyjson_is_obj(root)) {
        fail("", "the root must be an object");
        return Ref<SpriteSheet>();
    }

    Ref<SpriteSheet> sheet(new SpriteSheet());
    if (!readMeta(root, *sheet.ptr())) {
        return Ref<SpriteSheet>();
    }

    yyjson_val* frames = yyjson_obj_get(root, "frames");
    if (frames == nullptr) {
        fail("frames", "is required");
        return Ref<SpriteSheet>();
    }

    if (yyjson_is_obj(frames)) {
        // JSON (Hash): the frames are keyed by name
        yyjson_obj_iter iter = yyjson_obj_iter_with(frames);
        yyjson_val* key = nullptr;
        while ((key = yyjson_obj_iter_next(&iter)) != nullptr) {
            std::string name(getString(key));
            const std::string path = memberPath("frames", name);
            if (!readFrame(yyjson_obj_iter_get_val(key), std::move(name), path, *sheet.ptr())) {
                return Ref<SpriteSheet>();
            }
        }
    } else if (yyjson_is_arr(frames)) {
        // JSON (Array): each frame has its name in "filename"
        size_t index = 0;
        size_t count = 0;
        yyjson_val* value = nullptr;
        yyjson_arr_foreach(frames, index, count, value)
        {
            const std::string path = std::format("frames/{}", index);
            yyjson_val* filename =
                yyjson_is_obj(value) ? yyjson_obj_get(value, "filename") : nullptr;
            if (filename == nullptr || !yyjson_is_str(filename)) {
                fail(memberPath(path, "filename"), "must be a string");
                return Ref<SpriteSheet>();
            }
            if (!readFrame(value, std::string(getString(filename)), path, *sheet.ptr())) {
                return Ref<SpriteSheet>();
            }
        }
    } else {
        fail("frames", "must be an object or an array");
        return Ref<SpriteSheet>();
    }

    return sheet;
}

bool JsonReader::readMeta(yyjson_val* root, SpriteSheet& sheet)
{
    yyjson_val* meta = getObject(root, "meta", "", true);
    if (meta == nullptr) {
        return false;
    }

    yyjson_val* image = yyjson_obj_get(meta, "image");
    if (image == nullptr || !yyjson_is_str(image) || yyjson_get_len(image) == 0) {
        return fail("meta/image", "must be a non-empty string");
    }

    math::vec2 size(0.0f, 0.0f);
    if (!readPair(meta, "size", "w", "h", "meta", size)) {
        return false;
    }
    if (size.x < 0.0f || size.y < 0.0f) {
        return fail("meta/size", "must not be negative");
    }

    sheet.setImage(pathToUtf8(resolveAssetPath(m_baseDirectory, getString(image))),
                   static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
    return true;
}

bool JsonReader::readFrame(yyjson_val* value, std::string name, const std::string& path,
                           SpriteSheet& sheet)
{
    if (!yyjson_is_obj(value)) {
        return fail(path, "must be an object");
    }

    SpriteFrame frame;
    frame.name = std::move(name);

    yyjson_val* rect = getObject(value, "frame", path, true);
    if (rect == nullptr) {
        return false;
    }
    const std::string rectPath = memberPath(path, "frame");
    math::vec2 position(0.0f, 0.0f);
    math::vec2 size(0.0f, 0.0f);
    if (!readNumber(rect, "x", rectPath, position.x, true) ||
        !readNumber(rect, "y", rectPath, position.y, true) ||
        !readNumber(rect, "w", rectPath, size.x, true) ||
        !readNumber(rect, "h", rectPath, size.y, true)) {
        return false;
    }
    // The size is the one of the image as drawn, also when it is stored rotated
    frame.textureRect = math::Rect(position, size);
    frame.sourceSize = size;

    bool trimmed = false;
    if (!readBool(value, "rotated", path, frame.rotated) ||
        !readBool(value, "trimmed", path, trimmed) ||
        !readPair(value, "spriteSourceSize", "x", "y", path, frame.offset) ||
        !readPair(value, "sourceSize", "w", "h", path, frame.sourceSize) ||
        !readPair(value, "pivot", "x", "y", path, frame.pivot)) {
        return false;
    }
    if (!trimmed) {
        // Untrimmed frames may still carry spriteSourceSize; it is the whole image then
        frame.offset = math::vec2(0.0f, 0.0f);
    }

    if (std::optional<std::string> error = validateFrame(frame)) {
        return fail(path, *error);
    }

    sheet.addFrame(std::move(frame));
    return true;
}

} // namespace

SheetResult readJson(std::string_view text, const std::filesystem::path& baseDirectory)
{
    Result<json::Document, std::string> document = json::parse(text);
    if (document.isErr()) {
        return SheetResult::Err(document.unwrapErr());
    }

    JsonReader reader(baseDirectory);
    Ref<SpriteSheet> sheet = reader.read(yyjson_doc_get_root(document.unwrap().get()));
    if (sheet.ptr() == nullptr) {
        return SheetResult::Err(reader.getError());
    }
    return SheetResult::Ok(std::move(sheet));
}

} // namespace ocf::spritesheet
