// SPDX-License-Identifier: MIT
#include "ocf/core/base64.h"

#include <algorithm>

namespace ocf {

namespace {

constexpr char ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int getValue(char c)
{
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 26;
    }
    if (c >= '0' && c <= '9') {
        return c - '0' + 52;
    }
    if (c == '+') {
        return 62;
    }
    if (c == '/') {
        return 63;
    }
    return -1;
}

} // namespace

std::string encodeBase64(std::span<const uint8_t> data)
{
    std::string text;
    text.reserve((data.size() + 2) / 3 * 4);
    for (size_t i = 0; i < data.size(); i += 3) {
        const size_t count = std::min<size_t>(3, data.size() - i);
        uint32_t group = static_cast<uint32_t>(data[i]) << 16;
        if (count > 1) {
            group |= static_cast<uint32_t>(data[i + 1]) << 8;
        }
        if (count > 2) {
            group |= data[i + 2];
        }
        text += ALPHABET[(group >> 18) & 0x3F];
        text += ALPHABET[(group >> 12) & 0x3F];
        text += (count > 1) ? ALPHABET[(group >> 6) & 0x3F] : '=';
        text += (count > 2) ? ALPHABET[group & 0x3F] : '=';
    }
    return text;
}

std::optional<std::vector<uint8_t>> decodeBase64(std::string_view text)
{
    if (text.size() % 4 != 0) {
        return std::nullopt;
    }
    std::vector<uint8_t> data;
    data.reserve(text.size() / 4 * 3);
    for (size_t i = 0; i < text.size(); i += 4) {
        // Padding is only at the end, in the last one or two characters
        const bool last = (i + 4 == text.size());
        uint32_t group = 0;
        int padding = 0;
        for (size_t j = 0; j < 4; j++) {
            const char c = text[i + j];
            int value = 0;
            if (c == '=' && last && j >= 2) {
                padding++;
            }
            else {
                value = getValue(c);
                if (value < 0 || padding > 0) {
                    return std::nullopt;
                }
            }
            group = (group << 6) | static_cast<uint32_t>(value);
        }
        data.push_back(static_cast<uint8_t>(group >> 16));
        if (padding < 2) {
            data.push_back(static_cast<uint8_t>(group >> 8));
        }
        if (padding < 1) {
            data.push_back(static_cast<uint8_t>(group));
        }
    }
    return data;
}

} // namespace ocf
