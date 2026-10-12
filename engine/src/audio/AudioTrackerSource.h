// SPDX-License-Identifier: MIT
#pragma once
#include "audio/AudioNesApu.h"
#include "audio/AudioSource.h"
#include "audio/tracker/SongPlayer.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace ocf::audio {

class Apu;

/**
 * Plays a tracker song on its own APU. The song functions may be called from the main thread:
 * they are queued and applied on the audio thread at the start of render(), so that every APU
 * register write happens on the audio thread.
 *
 * Stopping the song only silences it. The source itself keeps playing (and stays in the mixer)
 * so that notes can be previewed while the song is stopped.
 */
class AudioTrackerSource : public AudioSource {
public:
    // The player is ticked once per NTSC frame
    static constexpr double CYCLES_PER_TICK = AudioNesApu::CPU_FREQUENCY / 60.0988;

    AudioTrackerSource();
    ~AudioTrackerSource() override;

    void update() override;

    void render(float* output, uint32_t frameCount, uint32_t channels) override;

    void play() override { m_state = AudioState::Playing; }

    void pause() override { m_state = AudioState::Paused; }

    void stop() override { m_state = AudioState::Stopped; }

    void setSong(std::shared_ptr<const tracker::Song> song);

    void playSong(uint16_t frame, uint16_t row);

    void stopSong();

    void playNote(tracker::ChannelType channel, uint8_t note, uint8_t instrument);

    void stopNote(tracker::ChannelType channel);

    void setChannelMuted(tracker::ChannelType channel, bool muted);

    /** Status as of the last render(). */
    tracker::PlaybackStatus getStatus() const;

private:
    struct Command {
        enum class Type : uint8_t { SetSong, PlaySong, StopSong, PlayNote, StopNote, SetMuted };

        Type type = Type::StopSong;
        std::shared_ptr<const tracker::Song> song;
        tracker::ChannelType channel = tracker::ChannelType::Pulse1;
        uint16_t frame = 0;
        uint16_t row = 0;
        uint8_t note = tracker::NOTE_NONE;
        uint8_t instrument = tracker::VALUE_NONE;
        bool muted = false;
    };

    void pushCommand(Command command);

    // Audio thread
    void processCommands();
    void writeRegister(uint16_t address, uint8_t data);

    std::unique_ptr<Apu> m_apu;
    tracker::SongPlayer m_player;
    double m_cpuRemainder = 0.0;
    double m_tickRemainder = 0.0;
    float m_previousInput = 0.0f;
    float m_previousOutput = 0.0f;

    std::mutex m_commandMutex;
    std::vector<Command> m_pendingCommands;
    std::vector<Command> m_processingCommands; // Audio thread only

    // Songs replaced on the audio thread, freed on the main thread by update()
    std::mutex m_retiredMutex;
    std::vector<std::shared_ptr<const tracker::Song>> m_retiredSongs;

    std::atomic<uint32_t> m_status{0};
};

} // namespace ocf::audio
