// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

/**
 * The standard base64 of RFC 4648, with padding.
 */
namespace ocf {

std::string encodeBase64(std::span<const uint8_t> data);

/** std::nullopt when the text is not base64: a wrong length, character or padding. */
std::optional<std::vector<uint8_t>> decodeBase64(std::string_view text);

} // namespace ocf
