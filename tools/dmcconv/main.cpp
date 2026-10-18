// SPDX-License-Identifier: MIT
/*
 * ocf_dmcconv: converts audio files (WAV, FLAC, MP3) to samples of the APU's DMC channel (.dmc).
 * See docs/dmc-converter-design.md.
 */
#include <ocf/audio/AudioFile.h>
#include <ocf/audio/tracker/DpcmEncoder.h>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;
using namespace ocf::audio::tracker;

namespace {

constexpr uint32_t PREVIEW_SAMPLE_RATE = 44100;
constexpr float DEFAULT_TRIM_THRESHOLD_DB = -50.0f;

constexpr const char* USAGE = R"(Usage: ocf_dmcconv <input> [<input> ...] [options]

Converts audio files (WAV, FLAC, MP3) to DMC samples (.dmc) for the APU's DMC channel.

Options:
  -o, --output <path>     Output file, or folder for several inputs (default: next to the input)
  -r, --rate <0-15|auto>  Rate (default: 15). auto: the highest rate that fits in 4081 bytes
      --normalize         Scale the peak to full scale, before the gain
  -g, --gain <x|NdB>      Gain, e.g. 0.5 or -6dB. 1.0 is an amplitude of 63 levels (default: 1.0)
      --trim [dBFS]       Trim the quieter ends (default threshold: -50)
      --center <0-127>    The level of 0.0 in the waveform (default: 64)
      --rest <0-127>      The level at the start and the end (default: the center)
      --no-return         Do not return to the rest level at the end
      --method <name>     optimal: least error (default), greedy: as FamiTracker
      --truncate          Truncate samples longer than 4081 bytes instead of failing
  -p, --preview <path>    Also write the result as a 44.1kHz WAV, to a file or a folder
  -q, --quiet             Print errors only
  -h, --help              Show this help
)";

struct Arguments {
    std::vector<fs::path> inputs;
    fs::path output;
    std::optional<fs::path> preview;
    DpcmEncodeOptions options;
    bool quiet = false;
    bool help = false;
};

std::string toDisplay(const fs::path& path)
{
    try {
        return path.string();
    }
    catch (const std::system_error&) {
        const std::u8string text = path.u8string();
        return std::string(text.begin(), text.end());
    }
}

