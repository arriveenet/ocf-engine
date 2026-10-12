// SPDX-License-Identifier: MIT
#pragma once

#include <ocf/audio/AudioSystem.h>
#include <ocf/audio/tracker/Song.h>

#include <memory>

namespace ocf::tracker_tool {

/** Owns the tracker audio source. Must be destroyed before the audio system. */
class TrackerAudio {
public:
    explicit TrackerAudio(audio::AudioSystem& audioSystem)
        : m_audioSystem(audioSystem)
        , m_source(audioSystem.createTrackerSource())
    {
        // The source always plays: a stopped song is silent, and notes can still be previewed
        m_audioSystem.play(m_source);
    }

    ~TrackerAudio()
    {
        m_audioSystem.stop(m_source);
        m_audioSystem.destroySource(m_source);
    }

    TrackerAudio(const TrackerAudio&) = delete;
    TrackerAudio& operator=(const TrackerAudio&) = delete;

    /** The player gets its own copy, so the editor may keep changing the song. */
    void setSong(const audio::tracker::Song& song)
    {
        m_audioSystem.tracker_setSong(m_source, std::make_shared<const audio::tracker::Song>(song));
    }

    void playSong(int frame, int row)
    {
        m_audioSystem.tracker_play(m_source, static_cast<uint16_t>(frame), static_cast<uint16_t>(row));
    }

    void stopSong() { m_audioSystem.tracker_stop(m_source); }

    void playNote(audio::tracker::ChannelType channel, uint8_t note, uint8_t instrument)
    {
        m_audioSystem.tracker_playNote(m_source, channel, note, instrument);
    }

    void stopNote(audio::tracker::ChannelType channel)
    {
        m_audioSystem.tracker_stopNote(m_source, channel);
    }

    void setChannelMuted(audio::tracker::ChannelType channel, bool muted)
    {
        m_audioSystem.tracker_setChannelMuted(m_source, channel, muted);
    }

    audio::tracker::PlaybackStatus getStatus() const
    {
        return m_audioSystem.tracker_getStatus(m_source);
    }

private:
    audio::AudioSystem& m_audioSystem;
    audio::AudioSourceHandle m_source;
};

} // namespace ocf::tracker_tool
