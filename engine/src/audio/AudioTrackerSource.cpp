// SPDX-License-Identifier: MIT
#include "audio/AudioTrackerSource.h"

#include "ocf/audio/AudioEnums.h"

namespace ocf::audio {

namespace {

constexpr uint32_t STATUS_PLAYING_BIT = 0x80000000u;

uint32_t packStatus(const tracker::PlaybackStatus& status)
{
    return (status.playing ? STATUS_PLAYING_BIT : 0u) | (static_cast<uint32_t>(status.frame) << 16) |
           status.row;
}

} // namespace

AudioTrackerSource::AudioTrackerSource()
    : m_synthesizer(InternalSampleRate)
{
    // Avoid allocating on the audio thread in the common case
    m_pendingCommands.reserve(64);
    m_processingCommands.reserve(64);
    m_retiredSongs.reserve(16);
}

AudioTrackerSource::~AudioTrackerSource()
{
}

void AudioTrackerSource::update()
{
    std::vector<std::shared_ptr<const tracker::Song>> retiredSongs;
    {
        std::lock_guard<std::mutex> lock(m_retiredMutex);
        if (m_retiredSongs.empty()) {
            return;
        }
        retiredSongs.reserve(m_retiredSongs.capacity());
        std::swap(retiredSongs, m_retiredSongs);
    }
    // The songs are freed here, on the main thread
}

void AudioTrackerSource::render(float* output, uint32_t frameCount, uint32_t channels)
{
    processCommands();

    for (uint32_t i = 0; i < frameCount; i++) {
        const float sample = m_synthesizer.renderSample();
        for (uint32_t c = 0; c < channels; c++) {
            output[i * channels + c] = sample;
        }
    }

    m_status.store(packStatus(m_synthesizer.getPlayer().getStatus()), std::memory_order_relaxed);
}

void AudioTrackerSource::setSong(std::shared_ptr<const tracker::Song> song)
{
    Command command;
    command.type = Command::Type::SetSong;
    command.song = std::move(song);
    pushCommand(std::move(command));
}

void AudioTrackerSource::playSong(uint16_t frame, uint16_t row)
{
    Command command;
    command.type = Command::Type::PlaySong;
    command.frame = frame;
    command.row = row;
    pushCommand(std::move(command));
}

void AudioTrackerSource::stopSong()
{
    Command command;
    command.type = Command::Type::StopSong;
    pushCommand(std::move(command));
}

void AudioTrackerSource::playNote(tracker::ChannelType channel, uint8_t note, uint8_t instrument)
{
    Command command;
    command.type = Command::Type::PlayNote;
    command.channel = channel;
    command.note = note;
    command.instrument = instrument;
    pushCommand(std::move(command));
}

void AudioTrackerSource::stopNote(tracker::ChannelType channel)
{
    Command command;
    command.type = Command::Type::StopNote;
    command.channel = channel;
    pushCommand(std::move(command));
}

void AudioTrackerSource::playDpcmSample(const tracker::DpcmKey& key)
{
    Command command;
    command.type = Command::Type::PlayDpcmSample;
    command.dpcmKey = key;
    pushCommand(std::move(command));
}

void AudioTrackerSource::setChannelMuted(tracker::ChannelType channel, bool muted)
{
    Command command;
    command.type = Command::Type::SetMuted;
    command.channel = channel;
    command.muted = muted;
    pushCommand(std::move(command));
}

tracker::PlaybackStatus AudioTrackerSource::getStatus() const
{
    const uint32_t status = m_status.load(std::memory_order_relaxed);
    tracker::PlaybackStatus result;
    result.playing = (status & STATUS_PLAYING_BIT) != 0;
    result.frame = static_cast<uint16_t>((status >> 16) & 0x7FFF);
    result.row = static_cast<uint16_t>(status & 0xFFFF);
    return result;
}

void AudioTrackerSource::pushCommand(Command command)
{
    std::lock_guard<std::mutex> lock(m_commandMutex);
    m_pendingCommands.push_back(std::move(command));
}

void AudioTrackerSource::processCommands()
{
    {
        std::lock_guard<std::mutex> lock(m_commandMutex);
        std::swap(m_pendingCommands, m_processingCommands);
    }

    tracker::SongPlayer& player = m_synthesizer.getPlayer();
    for (Command& command : m_processingCommands) {
        switch (command.type) {
        case Command::Type::SetSong: {
            std::shared_ptr<const tracker::Song> previous =
                m_synthesizer.setSong(std::move(command.song));
            if (previous) {
                std::lock_guard<std::mutex> lock(m_retiredMutex);
                m_retiredSongs.push_back(std::move(previous));
            }
            break;
        }
        case Command::Type::PlaySong: player.play(command.frame, command.row); break;
        case Command::Type::PlayDpcmSample: player.playDpcmSample(command.dpcmKey); break;
        case Command::Type::StopSong: player.stop(); break;
        case Command::Type::PlayNote:
            player.playNote(command.channel, command.note, command.instrument);
            break;
        case Command::Type::StopNote: player.stopNote(command.channel); break;
        case Command::Type::SetMuted:
            player.setChannelMuted(command.channel, command.muted);
            break;
        }
    }
    m_processingCommands.clear();
}

} // namespace ocf::audio
