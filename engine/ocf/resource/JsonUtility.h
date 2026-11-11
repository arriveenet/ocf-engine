// SPDX-License-Identifier: MIT
#pragma once

// Internal to the engine: yyjson is a private dependency, so do not include from public headers.

#include "ocf/core/Result.h"

#include <yyjson.h>

#include <memory>
#include <string>
#include <string_view>

namespace ocf::json {

struct DocumentDeleter {
    void operator()(yyjson_doc* document) const { yyjson_doc_free(document); }
};

using Document = std::unique_ptr<yyjson_doc, DocumentDeleter>;

/**
 * @brief Parse JSON text. A UTF-8 BOM is accepted.
 *
 * @return The document, or an error with the line and column of the problem.
 */
Result<Document, std::string> parse(std::string_view text);

/** @brief The string of a string value, which may contain null characters. */
inline std::string_view getString(yyjson_val* value)
{
    return std::string_view(yyjson_get_str(value), yyjson_get_len(value));
}

/** @brief "path/key", or "key" for the root. Used to point at a value in error messages. */
std::string memberPath(std::string_view path, std::string_view key);

} // namespace ocf::json