template <typename T>
std::optional<T> parseNumber(std::string_view text)
{
    T value{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc() || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

std::optional<uint8_t> parseLevel(std::string_view text)
{
    const std::optional<int> value = parseNumber<int>(text);
    if (!value || *value < 0 || *value > DPCM_LEVEL_MAX) {
        return std::nullopt;
    }
    return static_cast<uint8_t>(*value);
}

/** "0.5" or "-6dB". */
std::optional<float> parseGain(std::string_view text)
{
    if (text.size() > 2) {
        const std::string_view unit = text.substr(text.size() - 2);
        if (unit == "dB" || unit == "db" || unit == "DB") {
            const std::optional<float> decibels = parseNumber<float>(text.substr(0, text.size() - 2));
            if (!decibels) {
                return std::nullopt;
            }
            return std::pow(10.0f, *decibels / 20.0f);
        }
    }
    const std::optional<float> gain = parseNumber<float>(text);
    if (!gain || !std::isfinite(*gain) || *gain < 0.0f) {
        return std::nullopt;
    }
    return gain;
}

/** nullopt and a message when the arguments are invalid. */
std::optional<Arguments> parseArguments(int argc, char** argv)
{
    Arguments arguments;
    bool restSet = false;

    for (int i = 1; i < argc; i++) {
        const std::string_view argument = argv[i];
        auto takeValue = [&]() -> std::optional<std::string_view> {
            if (i + 1 >= argc) {
                std::cerr << std::format("{} needs a value\n", argument);
                return std::nullopt;
            }
            return std::string_view(argv[++i]);
        };
        auto invalid = [&](std::string_view value) {
            std::cerr << std::format("invalid value for {}: {}\n", argument, value);
            return std::nullopt;
        };

        if (argument == "-h" || argument == "--help") {
            arguments.help = true;
        }
        else if (argument == "-q" || argument == "--quiet") {
            arguments.quiet = true;
        }
        else if (argument == "-o" || argument == "--output") {
            const auto value = takeValue();
            if (!value) {
                return std::nullopt;
            }
            arguments.output = fs::path(std::string(*value));
        }
        else if (argument == "-p" || argument == "--preview") {
            const auto value = takeValue();
            if (!value) {
                return std::nullopt;
            }
            arguments.preview = fs::path(std::string(*value));
        }
        else if (argument == "-r" || argument == "--rate") {
            const auto value = takeValue();
            if (!value) {
                return std::nullopt;
            }
            if (*value == "auto") {
                arguments.options.rate = std::nullopt;
                continue;
            }
            const std::optional<int> rate = parseNumber<int>(*value);
            if (!rate || *rate < 0 || *rate > DPCM_MAX_RATE) {
                return invalid(*value);
            }
            arguments.options.rate = static_cast<uint8_t>(*rate);
        }
        else if (argument == "--normalize") {
            arguments.options.normalize = true;
        }
        else if (argument == "-g" || argument == "--gain") {
            const auto value = takeValue();
            if (!value) {
                return std::nullopt;
            }
            const std::optional<float> gain = parseGain(*value);
            if (!gain) {
                return invalid(*value);
            }
            arguments.options.gain = *gain;
        }
        else if (argument == "--trim") {
            // The threshold is optional
            std::optional<float> threshold;
            if (i + 1 < argc) {
                threshold = parseNumber<float>(argv[i + 1]);
            }
            if (threshold) {
                i++;
            }
            arguments.options.trimThresholdDb = threshold.value_or(DEFAULT_TRIM_THRESHOLD_DB);
        }
        else if (argument == "--center" || argument == "--rest") {
            const auto value = takeValue();
            if (!value) {
                return std::nullopt;
            }
            const std::optional<uint8_t> level = parseLevel(*value);
            if (!level) {
                return invalid(*value);
            }
            if (argument == "--center") {
                arguments.options.center = *level;
            }
            else {
                arguments.options.restLevel = *level;
                restSet = true;
            }
        }
        else if (argument == "--no-return") {
            arguments.options.returnToRest = false;
        }
        else if (argument == "--method") {
            const auto value = takeValue();
            if (!value) {
                return std::nullopt;
            }
            if (*value == "optimal") {
                arguments.options.method = DpcmEncodeMethod::Optimal;
            }
            else if (*value == "greedy") {
                arguments.options.method = DpcmEncodeMethod::Greedy;
            }
            else {
                return invalid(*value);
            }
        }
        else if (argument == "--truncate") {
            arguments.options.truncate = true;
        }
        else if (argument.size() > 1 && argument[0] == '-') {
            std::cerr << std::format("unknown option: {}\n", argument);
            return std::nullopt;
        }
        else {
            arguments.inputs.emplace_back(std::string(argument));
        }
    }

    if (!restSet) {
        arguments.options.restLevel = arguments.options.center;
    }
    return arguments;
}

/** The output of an input: the option itself, or a file named after the input in a folder. */
fs::path getOutputPath(const fs::path& option, const fs::path& input, std::string_view suffix,
                       bool severalInputs)
{
    fs::path name = input.stem();
    name += std::string(suffix);
    if (option.empty()) {
        return input.parent_path() / name;
    }
    std::error_code error;
    if (severalInputs || !option.has_filename() || fs::is_directory(option, error)) {
        return option / name;
    }
    return option;
}

bool writeFile(const fs::path& path, const void* data, size_t size)
{
    std::error_code error;
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path(), error);
    }
    std::ofstream file(path, std::ios::binary);
    file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    return static_cast<bool>(file);
}

void appendLittleEndian(std::vector<uint8_t>& bytes, uint32_t value, int size)
{
    for (int i = 0; i < size; i++) {
        bytes.push_back(static_cast<uint8_t>(value >> (8 * i)));
    }
}

