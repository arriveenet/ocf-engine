// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>

namespace ocf::audio::tracker {

/** NTSC timer period of a pulse channel for a note (1..96, clamped). */
uint16_t getPulsePeriod(uint8_t note);

/** NTSC timer period of the triangle channel for a note (1..96, clamped). */
uint16_t getTrianglePeriod(uint8_t note);

/** Noise period index ($400E bits 0-3) for a noise note (1..16 = "0-#".."F-#"). */
uint8_t getNoisePeriodIndex(uint8_t note);

} // namespace ocf::audio::tracker
