// SPDX-License-Identifier: MIT
#pragma once
#include "ocf/audio/apu/DmcChannel.h"
#include "ocf/audio/apu/FrameSequencer.h"
#include "ocf/audio/apu/NoiseChannel.h"
#include "ocf/audio/apu/PulseChannel.h"
#include "ocf/audio/apu/TriangleChannel.h"

#include <span>
#include <stdint.h>

namespace ocf::audio {

class Apu {
public:
    Apu();

    // Clock the APU
    void clock();

    // Write registers
    void writeRegister(uint16_t address, uint8_t data);

    // Status register
    uint8_t readStatusRegister();
    void writeStatusRegister(uint8_t data);

    // Output sample
    float getOutput();

    /** The DMC's samples at $C000-$FFFF. Copied, at most 16KB. The rest is zero. */
    void setDmcMemory(std::span<const uint8_t> data);

    /** Copies to $C000 + offset, keeping the rest of the memory. */
    void writeDmcMemory(size_t offset, std::span<const uint8_t> data);

private:
    void clockFrameCounterQuarterFrame();
    void clockFrameCounterHalfFrame();

    uint8_t m_statusRegister = 0;
    uint32_t cycles = 0;
    uint32_t counter = 0;

    // Frame sequencer
    FrameSequencer m_frameSequencer;

    // Channels
    PulseChannel m_pulseChannel[2] = {PulseChannel(true), PulseChannel(false)};
    TriangleChannel m_triangleChannel;
    NoiseChannel m_noiseChannel;
    DmcChannel m_dmcChannel;

    friend FrameSequencer;
};

} // namespace ocf::audio
