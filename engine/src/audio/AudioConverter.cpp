#include "audio/AudioConverter.h"

#include "audio/AudioUtility.h"
#include "audio/MiniaudioDevice.h"

namespace ocf::audio {

AudioConverter::AudioConverter(AudioSpec inputSpec, AudioSpec outputSpec)
    : m_inputSpec(inputSpec)
    , m_outputSpec(outputSpec)
    , m_converter{}
{
    initialize(inputSpec, outputSpec);
}

AudioConverter::~AudioConverter()
{
    ma_data_converter_uninit(&m_converter, nullptr);
}

bool AudioConverter::initialize(const AudioSpec& inputSpec, const AudioSpec& outputSpec)
{
    ma_data_converter_config config = ma_data_converter_config_init(
        AudioUtility::getMiniaudioFormat(inputSpec.format), AudioUtility::getMiniaudioFormat(outputSpec.format),
        inputSpec.channelCount, outputSpec.channelCount,
        inputSpec.sampleRate, outputSpec.sampleRate);

    config.resampling.algorithm = ma_resample_algorithm_linear;

    if (ma_data_converter_init(&config, nullptr, &m_converter) != MA_SUCCESS) {
        return false;
    }

    return true;
}

size_t AudioConverter::calculateInputFrameCount(size_t outputFrameCount) const
{
    ma_uint64 requiredInputFrameCount;
    ma_data_converter_get_required_input_frame_count(&m_converter, outputFrameCount,
                                                     &requiredInputFrameCount);
    return static_cast<size_t>(requiredInputFrameCount);
}

size_t AudioConverter::calculateOutputFrameCount(size_t inputFrameCount) const
{
    ma_uint64 expectedOutputFrameCount;
    ma_data_converter_get_expected_output_frame_count(&m_converter, inputFrameCount,
                                                      &expectedOutputFrameCount);
    return static_cast<size_t>(expectedOutputFrameCount);
}

size_t AudioConverter::process(const void* inputBuffer, size_t inputFrameCount,
                               void* outputBuffer, size_t outputFrameCount)
{
    ma_uint64 inFramesProcessed = inputFrameCount;
    ma_uint64 outFramesProcessed = outputFrameCount;

    ma_result result = ma_data_converter_process_pcm_frames(
        &m_converter, inputBuffer, &inFramesProcessed, outputBuffer, &outFramesProcessed);

    return static_cast<size_t>(outFramesProcessed);
}



} // namespace ocf::audio
