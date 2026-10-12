// SPDX-License-Identifier: MIT
#include "audio/tracker/SongPlayer.h"

#include "audio/tracker/NoteTable.h"

#include <algorithm>

namespace ocf::audio::tracker {

namespace {

// FamiTracker's tempo: a row is played each time the accumulator, which loses
// tempo * 24 / speed per tick, drops to zero. Speed 6 and tempo 150 give 6 ticks per row
constexpr int TICKS_PER_SECOND = 60;
constexpr int TEMPO_ACCUMULATOR_RELOAD = 60 * TICKS_PER_SECOND;
constexpr int TEMPO_FACTOR = 24;

constexpr uint16_t PULSE1_BASE = 0x4000;
constexpr uint16_t PULSE2_BASE = 0x4004;
constexpr uint16_t TRIANGLE_BASE = 0x4008;
constexpr uint16_t NOISE_BASE = 0x400C;
constexpr uint16_t STATUS_REGISTER = 0x4015;

constexpr uint8_t ENABLE_CHANNELS = 0x0F;      // Pulse 1, pulse 2, triangle and noise
constexpr uint8_t SWEEP_DISABLED = 0x08;       // Negate set so that low periods are not muted
constexpr uint8_t HALT_CONSTANT_VOLUME = 0x30; // Length counter halted, constant volume
constexpr uint8_t TRIANGLE_ON = 0xFF;          // Linear counter halted at its maximum
constexpr uint8_t TRIANGLE_OFF = 0x00;         // Linear counter reloads to zero
constexpr uint8_t LENGTH_LOAD = 0x08;          // Any non-zero length, since it is halted

} // namespace

SongPlayer::SongPlayer(RegisterWriter writer)
    : m_writer(std::move(writer))
{
}

std::shared_ptr<const Song> SongPlayer::setSong(std::shared_ptr<const Song> song)
{
    std::swap(m_song, song);
    if (!m_song) {
        stop();
    }
    // The position is checked against the new song when moving to the next row
    return song;
}

void SongPlayer::play(uint16_t frame, uint16_t row)
{
    if (!m_song) {
        return;
    }

    stop();
    for (ChannelState& state : m_channels) {
        const bool muted = state.muted;
        state = ChannelState{};
        state.muted = muted;
    }

    m_playing = true;
    m_firstRow = true;
    m_frame = frame;
    m_row = row;
    m_speed = std::clamp(m_song->speed, MIN_SPEED, MAX_SPEED);
    m_tempo = std::clamp(m_song->tempo, MIN_TEMPO, MAX_TEMPO);
    m_tempoAccumulator = 0;
}

void SongPlayer::stop()
{
    m_playing = false;
    m_firstRow = false;
    m_jumpFrame = -1;
    m_skipRow = -1;
    m_halt = false;

    for (ChannelState& state : m_channels) {
        state.active = false;
        state.released = false;
    }
}

void SongPlayer::playNote(ChannelType channel, uint8_t note, uint8_t instrument)
{
    ChannelState& state = m_channels[toIndex(channel)];
    if (instrument != VALUE_NONE) {
        state.instrument = instrument;
    }
    if (!m_playing) {
        state.volume = MAX_VOLUME;
    }

    if (note == NOTE_OFF) {
        state.active = false;
    }
    else if (note == NOTE_RELEASE) {
        releaseNote(state);
    }
    else {
        triggerNote(state, note);
    }
}

void SongPlayer::stopNote(ChannelType channel)
{
    m_channels[toIndex(channel)].active = false;
}

void SongPlayer::setChannelMuted(ChannelType channel, bool muted)
{
    m_channels[toIndex(channel)].muted = muted;
}

void SongPlayer::tick()
{
    if (!m_apuInitialized) {
        m_writer(STATUS_REGISTER, ENABLE_CHANNELS);
        m_writer(PULSE1_BASE + 1, SWEEP_DISABLED);
        m_writer(PULSE2_BASE + 1, SWEEP_DISABLED);
        m_apuInitialized = true;
    }

    if (m_playing && m_song) {
        if (m_tempoAccumulator <= 0) {
            if (m_halt) {
                stop();
            }
            else {
                advancePosition();
                processRow();
                m_tempoAccumulator += TEMPO_ACCUMULATOR_RELOAD;
            }
        }
        if (m_playing) {
            m_tempoAccumulator -= m_tempo * TEMPO_FACTOR / m_speed;
        }
    }

    for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
        updateChannel(static_cast<ChannelType>(channel));
    }
}

