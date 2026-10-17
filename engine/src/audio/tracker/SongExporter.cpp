// SPDX-License-Identifier: MIT
#include "ocf/audio/tracker/SongExporter.h"

#include "audio/tracker/SongPlayer.h"
#include "audio/tracker/SongSynthesizer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <fstream>
#include <memory>
#include <system_error>
#include <vector>

namespace ocf::audio::tracker {

namespace {

// After Cxx, so that the filtered output settles instead of ending abruptly
constexpr double HALT_TAIL_SECONDS = 0.25;

constexpr size_t WAV_HEADER_SIZE = 44;
constexpr size_t SAMPLES_PER_WRITE = 8192;

std::string toUtf8(const std::filesystem::path& path)
{
    const std::u8string text = path.u8string();
    return std::string(text.begin(), text.end());
}

/** The song is only used during the call, so it is not copied. */
std::shared_ptr<const Song> borrow(const Song& song)
{
    return std::shared_ptr<const Song>(&song, [](const Song*) {});
}

WavExportOptions clampOptions(const WavExportOptions& options)
{
    WavExportOptions result = options;
    result.playCount = std::clamp(options.playCount, 1, SongExporter::MAX_PLAY_COUNT);
    result.fadeOutSeconds = std::clamp(options.fadeOutSeconds, 0.0f, SongExporter::MAX_FADE_OUT_SECONDS);
    return result;
}

/** Tells when the song has ended, from the playback position after each tick. */
class EndDetector {
public:
    enum class Event : uint8_t {
        None,
        Halted,      // Cxx
        LastPlayEnded, // The song jumped back for the playCount-th time
    };

    EndDetector(const Song& song, int playCount)
        : m_rowsPerPattern(song.rowsPerPattern)
        , m_playCount(playCount)
    {
    }

