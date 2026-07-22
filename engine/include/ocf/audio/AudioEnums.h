#pragma once

#include <cstdint>

namespace ocf {
namespace audio {

// Audio handle type
using AudioHandle = uint32_t;

// Invalid audio handle constant
constexpr AudioHandle INVALID_AUDIO_HANDLE = UINT32_MAX;

enum class AudioFormat : uint8_t {
    Unknown, // Unknown format
    U8,      // Unsigned 8-bit PCM
    S16,     // Signed 16-bit PCM
    S24,     // Signed 24-bit PCM
    S32,     // Signed 32-bit PCM
    F32,     // 32-bit float PCM
};

} // namespace audio
} // namespace ocf
