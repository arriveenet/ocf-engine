// SPDX-License-Identifier: MIT
#include "ocf/audio/AudioFile.h"

#include "miniaudio.h"

#include <fstream>
#include <iterator>

namespace ocf::audio {

Result<MonoAudio, std::string> AudioFile::readMono(const std::filesystem::path& path)
{
    using AudioResult = Result<MonoAudio, std::string>;

    // Read through std::filesystem, so that any file name works on Windows
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return AudioResult::Err("cannot read the file");
    }
    const std::vector<char> data((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
    if (file.bad()) {
        return AudioResult::Err("cannot read the file");
    }

    const ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 1, 0);
    ma_decoder decoder;
    if (ma_decoder_init_memory(data.data(), data.size(), &config, &decoder) != MA_SUCCESS) {
        return AudioResult::Err("not a supported audio file (WAV, FLAC or MP3)");
    }

    MonoAudio audio;
    audio.sampleRate = decoder.outputSampleRate;

    // The length is not known in advance for some formats
    std::vector<float> chunk(16384);
    for (;;) {
        ma_uint64 framesRead = 0;
        const ma_result result =
            ma_decoder_read_pcm_frames(&decoder, chunk.data(), chunk.size(), &framesRead);
        audio.samples.insert(audio.samples.end(), chunk.begin(), chunk.begin() + framesRead);
        if (result != MA_SUCCESS || framesRead < chunk.size()) {
            break;
        }
    }
    ma_decoder_uninit(&decoder);
    return AudioResult::Ok(std::move(audio));
}

} // namespace ocf::audio
