// SPDX-License-Identifier: MIT
#include "ocf/audio/AudioSystem.h"

#include "audio/AudioDecoderMiniaudio.h"
#include "audio/AudioDevice.h"
#include "audio/AudioDeviceMiniaudio.h"
#include "audio/AudioMixer.h"
#include "audio/AudioNesApu.h"
#include "audio/AudioSource.h"
#include "audio/AudioStreamBuffer.h"
#include "audio/AudioWorkerThread.h"


#include "ocf/core/Logger.h"
#include "ocf/platform/FileSystem.h"

#include <unordered_map>

namespace ocf {
namespace audio {

struct AudioSystem::Impl {
    std::unique_ptr<AudioDevice> m_audioDevice = nullptr;
    std::unique_ptr<AudioMixer> m_audioMixer = nullptr;
    std::unordered_map<HandleId, AudioBuffer*> m_audioBuffers;
    std::unordered_map<HandleId, AudioSource*> m_audioSources;
    std::vector<AudioStreamBuffer*> m_audioStreams;
    AudioWorkerThread m_workerThread;
};


AudioSystem::AudioSystem()
{
    m_impl = std::make_unique<Impl>();

    m_impl->m_audioMixer = std::make_unique<AudioMixer>();
    m_impl->m_audioDevice = std::make_unique<AudioDeviceMiniaudio>();
}

AudioSystem::~AudioSystem()
{
    if (m_impl->m_audioDevice) {
        m_impl->m_audioDevice->shutdown();
        m_impl->m_audioDevice.reset();
    }

    if (m_impl->m_audioMixer) {
        m_impl->m_audioMixer.reset();
    }
}

bool AudioSystem::initialize()
{
    if (m_impl->m_audioDevice) {
        m_initialized = m_impl->m_audioDevice->initialize(m_impl->m_audioMixer.get());
        if (m_initialized) {
            OCF_LOG_INFO("Audio device initialized");
        } else {
            OCF_LOG_ERROR("Failed to initialize audio device");
        }
    }

    if (m_initialized) {
        m_impl->m_audioDevice->start();
        m_impl->m_workerThread.start();
    }

    return m_initialized;
}

void AudioSystem::shutdown()
{
    m_impl->m_workerThread.stop();

    // Delete all audio sources
    for (auto& [handle, source] : m_impl->m_audioSources) {
        m_impl->m_audioMixer->removeSource(source);
        delete source;
    }
    m_impl->m_audioSources.clear();

    // Delete all audio buffers
    for (auto& [handle, buffer] : m_impl->m_audioBuffers) {
        delete buffer;
    }
    m_impl->m_audioBuffers.clear();

    if (m_impl->m_audioDevice) {
        m_impl->m_audioDevice->stop();
        m_impl->m_audioDevice->shutdown();
    }
}

void AudioSystem::update()
{
    for (auto& [handle, source] : m_impl->m_audioSources) {
        source->update();

        if (source->isStopped()) {
            m_impl->m_audioMixer->removeSource(source);

            AudioBuffer* buffer = source->getBuffer();
            // If the source is using a stream buffer, remove it from the worker thread
            if ((buffer != nullptr) && (buffer->getType() == AudioBuffer::Type::Stream)) {
                m_impl->m_workerThread.removeStreamBuffer(
                    static_cast<AudioStreamBuffer*>(buffer));
            }
        }
    }
}

AudioBufferHandle AudioSystem::createStreamBuffer(std::string_view filename)
{
    auto fullPath = FileSystem::getInstance()->getAssetFullPath(filename);
    std::unique_ptr<AudioDecoder> decoder = std::make_unique<AudioDecoderMiniaudio>();
    if (decoder->open(fullPath)) {
        AudioStreamBuffer* streamBuffer = new AudioStreamBuffer(std::move(decoder));
        AudioBufferHandle handle = AudioBufferHandle(m_HandleCounter++);
        m_impl->m_audioBuffers[handle.getId()] = streamBuffer;

        OCF_LOG_DEBUG("[Audio] Loaded file: {}", filename);
        return handle;
    }
    else {
        OCF_LOG_ERROR("[Audio] Failed to load file: {}", filename);
    }

    return AudioBufferHandle::InvalidHandle;
}

AudioSourceHandle AudioSystem::createSource(AudioBufferHandle bufferHandle)
{
    auto iter = m_impl->m_audioBuffers.find(bufferHandle.getId());
    if (iter != m_impl->m_audioBuffers.end()) {
        AudioSource* source = new AudioSource(iter->second);
        AudioSourceHandle handle = AudioSourceHandle(m_HandleCounter++);
        m_impl->m_audioSources[handle.getId()] = source;
        return handle;
    }

    return AudioSourceHandle::InvalidHandle;
}

AudioSourceHandle AudioSystem::createNesApuSource()
{
    AudioNesApu* apu = new AudioNesApu();
    AudioSourceHandle handle = AudioSourceHandle(m_HandleCounter++);
    m_impl->m_audioSources[handle.getId()] = apu;

    return handle;
}

void AudioSystem::destroyBuffer(AudioBufferHandle handle)
{
    if (!handle) {
        return;
    }

    // TODO: Check if any sources are using this buffer and stop them before deleting the buffer
    auto iter = m_impl->m_audioBuffers.find(handle.getId());
    if (iter != m_impl->m_audioBuffers.end()) {
        delete iter->second;
        m_impl->m_audioBuffers.erase(iter);
    }
}

void AudioSystem::destroySource(AudioSourceHandle handle)
{
    if (!handle) {
        return;
    }

    // TODO: Remove source from mixer and stop playback if it's currently playing
    auto iter = m_impl->m_audioSources.find(handle.getId());
    if (iter != m_impl->m_audioSources.end()) {
        iter->second->stop();
        delete iter->second;
        m_impl->m_audioSources.erase(iter);
    }
}

void AudioSystem::play(AudioSourceHandle handle, bool loop, float volume)
{
    if (!handle) {
        return;
    }

    auto iter = m_impl->m_audioSources.find(handle.getId());
    if (iter != m_impl->m_audioSources.end()) {
        AudioSource* source = iter->second;
        m_impl->m_audioMixer->addSource(source);

        // If the source is using a stream buffer, add it to the worker thread for decoding
        AudioBuffer* buffer = source->getBuffer();
        if (buffer != nullptr && buffer->getType() == AudioBuffer::Type::Stream) {
            m_impl->m_workerThread.addStreamBuffer(static_cast<AudioStreamBuffer*>(buffer));
        }

        source->play();
        source->setLooping(loop);
        source->setVolume(volume);
    }
}

void AudioSystem::stop(AudioSourceHandle handle)
{
    if (!handle) {
        return;
    }

    auto iter = m_impl->m_audioSources.find(handle.getId());
    if (iter != m_impl->m_audioSources.end()) {
        m_impl->m_audioMixer->removeSource(iter->second);

        // If the source is using a stream buffer, remove it from the worker thread
        if (iter->second->getBuffer()->getType() == AudioBuffer::Type::Stream) {
            m_impl->m_workerThread.removeStreamBuffer(
                static_cast<AudioStreamBuffer*>(iter->second->getBuffer()));
        }

        iter->second->stop();
    }
}

void AudioSystem::pause(AudioSourceHandle handle)
{
    if (!handle) {
        return;
    }

    auto iter = m_impl->m_audioSources.find(handle.getId());
    if (iter != m_impl->m_audioSources.end()) {
        m_impl->m_audioMixer->removeSource(iter->second);
        iter->second->pause();
    }
}

AudioState AudioSystem::getState(AudioSourceHandle handle) const
{
    if (!handle) {
        return AudioState::Initial;
    }

    auto iter = m_impl->m_audioSources.find(handle.getId());
    if (iter != m_impl->m_audioSources.end()) {
        return iter->second->getState();
    }

    return AudioState::Initial;
}

float AudioSystem::getVolume(AudioSourceHandle handle) const
{
    if (!handle) {
        return 0.0f;
    }

    auto iter = m_impl->m_audioSources.find(handle.getId());
    if (iter != m_impl->m_audioSources.end()) {
        return iter->second->getVolume();
    }

    return 0.0f;
}

void AudioSystem::setVolume(AudioSourceHandle handle, float volume)
{
    if (!handle) {
        return;
    }

    auto iter = m_impl->m_audioSources.find(handle.getId());
    if (iter != m_impl->m_audioSources.end()) {
        iter->second->setVolume(volume);
    }
}

bool AudioSystem::isLoop(AudioSourceHandle handle) const
{
    if (!handle) {
        return false;
    }

    auto iter = m_impl->m_audioSources.find(handle.getId());
    if (iter != m_impl->m_audioSources.end()) {
        return iter->second->isLooping();
    }

    return false;
}

void AudioSystem::setLoop(AudioSourceHandle handle, bool loop)
{
    if (!handle) {
        return;
    }

    auto iter = m_impl->m_audioSources.find(handle.getId());
    if (iter != m_impl->m_audioSources.end()) {
        iter->second->setLooping(loop);
    }
}

void AudioSystem::apu_writeRegister(AudioSourceHandle handle, uint16_t address, uint8_t data)
{
    if (!handle) {
        return;
    }

    auto iter = m_impl->m_audioSources.find(handle.getId());
    if (iter != m_impl->m_audioSources.end()) {
        AudioNesApu* apu = static_cast<AudioNesApu*>(iter->second);
        apu->writeRegister(address, data);
    }
}

uint8_t AudioSystem::apu_readStatusRegister(AudioSourceHandle handle)
{
    if (!handle) {
        return 0;
    }

    auto iter = m_impl->m_audioSources.find(handle.getId());
    if (iter != m_impl->m_audioSources.end()) {
        AudioNesApu* apu = static_cast<AudioNesApu*>(iter->second);
        return apu->readStatusRegister();
    }
    return 0;
}

void AudioSystem::apu_writeStatusRegister(AudioSourceHandle handle, uint8_t data)
{
    if (!handle) {
        return;
    }

        auto iter = m_impl->m_audioSources.find(handle.getId());
    if (iter != m_impl->m_audioSources.end()) {
        AudioNesApu* apu = static_cast<AudioNesApu*>(iter->second);
        apu->writeStatusRegister(data);
    }
}

} // namespace audio
} // namespace ocf
