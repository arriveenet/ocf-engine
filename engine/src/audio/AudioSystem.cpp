#include "ocf/audio/AudioSystem.h"

#include "audio/AudioDecoderWav.h"
#include "audio/AudioDecoderMp3.h"
#include "audio/AudioDevice.h"
#include "audio/AudioMixer.h"
#include "audio/AudioSource.h"
#include "audio/AudioStream.h"
#include "audio/MiniaudioDevice.h"
#include "audio/AudioUtility.h"

#include "ocf/core/job/JobSystem.h"
#include "ocf/core/Logger.h"
#include "ocf/platform/FileSystem.h"

#include <unordered_map>

namespace ocf {
namespace audio {

struct AudioSystem::Imple {
    std::unique_ptr<AudioDevice> m_audioDevice = nullptr;
    std::unique_ptr<AudioMixer> m_audioMixer = nullptr;
    std::unordered_map<AudioHandle, AudioSource*> m_audioSources;
    std::vector<AudioStream*> m_audioStreams;
};


AudioSystem::AudioSystem()
{
    m_imple = std::make_unique<Imple>();

    m_imple->m_audioMixer = std::make_unique<AudioMixer>();
    m_imple->m_audioDevice = std::make_unique<MiniaudioDevice>();
}

AudioSystem::~AudioSystem()
{
    if (m_imple->m_audioDevice) {
        m_imple->m_audioDevice->shutdown();
        m_imple->m_audioDevice.reset();
    }

    if (m_imple->m_audioMixer) {
        m_imple->m_audioMixer.reset();
    }
}

bool AudioSystem::initialize()
{
    if (m_imple->m_audioDevice) {
        m_initialized = m_imple->m_audioDevice->initialize(m_imple->m_audioMixer.get());
        if (m_initialized) {
            OCF_LOG_INFO("Audio device initialized");
        } else {
            OCF_LOG_ERROR("Failed to initialize audio device");
        }
    }

    if (m_initialized) {
        m_imple->m_audioDevice->start();
    }

    return m_initialized;
}

void AudioSystem::shutdown()
{
    // Delete all audio sources
    for (auto& [handle, source] : m_imple->m_audioSources) {
        delete source;
    }
    m_imple->m_audioSources.clear();

    if (m_imple->m_audioDevice) {
        m_imple->m_audioDevice->stop();
        m_imple->m_audioDevice->shutdown();
    }
}

void AudioSystem::update()
{
    for (auto& stream : m_imple->m_audioStreams) {
        stream->update();
    }
}

AudioHandle AudioSystem::load(std::string_view filename)
{
    auto fullPath = FileSystem::getInstance()->getAssetFullPath(filename);
    AudioDecoderMp3* decoder = new AudioDecoderMp3();
    if (decoder->open(fullPath)) {
        AudioStream* stream = new AudioStream(decoder);
        m_imple->m_audioMixer->addSource(stream);
        m_imple->m_audioSources[0] = stream; // temporary, should generate a unique handle
        m_imple->m_audioStreams.push_back(stream);

        OCF_LOG_DEBUG("[Audio] Loaded file: {}", fullPath);
        OCF_LOG_DEBUG("[Audio] format: {}, Sample rate: {}, Channels: {}, Total frames: {}",
                          AudioUtility::getAudioFormatString(decoder->getFormat()),
                          decoder->getSampleRate(),
                          decoder->getChannelCount(),
                          decoder->getTotalFrames());
    }
    else {
        OCF_LOG_ERROR("[Audio] Failed to load file: {}", fullPath);
        delete decoder;
    }

    return AudioHandle(0);
}

void AudioSystem::play(AudioHandle handle)
{
    auto it = m_imple->m_audioSources.find(handle);
    if (it != m_imple->m_audioSources.end()) {
        it->second->play();
    }
}

} // namespace audio
} // namespace ocf
