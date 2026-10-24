// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/compiler.h"
#include "ocf/audio/AudioEnums.h"
#include "ocf/audio/tracker/TrackerTypes.h"

#include <cstdint>
#include <memory>
#include <string_view>

namespace ocf {
namespace audio {

namespace tracker {
struct DpcmKey;
struct Song;
}

class OCF_API AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    bool initialize();

    void shutdown();

    void update();

    AudioBufferHandle createStreamBuffer(std::string_view filename);

    AudioSourceHandle createSource(AudioBufferHandle bufferHandle);

    AudioSourceHandle createNesApuSource();

    void destroyBuffer(AudioBufferHandle handle);

    void destroySource(AudioSourceHandle handle);

    void play(AudioSourceHandle handle, bool loop = false, float volume = 1.0f);

    void stop(AudioSourceHandle handle);

    void pause(AudioSourceHandle handle);

    AudioState getState(AudioSourceHandle handle) const;

    float getVolume(AudioSourceHandle handle) const;

    void setVolume(AudioSourceHandle handle, float volume);

    bool isLoop(AudioSourceHandle handle) const;

    void setLoop(AudioSourceHandle handle, bool loop);

    bool isInitialized() const noexcept { return m_initialized; }

    // APU specific functions
    void apu_writeRegister(AudioSourceHandle handle, uint16_t address, uint8_t data);

    uint8_t apu_readStatusRegister(AudioSourceHandle handle);

    void apu_writeStatusRegister(AudioSourceHandle handle, uint8_t data);

    // Tracker specific functions. The song plays on its own APU, on the audio thread
    AudioSourceHandle createTrackerSource();

    /** The song is shared with the audio thread, so it must not be changed afterwards. */
    void tracker_setSong(AudioSourceHandle handle, std::shared_ptr<const tracker::Song> song);

    void tracker_play(AudioSourceHandle handle, uint16_t frame = 0, uint16_t row = 0);

    void tracker_stop(AudioSourceHandle handle);

    tracker::PlaybackStatus tracker_getStatus(AudioSourceHandle handle) const;

    void tracker_playNote(AudioSourceHandle handle, tracker::ChannelType channel, uint8_t note,
                          uint8_t instrument);

    void tracker_stopNote(AudioSourceHandle handle, tracker::ChannelType channel);

    /** Plays a DPCM sample of the song without an instrument, for previews. */
    void tracker_playDpcmSample(AudioSourceHandle handle, const tracker::DpcmKey& key);

    void tracker_setChannelMuted(AudioSourceHandle handle, tracker::ChannelType channel,
                                 bool muted);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    bool m_initialized = false;
    uint32_t m_HandleCounter = 0;
};

} // namespace audio
} // namespace ocf
