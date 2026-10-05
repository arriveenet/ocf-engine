// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>

namespace ocf {
namespace audio {

class AudioBuffer;
class AudioSource;

using HandleId = uint32_t;

template <typename T>
struct Handle {
    constexpr static uint32_t InvalidHandle = UINT32_MAX;

    Handle() : m_handleId(InvalidHandle) {}
    Handle(HandleId id)
        : m_handleId(id)
    {
    }

    bool operator==(const Handle<T>& other) const { return m_handleId == other.m_handleId; }
    bool operator!=(const Handle<T>& other) const { return m_handleId != other.m_handleId; }
    explicit operator bool() const { return m_handleId != InvalidHandle; }

    HandleId getId() const noexcept{ return m_handleId; }

    void reset() { m_handleId = InvalidHandle; }

private:
    HandleId m_handleId;
};

using AudioBufferHandle = Handle<AudioBuffer>;
using AudioSourceHandle = Handle<AudioSource>;

enum class AudioFormat : uint8_t {
    Unknown, // Unknown format
    U8,      // Unsigned 8-bit PCM
    S16,     // Signed 16-bit PCM
    S24,     // Signed 24-bit PCM
    S32,     // Signed 32-bit PCM
    F32,     // 32-bit float PCM
};

enum class AudioState : uint8_t {
    Initial, // Initial state, not yet started
    Playing, // Currently playing audio
    Paused,  // Currently paused audio
    Stopped  // Audio has been stopped
};

// Internal audio system sample rate (48 kHz)
constexpr uint32_t InternalSampleRate = 48000;

// Internal audio system channel count (stereo)
constexpr uint32_t InternalChannels = 2;

// Internal audio system format
constexpr AudioFormat InternalFormat = AudioFormat::F32;

} // namespace audio
} // namespace ocf
