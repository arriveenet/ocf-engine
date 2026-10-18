// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/audio/tracker/Song.h"

#include <optional>
#include <string>
#include <string_view>

/**
 * Text notation of song data, shared by the file format (docs/tracker-format.md) and the editor.
 * Writers always produce the canonical form; readers return std::nullopt for invalid text.
 */
namespace ocf::audio::tracker {

/** "C-4", "C#4", "0-#".."F-#" (noise), "---" (off), "===" (release), "..." (none). */
std::string noteToString(ChannelType channel, uint8_t note);
std::optional<uint8_t> noteFromString(ChannelType channel, std::string_view text);

/** Letter of the effect column ('F', 'B', ...), '.' for none. */
char effectTypeToLetter(EffectType type);
std::optional<EffectType> effectTypeFromLetter(char letter);

/** "F06", "..." (none). Hex digits are uppercase when written and either case when read. */
std::string effectToString(const Effect& effect);
std::optional<Effect> effectFromString(std::string_view text);

/** "pulse1", "pulse2", "triangle", "noise", "dpcm". */
std::string_view channelTypeToString(ChannelType channel);
std::optional<ChannelType> channelTypeFromString(std::string_view text);

/** "volume", "duty". */
std::string_view sequenceTypeToString(SequenceType type);
std::optional<SequenceType> sequenceTypeFromString(std::string_view text);

/**
 * FamiTracker style MML: "15 12 | 10 8 / 4 0". "|" comes before the loop point and "/" before
 * the release point.
 */
std::string sequenceToMml(const Sequence& sequence);
std::optional<Sequence> sequenceFromMml(std::string_view text);

} // namespace ocf::audio::tracker