void SongPlayer::advancePosition()
{
    if (m_firstRow) {
        m_firstRow = false;
    }
    else if (m_jumpFrame >= 0 || m_skipRow >= 0) {
        m_frame = static_cast<uint16_t>(m_jumpFrame >= 0 ? m_jumpFrame : m_frame + 1);
        m_row = static_cast<uint16_t>(m_skipRow >= 0 ? m_skipRow : 0);
    }
    else if (++m_row >= m_song->rowsPerPattern) {
        m_row = 0;
        m_frame++;
    }
    m_jumpFrame = -1;
    m_skipRow = -1;

    // Also wraps around after the last frame, and keeps the position valid after the song changed
    if (m_frame >= m_song->frames.size()) {
        m_frame = 0;
    }
    if (m_row >= m_song->rowsPerPattern) {
        m_row = 0;
    }
}

void SongPlayer::processRow()
{
    for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
        const Cell* cell = m_song->findCell(static_cast<ChannelType>(channel), m_frame, m_row);
        if (cell != nullptr) {
            applyCell(m_channels[channel], *cell);
        }
    }
}

void SongPlayer::applyCell(ChannelState& state, const Cell& cell)
{
    if (cell.instrument != VALUE_NONE) {
        state.instrument = cell.instrument;
    }
    if (cell.volume != VALUE_NONE) {
        state.volume = std::min(cell.volume, MAX_VOLUME);
    }

    if (cell.note == NOTE_OFF) {
        state.active = false;
    }
    else if (cell.note == NOTE_RELEASE) {
        releaseNote(state);
    }
    else if (cell.note != NOTE_NONE) {
        triggerNote(state, cell.note);
    }

    for (const Effect& effect : cell.effects) {
        switch (effect.type) {
        case EffectType::Speed:
            if (effect.param == 0) {
                break;
            }
            if (effect.param < MIN_TEMPO) {
                m_speed = effect.param;
            }
            else {
                m_tempo = effect.param;
            }
            break;
        case EffectType::Jump: m_jumpFrame = effect.param; break;
        case EffectType::Skip: m_skipRow = effect.param; break;
        case EffectType::Halt: m_halt = true; break;
        default:               break;
        }
    }
}

void SongPlayer::triggerNote(ChannelState& state, uint8_t note)
{
    if (note < NOTE_MIN || note > NOTE_MAX) {
        return;
    }
    state.note = note;
    state.active = true;
    state.released = false;
    state.triggered = true;
    state.sequencePositions.fill(0);
}

void SongPlayer::releaseNote(ChannelState& state)
{
    if (!state.active) {
        return;
    }
    state.released = true;

    const Instrument* instrument = m_song ? m_song->findInstrument(state.instrument) : nullptr;
    if (instrument == nullptr) {
        return;
    }
    for (size_t type = 0; type < SEQUENCE_TYPE_COUNT; type++) {
        const Sequence& sequence = instrument->sequences[type];
        if (sequence.releasePoint >= 0 &&
            static_cast<size_t>(sequence.releasePoint) < sequence.values.size()) {
            state.sequencePositions[type] = static_cast<uint16_t>(sequence.releasePoint + 1);
        }
    }
}

int8_t SongPlayer::stepSequence(ChannelState& state, SequenceType type, int8_t defaultValue)
{
    const Instrument* instrument = m_song ? m_song->findInstrument(state.instrument) : nullptr;
    if (instrument == nullptr || instrument->getSequence(type).isEmpty()) {
        return defaultValue;
    }

    const Sequence& sequence = instrument->getSequence(type);
    const int size = static_cast<int>(sequence.values.size());
    const int loop = (sequence.loopPoint >= 0 && sequence.loopPoint < size) ? sequence.loopPoint : -1;
    const int release =
        (sequence.releasePoint >= 0 && sequence.releasePoint < size) ? sequence.releasePoint : -1;

    uint16_t& position = state.sequencePositions[toIndex(type)];
    if (position >= size) {
        position = static_cast<uint16_t>(size - 1);
    }
    const int8_t value = sequence.values[position];

    if (!state.released && release >= 0 && position == release) {
        // Wait at the release point, or loop before it
        if (loop >= 0 && loop < release) {
            position = static_cast<uint16_t>(loop);
        }
    }
    else if (position + 1 >= size) {
        // A loop before the release point is only for the part before the release
        if (loop >= 0 && (release < 0 || loop > release)) {
            position = static_cast<uint16_t>(loop);
        }
    }
    else {
        position++;
    }
    return value;
}