    Event update(const PlaybackStatus& status)
    {
        if (!status.playing) {
            return Event::Halted;
        }
        const int64_t position = static_cast<int64_t>(status.frame) * m_rowsPerPattern + status.row;
        const bool jumpedBack = m_previousPosition >= 0 && position < m_previousPosition;
        m_previousPosition = position;
        if (jumpedBack && ++m_plays >= m_playCount) {
            return Event::LastPlayEnded;
        }
        return Event::None;
    }

private:
    int64_t m_rowsPerPattern;
    int m_playCount;
    int m_plays = 0;
    int64_t m_previousPosition = -1;
};

void appendLittleEndian(std::array<uint8_t, WAV_HEADER_SIZE>& header, size_t& offset, uint32_t value,
                        int bytes)
{
    for (int i = 0; i < bytes; i++) {
        header[offset++] = static_cast<uint8_t>(value >> (8 * i));
    }
}

/** 16-bit mono PCM. */
std::array<uint8_t, WAV_HEADER_SIZE> createWavHeader(uint32_t sampleRate, uint32_t sampleCount)
{
    const uint32_t dataSize = sampleCount * 2;
    std::array<uint8_t, WAV_HEADER_SIZE> header{};
    size_t offset = 0;
    auto appendTag = [&](const char* tag) {
        for (int i = 0; i < 4; i++) {
            header[offset++] = static_cast<uint8_t>(tag[i]);
        }
    };
    appendTag("RIFF");
    appendLittleEndian(header, offset, 36 + dataSize, 4);
    appendTag("WAVE");
    appendTag("fmt ");
    appendLittleEndian(header, offset, 16, 4);             // Chunk size
    appendLittleEndian(header, offset, 1, 2);              // PCM
    appendLittleEndian(header, offset, 1, 2);              // Mono
    appendLittleEndian(header, offset, sampleRate, 4);
    appendLittleEndian(header, offset, sampleRate * 2, 4); // Bytes per second
    appendLittleEndian(header, offset, 2, 2);              // Block size
    appendLittleEndian(header, offset, 16, 2);             // Bits per sample
    appendTag("data");
    appendLittleEndian(header, offset, dataSize, 4);
    return header;
}

int16_t toPcm16(float sample)
{
    return static_cast<int16_t>(std::lround(std::clamp(sample, -1.0f, 1.0f) * 32767.0f));
}

} // namespace

WavExportLength SongExporter::measure(const Song& song, const WavExportOptions& options)
{
    const WavExportOptions clamped = clampOptions(options);
    SongPlayer player([](uint16_t, uint8_t) {});
    player.setSong(borrow(song));
    player.play(0, 0);

    EndDetector detector(song, clamped.playCount);
    const auto maxTicks = static_cast<int64_t>(MAX_SECONDS * SongSynthesizer::TICK_RATE);
    for (int64_t ticks = 1; ticks <= maxTicks; ticks++) {
        player.tick();
        switch (detector.update(player.getStatus())) {
        case EndDetector::Event::Halted:
            return {(ticks - 1) / SongSynthesizer::TICK_RATE + HALT_TAIL_SECONDS, false};
        case EndDetector::Event::LastPlayEnded:
            return {(ticks - 1) / SongSynthesizer::TICK_RATE + clamped.fadeOutSeconds, true};
        default: break;
        }
    }
    return {MAX_SECONDS, true};
}

Result<void, std::string> SongExporter::exportWav(const Song& song, const std::filesystem::path& path,
                                                  const WavExportOptions& options,
                                                  const ProgressCallback& onProgress)
{
    using ExportResult = Result<void, std::string>;
    const WavExportOptions clamped = clampOptions(options);
    if (clamped.sampleRate < 8000 || clamped.sampleRate > 192000) {
        return ExportResult::Err(std::format("unsupported sample rate {}", clamped.sampleRate));
    }

    std::error_code error;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), error);
    }
    std::filesystem::path temporaryPath = path;
    temporaryPath += ".tmp";
    std::ofstream file(temporaryPath, std::ios::binary | std::ios::trunc);
    if (!file) {
        return ExportResult::Err(std::format("cannot open {} for writing", toUtf8(path)));
    }
    auto fail = [&](std::string message) {
        file.close();
        std::filesystem::remove(temporaryPath, error);
        return ExportResult::Err(std::move(message));
    };

    // The sizes in the header are written at the end, when they are known
    const auto placeholder = createWavHeader(clamped.sampleRate, 0);
    file.write(reinterpret_cast<const char*>(placeholder.data()), placeholder.size());

    SongSynthesizer synthesizer(clamped.sampleRate);
    synthesizer.setSong(borrow(song));
    synthesizer.getPlayer().play(0, 0);

    const double expectedSamples =
        std::max(1.0, measure(song, clamped).seconds * clamped.sampleRate);
    const auto maxSamples = static_cast<uint64_t>(MAX_SECONDS * clamped.sampleRate);
    const auto fadeSamples = static_cast<uint64_t>(clamped.fadeOutSeconds * clamped.sampleRate);
    const auto tailSamples = static_cast<uint64_t>(HALT_TAIL_SECONDS * clamped.sampleRate);

    EndDetector detector(song, clamped.playCount);
    bool ending = false;
    uint64_t endingLength = 0;
    uint64_t endingRemaining = 0;
    bool fading = false;

    std::vector<int16_t> buffer;
    buffer.reserve(SAMPLES_PER_WRITE);
    uint64_t written = 0;
    auto flush = [&]() {
        file.write(reinterpret_cast<const char*>(buffer.data()),
                   static_cast<std::streamsize>(buffer.size() * sizeof(int16_t)));
        written += buffer.size();
        buffer.clear();
    };

    while (written + buffer.size() < maxSamples) {
        const float sample = synthesizer.renderSample();

        if (!ending) {
            const EndDetector::Event event = detector.update(synthesizer.getPlayer().getStatus());
            if (event == EndDetector::Event::Halted) {
                ending = true;
                endingLength = endingRemaining = tailSamples;
            }
            else if (event == EndDetector::Event::LastPlayEnded) {
                // This sample already belongs to the next play
                if (fadeSamples == 0) {
                    break;
                }
                ending = true;
                fading = true;
                endingLength = endingRemaining = fadeSamples;
            }
        }

        float gain = 1.0f;
        if (fading) {
            gain = static_cast<float>(endingRemaining) / static_cast<float>(endingLength);
        }
        buffer.push_back(toPcm16(sample * gain));

        if (ending && --endingRemaining == 0) {
            break;
        }

        if (buffer.size() == SAMPLES_PER_WRITE) {
            flush();
            if (!file) {
                return fail(std::format("cannot write {}", toUtf8(path)));
            }
            if (onProgress && !onProgress(std::min(1.0, written / expectedSamples))) {
                return fail("the export was cancelled");
            }
        }
    }
    flush();

    // The sizes, now that the length is known
    const auto header = createWavHeader(clamped.sampleRate, static_cast<uint32_t>(written));
    file.seekp(0);
    file.write(reinterpret_cast<const char*>(header.data()), header.size());
    file.close();
    if (!file) {
        return fail(std::format("cannot write {}", toUtf8(path)));
    }

    std::filesystem::rename(temporaryPath, path, error);
    if (error) {
        const std::string message = error.message();
        std::filesystem::remove(temporaryPath, error);
        return ExportResult::Err(std::format("cannot replace {}: {}", toUtf8(path), message));
    }
    if (onProgress) {
        onProgress(1.0);
    }
    return ExportResult::Ok();
}

} // namespace ocf::audio::tracker
