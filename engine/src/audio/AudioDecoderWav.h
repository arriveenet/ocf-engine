// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/audio/AudioEnums.h"

#include "audio/AudioDecoder.h"

#include <string.h>

#if !defined(_WIN32)
typedef struct _GUID
{
    uint32_t Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;
__inline int IsEqualGUID(const GUID& rguid1, const GUID& rguid2)
{
    return !::memcmp(&rguid1, &rguid2, sizeof(GUID));
}
#else
#include <guiddef.h>
#endif

namespace ocf::audio {

enum class WAV_FORMAT : uint16_t {
    UNKNOWN   = 0x0000,
    PCM       = 0x0001,
    ADPCM     = 0x0002,
    IEEE      = 0x0003,
    ALAW      = 0x0006,
    MULAW     = 0x0007,
    IMA_ADPCM = 0x0011,
    EXT       = 0xFFEE
};

#pragma pack(push, 1)
struct WAV_CHUNK_HEADER {
    uint32_t chunkId;
    uint32_t chunkSize;
};

struct WAV_RIFF_CHUNK {
    WAV_CHUNK_HEADER header;
    uint32_t format;
};

struct WAVE_FMT_CHUNK {
    WAV_CHUNK_HEADER header;
    WAV_FORMAT audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAling;
    uint16_t bitsPerSample;
    struct {
        uint16_t cbSize;
        union {
            uint16_t validBitsPerSample;
            uint16_t samplesPerBlock;
            uint16_t reserved;
        };
        uint32_t channelMask;
        GUID subFormat;
    } ExtParams;
};

struct WAV_FILE_HEADER {
    WAV_RIFF_CHUNK riff;
    WAVE_FMT_CHUNK fmt;
    WAV_CHUNK_HEADER pcmData;
};
#pragma pack(pop)

struct WavFile {
    WAV_FILE_HEADER fileHeader;
    uint32_t pcmDataOffset;
    AudioFormat sourceFormat;
    FILE* pFile;
};

class AudioDecoderWav : public AudioDecoder {
public:
    AudioDecoderWav();
    ~AudioDecoderWav() override;

    bool open(std::string_view filename) override;

    void close() override;

    uint32_t framesToBytes(uint32_t frames) const override;

    uint32_t bytesToFrames(uint32_t bytes) const override;

    uint32_t read(void* buffer, uint32_t frameCount) override;

    bool seek(uint32_t frameOffset) override;

protected:
    WavFile m_wavFile;
};

} // namespace ocf::audio