void SongPlayer::updateChannel(ChannelType channel)
{
    ChannelState& state = m_channels[toIndex(channel)];

    uint8_t volume = 0;
    uint8_t duty = 0;
    if (state.active) {
        const int sequenceVolume =
            std::clamp<int>(stepSequence(state, SequenceType::Volume, MAX_VOLUME), 0, MAX_VOLUME);
        duty = static_cast<uint8_t>(stepSequence(state, SequenceType::Duty, 0) & 0x03);

        volume = static_cast<uint8_t>(sequenceVolume * state.volume / MAX_VOLUME);
        if (volume == 0 && sequenceVolume > 0 && state.volume > 0) {
            volume = 1;
        }
    }
    if (state.muted) {
        volume = 0;
    }

    switch (channel) {
    case ChannelType::Pulse1:
    case ChannelType::Pulse2:   writePulse(channel, volume, duty); break;
    case ChannelType::Triangle: writeTriangle(volume); break;
    case ChannelType::Noise:    writeNoise(volume, duty); break;
    default:                    break; // DPCM is not played yet
    }
    state.triggered = false;
}

void SongPlayer::writePulse(ChannelType channel, uint8_t volume, uint8_t duty)
{
    ChannelState& state = m_channels[toIndex(channel)];
    const uint16_t base = (channel == ChannelType::Pulse1) ? PULSE1_BASE : PULSE2_BASE;

    m_writer(base, static_cast<uint8_t>((duty << 6) | HALT_CONSTANT_VOLUME | volume));
    if (!state.active) {
        return;
    }

    // Writing the high bits restarts the waveform, so they are only written when they change
    const uint16_t period = getPulsePeriod(state.note);
    const int high = period >> 8;
    m_writer(base + 2, static_cast<uint8_t>(period & 0xFF));
    if (state.triggered || high != state.periodHigh) {
        m_writer(base + 3, static_cast<uint8_t>(LENGTH_LOAD | high));
        state.periodHigh = high;
    }
}

void SongPlayer::writeTriangle(uint8_t volume)
{
    ChannelState& state = m_channels[toIndex(ChannelType::Triangle)];

    // The triangle has no volume control: any volume above zero plays at full volume
    const bool audible = state.active && volume > 0;
    m_writer(TRIANGLE_BASE, audible ? TRIANGLE_ON : TRIANGLE_OFF);
    if (audible) {
        const uint16_t period = getTrianglePeriod(state.note);
        const int high = period >> 8;
        m_writer(TRIANGLE_BASE + 2, static_cast<uint8_t>(period & 0xFF));
        // Writing the high bits also reloads the linear counter, which is needed after silence
        if (state.triggered || !state.audible || high != state.periodHigh) {
            m_writer(TRIANGLE_BASE + 3, static_cast<uint8_t>(LENGTH_LOAD | high));
            state.periodHigh = high;
        }
    }
    state.audible = audible;
}

void SongPlayer::writeNoise(uint8_t volume, uint8_t duty)
{
    ChannelState& state = m_channels[toIndex(ChannelType::Noise)];

    m_writer(NOISE_BASE, static_cast<uint8_t>(HALT_CONSTANT_VOLUME | volume));
    if (!state.active) {
        return;
    }
    const uint8_t mode = (duty & 0x01) << 7;
    m_writer(NOISE_BASE + 2, static_cast<uint8_t>(mode | getNoisePeriodIndex(state.note)));
    if (state.triggered) {
        m_writer(NOISE_BASE + 3, LENGTH_LOAD);
    }
}

} // namespace ocf::audio::tracker
