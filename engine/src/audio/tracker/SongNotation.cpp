// SPDX-License-Identifier: MIT
#include "ocf/audio/tracker/SongNotation.h"

#include <array>
#include <charconv>
#include <format>

namespace ocf::audio::tracker {

namespace {

constexpr std::array<std::string_view, 12> NOTE_NAMES = {
    "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-",
};

constexpr std::array<std::string_view, CHANNEL_COUNT> CHANNEL_NAMES = {
    "pulse1", "pulse2", "triangle", "noise", "dpcm",
};

constexpr std::array<std::string_view, SEQUENCE_TYPE_COUNT> SEQUENCE_NAMES = {
    "volume",
    "duty",
};

constexpr char HEX_DIGITS[] = "0123456789ABCDEF";

std::optional<uint8_t> hexDigitValue(char c)
{
    if (c >= '0' && c <= '9') {
        return static_cast<uint8_t>(c - '0');
    }
    if (c >= 'A' && c <= 'F') {
        return static_cast<uint8_t>(c - 'A' + 10);
    }
    if (c >= 'a' && c <= 'f') {
        return static_cast<uint8_t>(c - 'a' + 10);
    }
    return std::nullopt;
}

} // namespace

std::string noteToString(ChannelType channel, uint8_t note)
{
    if (note == NOTE_NONE) {
        return "...";
    }
    if (note == NOTE_OFF) {
        return "---";
    }
    if (note == NOTE_RELEASE) {
        return "===";
    }

    if (channel == ChannelType::Noise) {
        if (note > NOISE_NOTE_MAX) {
            return "???";
        }
        return std::format("{}-#", HEX_DIGITS[note - 1]);
    }

    if (note > NOTE_MAX) {
        return "???";
    }
    const int semitone = note - 1;
    return std::format("{}{}", NOTE_NAMES[semitone % 12], semitone / 12);
}

std::optional<uint8_t> noteFromString(ChannelType channel, std::string_view text)
{
    if (text == "...") {
        return NOTE_NONE;
    }
    if (text == "---") {
        return NOTE_OFF;
    }
    if (text == "===") {
        return NOTE_RELEASE;
    }
    if (text.size() != 3) {
        return std::nullopt;
    }

    if (channel == ChannelType::Noise) {
        const std::optional<uint8_t> period = hexDigitValue(text[0]);
        if (!period || text.substr(1) != "-#") {
            return std::nullopt;
        }
        return static_cast<uint8_t>(*period + 1);
    }

    const char octave = text[2];
    if (octave < '0' || octave > '7') {
        return std::nullopt;
    }
    for (size_t i = 0; i < NOTE_NAMES.size(); i++) {
        if (text.substr(0, 2) == NOTE_NAMES[i]) {
            return static_cast<uint8_t>((octave - '0') * 12 + i + 1);
        }
    }
    return std::nullopt;
}

char effectTypeToLetter(EffectType type)
{
    switch (type) {
    case EffectType::Speed: return 'F';
    case EffectType::Jump:  return 'B';
    case EffectType::Skip:  return 'D';
    case EffectType::Halt:  return 'C';
    default:                return '.';
    }
}

std::optional<EffectType> effectTypeFromLetter(char letter)
{
    switch (letter) {
    case 'F': return EffectType::Speed;
    case 'B': return EffectType::Jump;
    case 'D': return EffectType::Skip;
    case 'C': return EffectType::Halt;
    default:  return std::nullopt;
    }
}

std::string effectToString(const Effect& effect)
{
    if (effect.type == EffectType::None) {
        return "...";
    }
    return std::format("{}{:02X}", effectTypeToLetter(effect.type), effect.param);
}

std::optional<Effect> effectFromString(std::string_view text)
{
    if (text == "...") {
        return Effect{};
    }
    if (text.size() != 3) {
        return std::nullopt;
    }

    const std::optional<EffectType> type = effectTypeFromLetter(text[0]);
    const std::optional<uint8_t> high = hexDigitValue(text[1]);
    const std::optional<uint8_t> low = hexDigitValue(text[2]);
    if (!type || !high || !low) {
        return std::nullopt;
    }
    return Effect{*type, static_cast<uint8_t>((*high << 4) | *low)};
}

std::string_view channelTypeToString(ChannelType channel)
{
    return CHANNEL_NAMES[toIndex(channel)];
}

std::optional<ChannelType> channelTypeFromString(std::string_view text)
{
    for (size_t i = 0; i < CHANNEL_NAMES.size(); i++) {
        if (text == CHANNEL_NAMES[i]) {
            return static_cast<ChannelType>(i);
        }
    }
    return std::nullopt;
}

std::string_view sequenceTypeToString(SequenceType type)
{
    return SEQUENCE_NAMES[toIndex(type)];
}

std::optional<SequenceType> sequenceTypeFromString(std::string_view text)
{
    for (size_t i = 0; i < SEQUENCE_NAMES.size(); i++) {
        if (text == SEQUENCE_NAMES[i]) {
            return static_cast<SequenceType>(i);
        }
    }
    return std::nullopt;
}

std::string sequenceToMml(const Sequence& sequence)
{
    std::string mml;
    for (size_t i = 0; i < sequence.values.size(); i++) {
        if (!mml.empty()) {
            mml += ' ';
        }
        if (static_cast<int>(i) == sequence.loopPoint) {
            mml += "| ";
        }
        if (static_cast<int>(i) == sequence.releasePoint) {
            mml += "/ ";
        }
        mml += std::to_string(sequence.values[i]);
    }
    return mml;
}

std::optional<Sequence> sequenceFromMml(std::string_view text)
{
    Sequence sequence;
    size_t pos = 0;
    while (pos < text.size()) {
        const char c = text[pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            pos++;
            continue;
        }

        if (c == '|' || c == '/') {
            int16_t& point = (c == '|') ? sequence.loopPoint : sequence.releasePoint;
            if (point != -1) {
                return std::nullopt; // Marker given twice
            }
            point = static_cast<int16_t>(sequence.values.size());
            pos++;
            continue;
        }

        int value = 0;
        const char* begin = text.data() + pos;
        const char* end = text.data() + text.size();
        const auto [next, error] = std::from_chars(begin, end, value);
        if (error != std::errc() || value < INT8_MIN || value > INT8_MAX) {
            return std::nullopt;
        }
        if (sequence.values.size() >= MAX_SEQUENCE_LENGTH) {
            return std::nullopt;
        }
        sequence.values.push_back(static_cast<int8_t>(value));
        pos += static_cast<size_t>(next - begin);
    }

    // A marker must come before a value
    const auto size = static_cast<int16_t>(sequence.values.size());
    if (sequence.loopPoint >= size || sequence.releasePoint >= size) {
        return std::nullopt;
    }
    return sequence;
}

} // namespace ocf::audio::tracker
