// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <string_view>

namespace ocf {

class TextUtility {
public:
    TextUtility() = delete;

    /**
     * @brief Convert a UTF-8 string to UTF-32.
     *
     * Malformed sequences (invalid lead/continuation bytes, overlong encodings, surrogates and
     * code points above U+10FFFF) are replaced with U+FFFD, one per offending byte sequence.
     */
    static std::u32string utf8ToUtf32(std::string_view utf8);
};

} // namespace ocf
