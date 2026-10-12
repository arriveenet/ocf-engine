// SPDX-License-Identifier: MIT
#include "audio/AudioTrackerSource.h"

#include "audio/apu/Apu.h"

#include <algorithm>

namespace ocf::audio {

namespace {

constexpr uint16_t STATUS_REGISTER = 0x4015;

// The APU output is always positive. This first order high-pass filter (about 38 Hz at 48 kHz)
// removes the DC offset, which would otherwise click when the song starts and stops
constexpr float HIGH_PASS_COEFFICIENT = 0.995f;

constexpr uint32_t STATUS_PLAYING_BIT = 0x80000000u;

uint32_t packStatus(const tracker::PlaybackStatus& status)
{
    return (status.playing ? STATUS_PLAYING_BIT : 0u) | (static_cast<uint32_t>(status.frame) << 16) |
           status.row;
}

} // namespace

AudioTrackerSource::AudioTrackerSource()
    : m_apu(std::make_unique<Apu>())
    , m_player([this](uint16_t address, uint8_t data) { writeRegister(address, data); })
{
    // The triangle rests at a non-zero level. Starting the filter there avoids a click
    m_previousInput = m_apu->getOutput();

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

    const double cyclesPerSample = AudioNesApu::CPU_FREQUENCY / static_cast<double>(InternalSampleRate);
    for (uint32_t i = 0; i < frameCount; i++) {
        m_cpuRemainder += cyclesPerSample;
        while (m_cpuRemainder >= 1.0) {
            m_tickRemainder -= 1.0;
            if (m_tickRemainder <= 0.0) {
                m_player.tick();
                m_tickRemainder += CYCLES_PER_TICK;
            }
            m_apu->clock();
            m_cpuRemainder -= 1.0;
        }

        const float input = m_apu->getOutput();
        const float filtered = input - m_previousInput + HIGH_PASS_COEFFICIENT * m_previousOutput;
        m_previousInput = input;
        m_previousOutput = filtered;

        const float sample = std::clamp(filtered, -1.0f, 1.0f);
        for (uint32_t c = 0; c < channels; c++) {
            output[i * channels + c] = sample;
        }
    }

    m_status.store(packStatus(m_player.getStatus()), std::memory_order_relaxed);
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

    for (Command& command : m_processingCommands) {
        switch (command.type) {
        case Command::Type::SetSong: {
            std::shared_ptr<const tracker::Song> previous = m_player.setSong(std::move(command.song));
            if (previous) {
                std::lock_guard<std::mutex> lock(m_retiredMutex);
                m_retiredSongs.push_back(std::move(previous));
            }
            break;
        }
        case Command::Type::PlaySong: m_player.play(command.frame, command.row); break;
        case Command::Type::StopSong: m_player.stop(); break;
        case Command::Type::PlayNote:
            m_player.playNote(command.channel, command.note, command.instrument);
            break;
        case Command::Type::StopNote: m_player.stopNote(command.channel); break;
        case Command::Type::SetMuted:
            m_player.setChannelMuted(command.channel, command.muted);
            break;
        }
    }
    m_processingCommands.clear();
}

void AudioTrackerSource::writeRegister(uint16_t address, uint8_t data)
{
    if (address == STATUS_REGISTER) {
        m_apu->writeStatusRegister(data);
    }
    else {
        m_apu->writeRegister(address, data);
    }
}

} // namespace ocf::audio
