// SPDX-License-Identifier: MIT
#include "audio/AudioDecoderWav.h"

#include <stdio.h>

namespace ocf::audio {

static constexpr uint32_t makeFourCC(char a, char b, char c, char d)
{
    return static_cast<uint32_t>((a) | ((b) << 8) | ((c) << 16) | ((static_cast<uint32_t>(d)) << 24));
}

constexpr uint32_t WAV_SIGN_ID = makeFourCC('W', 'A', 'V', 'E');
constexpr uint32_t WAV_FMT_ID = makeFourCC('f', 'm', 't', ' ');
constexpr uint32_t WAV_DATA_ID = makeFourCC('d', 'a', 't', 'a');

static bool scanWavChunk(WavFile& wavFile, uint32_t chunkID, WAV_CHUNK_HEADER* header, void* body, uint32_t bodySize)
{
    FILE* pFile = wavFile.pFile;
    for (; fread(header, sizeof(WAV_CHUNK_HEADER), 1, pFile); ) {
        wavFile.pcmDataOffset += sizeof(WAV_CHUNK_HEADER);
        if (header->chunkId == chunkID) {
            if (body != nullptr) {
                const uint32_t readSize = std::min(bodySize, header->chunkSize);
                fread(body, readSize, 1, pFile);
                if (header->chunkSize > bodySize) {
                    fseek(pFile, header->chunkSize - bodySize, SEEK_CUR);
                }
                wavFile.pcmDataOffset += header->chunkSize;
            }
            return true;
        }
        else {
            fseek(pFile, header->chunkSize, SEEK_CUR);
            wavFile.pcmDataOffset += header->chunkSize;
        }
    }
    return false;
}

static bool openWav(WavFile& wavFile)
{
    FILE* pFile = wavFile.pFile;
    wavFile.pcmDataOffset = 0;

    WAV_FILE_HEADER* header = &wavFile.fileHeader;

    fread(header, sizeof(WAV_RIFF_CHUNK), 1, pFile);
    wavFile.pcmDataOffset += sizeof(WAV_RIFF_CHUNK);

    if (wavFile.fileHeader.riff.format != WAV_SIGN_ID) {
        return false;
    }

    if (!scanWavChunk(wavFile, WAV_FMT_ID, &header->fmt.header, &header->fmt.audioFormat,
        sizeof(header->fmt) - sizeof(WAV_RIFF_CHUNK))) {
        return false;
    }

    WAVE_FMT_CHUNK& fmtInfo = header->fmt;
    const int bitDepth = fmtInfo.bitsPerSample;

    switch (fmtInfo.audioFormat) {
    case WAV_FORMAT::PCM:
    case WAV_FORMAT::IEEE:
        switch (bitDepth) {
        case 8:
            wavFile.sourceFormat = AudioFormat::U8;
            break;
        case 16:
            wavFile.sourceFormat = AudioFormat::S16;
            break;
        case 24:
            wavFile.sourceFormat = AudioFormat::S24;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return scanWavChunk(wavFile, WAV_DATA_ID, &header->pcmData, nullptr, 0);
}

AudioDecoderWav::AudioDecoderWav()
    : m_wavFile()
{
}

AudioDecoderWav::~AudioDecoderWav()
{
    close();
}

bool AudioDecoderWav::open(std::string_view filename)
{
#ifdef _WIN32
    if (fopen_s(&m_wavFile.pFile, filename.data(), "rb") != 0) {
        return false;
    }
#else
    m_wavFile.pFile = fopen64(filename.data(), "rb");
    if (m_wavFile.pFile == nullptr) {
        return false;
    }
#endif

    if (openWav(m_wavFile)) {
       const WAVE_FMT_CHUNK& fmtInfo =  m_wavFile.fileHeader.fmt;
       m_sampleRate                  = fmtInfo.sampleRate;
       m_channelCount                = fmtInfo.numChannels;
       m_bytesPerBlock               = fmtInfo.blockAling;
       m_format                = m_wavFile.sourceFormat;

       m_totalFrames = bytesToFrames(m_wavFile.fileHeader.pcmData.chunkSize);

        m_isOpened = true;
        return true;
    }

    return false;
}

void AudioDecoderWav::close()
{
    if (isOpened()) {
        fclose(m_wavFile.pFile);
        m_isOpened = false;
    }
}

uint32_t AudioDecoderWav::framesToBytes(uint32_t frames) const
{
    if (m_samplesPerBlock == 1) {
        return m_bytesPerBlock * frames;
    }
    return frames / m_samplesPerBlock * m_bytesPerBlock;
}

uint32_t AudioDecoderWav::bytesToFrames(uint32_t bytes) const
{
    if (m_samplesPerBlock == 1) {
        return bytes / m_bytesPerBlock;
    }
    return bytes / m_bytesPerBlock * m_samplesPerBlock;
}

uint32_t AudioDecoderWav::read(void* buffer, uint32_t frameCount)
{
    uint32_t bytesToRead = framesToBytes(frameCount);
    size_t readCount = fread(buffer, bytesToRead, 1, m_wavFile.pFile);
    uint32_t bytesRead = static_cast<uint32_t>(bytesToRead * readCount);
    return bytesToFrames(bytesRead);
}

bool AudioDecoderWav::seek(uint32_t frameOffset)
{
    auto offset = framesToBytes(frameOffset);
    fseek(m_wavFile.pFile, m_wavFile.pcmDataOffset + offset, SEEK_SET);
    uint32_t newOffset = static_cast<uint32_t>(ftell(m_wavFile.pFile));
    newOffset = (newOffset >= m_wavFile.pcmDataOffset) ? newOffset - m_wavFile.pcmDataOffset : -1;

    return offset == newOffset;
}

} // namespace ocf::audio
