// SPDX-License-Identifier: MIT
#include "ocf/core/TextUtility.h"

#include <cstdint>

namespace ocf {

static constexpr char32_t REPLACEMENT_CHARACTER = U'�';

std::u32string TextUtility::utf8ToUtf32(std::string_view utf8)
{
    std::u32string result;
    result.reserve(utf8.size());

    const size_t length = utf8.size();
    size_t i = 0;

    while (i < length) {
        const uint8_t lead = static_cast<uint8_t>(utf8[i]);

        if (lead < 0x80) {
            result.push_back(lead);
            i++;
            continue;
        }

        int trailCount = 0;
        char32_t codePoint = 0;
        char32_t minCodePoint = 0;

        if ((lead & 0xE0) == 0xC0) {
            trailCount = 1;
            codePoint = lead & 0x1F;
            minCodePoint = 0x80;
        }
        else if ((lead & 0xF0) == 0xE0) {
            trailCount = 2;
            codePoint = lead & 0x0F;
            minCodePoint = 0x800;
        }
        else if ((lead & 0xF8) == 0xF0) {
            trailCount = 3;
            codePoint = lead & 0x07;
            minCodePoint = 0x10000;
        }
        else {
            // Stray continuation byte or an invalid lead byte
            result.push_back(REPLACEMENT_CHARACTER);
            i++;
            continue;
        }

        size_t next = i + 1;
        bool valid = true;
        for (int n = 0; n < trailCount; n++, next++) {
            if (next >= length || (static_cast<uint8_t>(utf8[next]) & 0xC0) != 0x80) {
                valid = false;
                break;
            }
            codePoint = (codePoint << 6) | (static_cast<uint8_t>(utf8[next]) & 0x3F);
        }

        // Resume after the bytes consumed so far so a truncated sequence does not swallow
        // the following character
        i = next;

        if (!valid || codePoint < minCodePoint || codePoint > 0x10FFFF ||
            (codePoint >= 0xD800 && codePoint <= 0xDFFF)) {
            result.push_back(REPLACEMENT_CHARACTER);
            continue;
        }

        result.push_back(codePoint);
    }

    return result;
}

} // namespace ocf
