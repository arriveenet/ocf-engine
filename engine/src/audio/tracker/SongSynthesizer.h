// SPDX-License-Identifier: MIT
#pragma once

#include "audio/tracker/SongPlayer.h"

#include <cstdint>
#include <memory>

namespace ocf::audio {
class Apu;
}

namespace ocf::audio::tracker {

/**
 * Turns a song into samples: owns an APU and the SongPlayer that drives it, clocks the APU at
 * the CPU rate, ticks the player once per NTSC frame, and removes the DC offset of the APU
 * output. Used both for real-time playback (AudioTrackerSource) and for exporting.
 */
class SongSynthesizer {
public:
    // The player is ticked once per NTSC frame
    static constexpr double TICK_RATE = 60.0988;

    explicit SongSynthesizer(uint32_t sampleRate);
    ~SongSynthesizer();

    SongSynthesizer(const SongSynthesizer&) = delete;
    SongSynthesizer& operator=(const SongSynthesizer&) = delete;

    SongPlayer& getPlayer() noexcept { return m_player; }
    const SongPlayer& getPlayer() const noexcept { return m_player; }

    /** Replaces the song and loads its DPCM samples into the APU. Returns the previous song. */
    std::shared_ptr<const Song> setSong(std::shared_ptr<const Song> song);

    /** Advances by one output sample and returns it (mono, -1.0 to 1.0). */
    float renderSample();

    uint32_t getSampleRate() const noexcept { return m_sampleRate; }

private:
    void loadDpcmSamples();
    void writeRegister(uint16_t address, uint8_t data);

    std::unique_ptr<Apu> m_apu;
    SongPlayer m_player;
    uint32_t m_sampleRate;
    double m_cyclesPerSample;
    double m_cpuRemainder = 0.0;
    double m_tickRemainder = 0.0;
    float m_highPassCoefficient;
    float m_previousInput = 0.0f;
    float m_previousOutput = 0.0f;
};

} // namespace ocf::audio::tracker
