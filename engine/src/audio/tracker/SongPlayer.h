// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/audio/tracker/Song.h"

#include <array>
#include <functional>
#include <memory>
#include <optional>

namespace ocf::audio::tracker {

/**
 * Plays a Song by writing APU registers. tick() must be called at the NTSC frame rate (about
 * 60 Hz). Not thread safe: every call must come from the thread that clocks the APU.
 */
class SongPlayer {
public:
    using RegisterWriter = std::function<void(uint16_t address, uint8_t data)>;

    explicit SongPlayer(RegisterWriter writer);

    /** Replaces the song, keeping the position where possible. Returns the previous song. */
    std::shared_ptr<const Song> setSong(std::shared_ptr<const Song> song);

    const Song* getSong() const noexcept { return m_song.get(); }

    /** Plays from the given position. The first row is played on the next tick. */
    void play(uint16_t frame, uint16_t row);

    /** Stops the song and silences every channel. */
    void stop();

    /** Plays a note outside of the song, for previews. Works while the song is stopped. */
    void playNote(ChannelType channel, uint8_t note, uint8_t instrument);

    void stopNote(ChannelType channel);

    /** Plays a sample on the DPCM channel without a key, for previews. stopNote() stops it. */
    void playDpcmSample(const DpcmKey& key);

    void setChannelMuted(ChannelType channel, bool muted);

    bool isChannelMuted(ChannelType channel) const { return m_channels[toIndex(channel)].muted; }

    void tick();

    PlaybackStatus getStatus() const noexcept { return {m_playing, m_frame, m_row}; }

private:
    struct ChannelState {
        uint8_t note = NOTE_NONE;
        uint8_t instrument = VALUE_NONE;
        uint8_t volume = MAX_VOLUME; // Volume column
        bool active = false;         // A note is playing
        bool released = false;
        bool triggered = false; // The note started this tick
        bool muted = false;
        bool audible = false;  // Triangle: sounding after the last tick. DPCM: a sample started
        bool dpcmLoop = false; // DPCM: the sample loops
        uint8_t dpcmRate = 0;  // DPCM: the rate of the sample
        std::optional<DpcmKey> dpcmPreview; // DPCM: played instead of the instrument's key
        int periodHigh = -1;   // Last value written to the period high bits
        std::array<uint16_t, SEQUENCE_TYPE_COUNT> sequencePositions{};
    };

    void advancePosition();
    void processRow();
    void applyCell(ChannelState& state, const Cell& cell);
    void triggerNote(ChannelState& state, uint8_t note);
    void releaseNote(ChannelState& state);

    int8_t stepSequence(ChannelState& state, SequenceType type, int8_t defaultValue);
    void updateChannel(ChannelType channel);
    void writePulse(ChannelType channel, uint8_t volume, uint8_t duty);
    void writeTriangle(uint8_t volume);
    void writeNoise(uint8_t volume, uint8_t duty);
    void writeDpcm();
    void startDpcm(ChannelState& state);
    void stopDpcm(ChannelState& state);

    RegisterWriter m_writer;
    std::shared_ptr<const Song> m_song;
    std::array<ChannelState, CHANNEL_COUNT> m_channels;
    bool m_apuInitialized = false;

    bool m_playing = false;
    bool m_firstRow = false; // The next row to play is m_frame / m_row itself
    uint16_t m_frame = 0;
    uint16_t m_row = 0;
    uint8_t m_speed = DEFAULT_SPEED;
    uint8_t m_tempo = DEFAULT_TEMPO;
    int m_tempoAccumulator = 0;

    // Set by Bxx / Dxx / Cxx, applied when moving to the next row
    int m_jumpFrame = -1;
    int m_skipRow = -1;
    bool m_halt = false;
};

} // namespace ocf::audio::tracker
