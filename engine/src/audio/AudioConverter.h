// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/audio/AudioEnums.h"

#include <miniaudio.h>

namespace ocf::audio {

struct AudioSpec {
    AudioFormat format;
    uint32_t sampleRate;
    uint32_t channelCount;
};

class AudioConverter {
public:
    AudioConverter() = default;
    AudioConverter(AudioSpec inputSpec, AudioSpec outputSpec);
    ~AudioConverter();

    bool initialize(const AudioSpec& inputSpec, const AudioSpec& outputSpec);

    size_t calculateInputFrameCount(size_t outputFrameCount) const;

    size_t calculateOutputFrameCount(size_t inputFrameCount) const;

    size_t process(const void* inputBuffer, size_t inputFrameCount,
                   void* outputBuffer,size_t outputFrameCount);

    const AudioSpec& getInputSpec() const noexcept { return m_inputSpec; }
    const AudioSpec& getOutputSpec() const noexcept { return m_outputSpec; }

private:
    AudioSpec m_inputSpec;
    AudioSpec m_outputSpec;

    ma_data_converter m_converter;
};

} // namespace ocf::audio
