// SPDX-License-Identifier: MIT
#include "ocf/resource/JsonUtility.h"

#include <format>

namespace ocf::json {

Result<Document, std::string> parse(std::string_view text)
{
    // A UTF-8 BOM is accepted, since some editors add one
    const yyjson_read_flag flags = YYJSON_READ_ALLOW_BOM | YYJSON_READ_ALLOW_INVALID_UNICODE;
    yyjson_read_err error{};
    // The text is not modified without YYJSON_READ_INSITU
    Document document(
        yyjson_read_opts(const_cast<char*>(text.data()), text.size(), flags, nullptr, &error));
    if (!document) {
        size_t line = 0;
        size_t column = 0;
        size_t character = 0;
        if (yyjson_locate_pos(text.data(), text.size(), error.pos, &line, &column, &character)) {
            return Result<Document, std::string>::Err(
                std::format("invalid JSON at line {}, column {}: {}", line, column, error.msg));
        }
        return Result<Document, std::string>::Err(std::format("invalid JSON: {}", error.msg));
    }
    return Result<Document, std::string>::Ok(std::move(document));
}

std::string memberPath(std::string_view path, std::string_view key)
{
    return path.empty() ? std::string(key) : std::format("{}/{}", path, key);
}

} // namespace ocf::json