/** The levels as the DAC holds them, resampled to 16-bit mono WAV. */
std::vector<uint8_t> makePreviewWav(const DpcmEncodeResult& result, uint8_t startLevel,
                                    uint8_t center)
{
    const std::vector<uint8_t> levels = decodeDpcm(result.data, startLevel);
    const double frequency = getDpcmRateFrequency(result.rate);
    const auto count = static_cast<size_t>(std::ceil(levels.size() * PREVIEW_SAMPLE_RATE / frequency));
    const uint32_t dataSize = static_cast<uint32_t>(count * 2);

    std::vector<uint8_t> wav;
    wav.reserve(44 + dataSize);
    wav.insert(wav.end(), {'R', 'I', 'F', 'F'});
    appendLittleEndian(wav, 36 + dataSize, 4);
    wav.insert(wav.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
    appendLittleEndian(wav, 16, 4);                       // Chunk size
    appendLittleEndian(wav, 1, 2);                        // PCM
    appendLittleEndian(wav, 1, 2);                        // Mono
    appendLittleEndian(wav, PREVIEW_SAMPLE_RATE, 4);
    appendLittleEndian(wav, PREVIEW_SAMPLE_RATE * 2, 4);  // Bytes per second
    appendLittleEndian(wav, 2, 2);                        // Block size
    appendLittleEndian(wav, 16, 2);                       // Bits per sample
    wav.insert(wav.end(), {'d', 'a', 't', 'a'});
    appendLittleEndian(wav, dataSize, 4);

    for (size_t i = 0; i < count; i++) {
        const size_t index = std::min(levels.size() - 1, static_cast<size_t>(i * frequency / PREVIEW_SAMPLE_RATE));
        const double value = std::clamp((levels[index] - center) / 64.0, -1.0, 1.0);
        appendLittleEndian(wav, static_cast<uint16_t>(static_cast<int16_t>(std::lround(value * 32767.0))), 2);
    }
    return wav;
}

std::string formatResult(const DpcmEncodeResult& result)
{
    const double frequency = getDpcmRateFrequency(result.rate);
    const std::string snr =
        std::isinf(result.snrDb) ? std::string("inf") : std::format("{:.1f}", result.snrDb);
    return std::format("rate {:2} ({:.1f}Hz)  {:4} bytes  {:.2f}s  SNR {}dB  overload {:.1f}%{}",
                       result.rate, frequency, result.data.size(),
                       result.data.size() * 8 / frequency, snr, result.overloadRatio * 100.0,
                       result.truncated ? "  (truncated)" : "");
}

bool convert(const Arguments& arguments, const fs::path& input)
{
    const bool severalInputs = arguments.inputs.size() > 1;
    const std::string inputName = toDisplay(input);
    auto fail = [&](std::string_view message) {
        std::cerr << std::format("{}: {}\n", inputName, message);
        return false;
    };

    auto audio = ocf::audio::AudioFile::readMono(input);
    if (audio.isErr()) {
        return fail(audio.unwrapErr());
    }

    auto encoded =
        DpcmEncoder::encode(audio.unwrap().samples, audio.unwrap().sampleRate, arguments.options);
    if (encoded.isErr()) {
        return fail(encoded.unwrapErr());
    }
    const DpcmEncodeResult& result = encoded.unwrap();

    const fs::path output = getOutputPath(arguments.output, input, ".dmc", severalInputs);
    if (!writeFile(output, result.data.data(), result.data.size())) {
        return fail(std::format("cannot write {}", toDisplay(output)));
    }

    if (arguments.preview) {
        const fs::path preview =
            getOutputPath(*arguments.preview, input, ".preview.wav", severalInputs);
        const std::vector<uint8_t> wav =
            makePreviewWav(result, arguments.options.restLevel, arguments.options.center);
        if (!writeFile(preview, wav.data(), wav.size())) {
            return fail(std::format("cannot write {}", toDisplay(preview)));
        }
    }

    if (!arguments.quiet) {
        std::cout << std::format("{} -> {}  {}\n", inputName, toDisplay(output.filename()),
                                 formatResult(result));
    }
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    const std::optional<Arguments> arguments = parseArguments(argc, argv);
    if (!arguments) {
        std::cerr << "Run with --help for the usage.\n";
        return 2;
    }
    if (arguments->help || arguments->inputs.empty()) {
        std::cout << USAGE;
        return arguments->help ? 0 : 2;
    }

    bool succeeded = true;
    for (const fs::path& input : arguments->inputs) {
        succeeded = convert(*arguments, input) && succeeded;
    }
    return succeeded ? 0 : 1;
}
