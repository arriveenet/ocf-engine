// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <cstdint>

namespace ocf::audio::tracker {

/** Channels of the NES APU (2A03), in the order used by songs and the file format. */
enum class ChannelType : uint8_t {
    Pulse1,
    Pulse2,
    Triangle,
    Noise,
    Dpcm,
};

constexpr size_t CHANNEL_COUNT = 5;

// Note values. 1..96 are C-0..B-7. On the noise channel, 1..16 are the noise periods 0..F
constexpr uint8_t NOTE_NONE = 0;
constexpr uint8_t NOTE_MIN = 1;
constexpr uint8_t NOTE_MAX = 96;
constexpr uint8_t NOISE_NOTE_MAX = 16;
constexpr uint8_t NOTE_RELEASE = 0xFD; // "===": continue the instrument from its release point
constexpr uint8_t NOTE_OFF = 0xFE;     // "---": silence the channel

// Empty instrument or volume column
constexpr uint8_t VALUE_NONE = 0xFF;

constexpr uint8_t MAX_VOLUME = 15;

constexpr uint8_t DEFAULT_SPEED = 6;
constexpr uint8_t MIN_SPEED = 1;
constexpr uint8_t MAX_SPEED = 31;
constexpr uint8_t DEFAULT_TEMPO = 150;
constexpr uint8_t MIN_TEMPO = 32;
constexpr uint8_t MAX_TEMPO = 255;

constexpr uint16_t DEFAULT_ROWS_PER_PATTERN = 64;
constexpr size_t MAX_ROWS = 256;
constexpr size_t MAX_FRAMES = 256;
constexpr size_t MAX_PATTERNS = 256; // Per channel
constexpr size_t MAX_INSTRUMENTS = 64;
constexpr size_t MAX_DPCM_SAMPLES = 64;
constexpr size_t MAX_EFFECT_COLUMNS = 4;
constexpr size_t MAX_SEQUENCE_LENGTH = 256;

enum class EffectType : uint8_t {
    None,
    Speed, // Fxx: xx < 0x20 sets the speed, otherwise the tempo
    Jump,  // Bxx: after this row, jump to frame xx
    Skip,  // Dxx: after this row, go to row xx of the next frame
    Halt,  // Cxx: after this row, stop the song
};

enum class SequenceType : uint8_t {
    Volume, // 0..15
    Duty,   // Pulse: duty cycle 0..3, noise: mode in bit 0
};

constexpr size_t SEQUENCE_TYPE_COUNT = 2;

struct PlaybackStatus {
    bool playing = false;
    uint16_t frame = 0;
    uint16_t row = 0;
};

constexpr size_t toIndex(ChannelType channel)
{
    return static_cast<size_t>(channel);
}

constexpr size_t toIndex(SequenceType type)
{
    return static_cast<size_t>(type);
}

} // namespace ocf::audio::tracker
