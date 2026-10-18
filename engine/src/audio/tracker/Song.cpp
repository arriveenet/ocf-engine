// SPDX-License-Identifier: MIT
#include "ocf/audio/tracker/Song.h"

#include <algorithm>

namespace ocf::audio::tracker {

bool Cell::isEmpty() const
{
    if (note != NOTE_NONE || instrument != VALUE_NONE || volume != VALUE_NONE) {
        return false;
    }
    return std::all_of(effects.begin(), effects.end(),
                       [](const Effect& effect) { return effect.type == EffectType::None; });
}

bool Pattern::isEmpty() const
{
    return std::all_of(rows.begin(), rows.end(), [](const Cell& cell) { return cell.isEmpty(); });
}

DpcmKey* Instrument::findDpcmKey(uint8_t note)
{
    return (note >= NOTE_MIN && note <= NOTE_MAX) ? &dpcmKeys[note - NOTE_MIN] : nullptr;
}

const DpcmKey* Instrument::findDpcmKey(uint8_t note) const
{
    return (note >= NOTE_MIN && note <= NOTE_MAX) ? &dpcmKeys[note - NOTE_MIN] : nullptr;
}

Song Song::createDefault()
{
    Song song;
    song.frames.push_back(Frame{});
    for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
        song.getPattern(static_cast<ChannelType>(channel), 0);
    }

    Instrument instrument;
    instrument.name = "Instrument";
    song.instruments.push_back(instrument);
    return song;
}

Pattern& Song::getPattern(ChannelType channel, uint8_t index)
{
    std::vector<Pattern>& pool = patterns[toIndex(channel)];
    while (pool.size() <= index) {
        Pattern pattern;
        pattern.rows.resize(rowsPerPattern);
        pool.push_back(std::move(pattern));
    }
    return pool[index];
}

const Pattern* Song::findPattern(ChannelType channel, uint8_t index) const
{
    const std::vector<Pattern>& pool = patterns[toIndex(channel)];
    return index < pool.size() ? &pool[index] : nullptr;
}

const Cell* Song::findCell(ChannelType channel, size_t frame, size_t row) const
{
    if (frame >= frames.size()) {
        return nullptr;
    }
    const Pattern* pattern = findPattern(channel, frames[frame][toIndex(channel)]);
    if (pattern == nullptr || row >= pattern->rows.size()) {
        return nullptr;
    }
    return &pattern->rows[row];
}

const Instrument* Song::findInstrument(uint8_t index) const
{
    return index < instruments.size() ? &instruments[index] : nullptr;
}

std::optional<size_t> Song::getDpcmSampleOffset(size_t index) const
{
    if (index >= dpcmSamples.size() || dpcmSamples[index].data.empty()) {
        return std::nullopt;
    }
    size_t offset = 0;
    for (size_t i = 0; i < index; i++) {
        offset += getDpcmAlignedSize(dpcmSamples[i].data.size());
    }
    if (offset + dpcmSamples[index].data.size() > DPCM_MEMORY_SIZE) {
        return std::nullopt;
    }
    return offset;
}

size_t Song::getDpcmMemoryUsed() const
{
    size_t used = 0;
    for (const DpcmSample& sample : dpcmSamples) {
        used += getDpcmAlignedSize(sample.data.size());
    }
    return used;
}

void Song::setRowsPerPattern(uint16_t rows)
{
    rowsPerPattern = std::clamp<uint16_t>(rows, 1, static_cast<uint16_t>(MAX_ROWS));
    for (std::vector<Pattern>& pool : patterns) {
        for (Pattern& pattern : pool) {
            pattern.rows.resize(rowsPerPattern);
        }
    }
}

} // namespace ocf::audio::tracker
