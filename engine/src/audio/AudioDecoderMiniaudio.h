// SPDX-License-Identifier: MIT
#pragma once

#include "audio/AudioDecoder.h"

#include <miniaudio.h>

namespace ocf::audio {

class AudioDecoderMiniaudio :public AudioDecoder {
public:
    AudioDecoderMiniaudio();
    ~AudioDecoderMiniaudio();

    bool open(std::string_view filename) override;

    void close() override;

    uint64_t read(void* buffer, uint64_t frameCount) override;

    bool seek(uint64_t frameOffset) override;

    uint64_t tell() override;

private:
    ma_decoder m_decoder;
};

} // namespace ocf::audio
