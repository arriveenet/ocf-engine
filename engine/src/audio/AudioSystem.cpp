// SPDX-License-Identifier: MIT
#include "ocf/audio/AudioSystem.h"

#include "audio/AudioDecoderMiniaudio.h"
#include "audio/AudioDecoderWav.h"
#include "audio/AudioDevice.h"
#include "audio/AudioDeviceMiniaudio.h"
#include "audio/AudioMixer.h"
#include "audio/AudioSource.h"
#include "audio/AudioStream.h"
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
    m_imple->m_audioDevice = std::make_unique<AudioDeviceMiniaudio>();
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
    std::unique_ptr<AudioDecoder> decoder = std::make_unique<AudioDecoderMiniaudio>();
    if (decoder->open(fullPath)) {
        AudioStream* stream = new AudioStream(std::move(decoder));
        AudioHandle handle = AudioHandle(m_HandleCounter++);
        m_imple->m_audioSources[handle] = stream;
        m_imple->m_audioStreams.push_back(stream);

        OCF_LOG_DEBUG("[Audio] Loaded file: {}", filename);
        return handle;
    }
    else {
        OCF_LOG_ERROR("[Audio] Failed to load file: {}", filename);
    }

    return INVALID_AUDIO_HANDLE;
}

void AudioSystem::play(AudioHandle handle, bool loop, float volume)
{
    if (handle == INVALID_AUDIO_HANDLE) {
        return;
    }

    auto iter = m_imple->m_audioSources.find(handle);
    if (iter != m_imple->m_audioSources.end()) {
        m_imple->m_audioMixer->addSource(iter->second);

        iter->second->play();
        iter->second->setLooping(loop);
        iter->second->setVolume(volume);
    }
}

void AudioSystem::stop(AudioHandle handle)
{
    if (handle == INVALID_AUDIO_HANDLE) {
        return;
    }

    auto iter = m_imple->m_audioSources.find(handle);
    if (iter != m_imple->m_audioSources.end()) {
        m_imple->m_audioMixer->removeSource(iter->second);
        iter->second->stop();
    }
}

void AudioSystem::pause(AudioHandle handle)
{
    if (handle == INVALID_AUDIO_HANDLE) {
        return;
    }

    auto iter = m_imple->m_audioSources.find(handle);
    if (iter != m_imple->m_audioSources.end()) {
        m_imple->m_audioMixer->removeSource(iter->second);
        iter->second->pause();
    }
}

void AudioSystem::setVolume(AudioHandle handle, float volume)
{
    if (handle == INVALID_AUDIO_HANDLE) {
        return;
    }

    auto iter = m_imple->m_audioSources.find(handle);
    if (iter != m_imple->m_audioSources.end()) {
        iter->second->setVolume(volume);
    }
}

void AudioSystem::setLoop(AudioHandle handle, bool loop)
{
    if (handle == INVALID_AUDIO_HANDLE) {
        return;
    }

    auto iter = m_imple->m_audioSources.find(handle);
    if (iter != m_imple->m_audioSources.end()) {
        iter->second->setLooping(loop);
    }
}

} // namespace audio
} // namespace ocf
