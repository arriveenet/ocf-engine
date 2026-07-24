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

    uint32_t read(void* buffer, uint32_t frameCount) override;

    bool seek(uint32_t frameOffset) override;

private:
    ma_decoder m_decoder;
};

} // namespace ocf::audio
