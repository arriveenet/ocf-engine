// SPDX-License-Identifier: MIT
#include "ocf/audio/AudioSystem.h"

#include "audio/AudioDecoderMiniaudio.h"
#include "audio/AudioDevice.h"
#include "audio/AudioDeviceMiniaudio.h"
#include "audio/AudioMixer.h"
#include "audio/AudioNesApu.h"
#include "audio/AudioSource.h"
#include "audio/AudioStreamBuffer.h"
#include "audio/AudioTrackerSource.h"
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

    /**
     * Stops the mixer and the worker thread from using the source and its stream buffer. Once
     * this returns, neither thread uses them.
     */
    void detachSource(AudioSource* source)
    {
        m_audioMixer->removeSource(source);

        AudioBuffer* buffer = source->getBuffer();
        if (buffer != nullptr && buffer->getType() == AudioBuffer::Type::Stream) {
            m_workerThread.removeStreamBuffer(static_cast<AudioStreamBuffer*>(buffer));
        }
    }

    AudioTrackerSource* findTrackerSource(AudioSourceHandle handle) const
    {
        if (!handle) {
            return nullptr;
        }
        auto iter = m_audioSources.find(handle.getId());
        if (iter == m_audioSources.end()) {
            return nullptr;
        }
        return dynamic_cast<AudioTrackerSource*>(iter->second);
    }
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
    // Stop the audio thread and the worker thread first, so that neither uses the sources and
    // buffers deleted below
    if (m_impl->m_audioDevice) {
        m_impl->m_audioDevice->stop();
    }
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
        m_impl->m_audioDevice->shutdown();
    }
}

void AudioSystem::update()
{
    for (auto& [handle, source] : m_impl->m_audioSources) {
        source->update();

        if (source->isStopped()) {
            m_impl->detachSource(source);
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

    auto iter = m_impl->m_audioBuffers.find(handle.getId());
    if (iter != m_impl->m_audioBuffers.end()) {
        // Stop the sources that use the buffer, so that no thread reads it after it is deleted
        for (auto& [sourceHandle, source] : m_impl->m_audioSources) {
            if (source->getBuffer() == iter->second) {
                m_impl->detachSource(source);
                source->stop();
                source->setBuffer(nullptr);
            }
        }
        if (iter->second->getType() == AudioBuffer::Type::Stream) {
            m_impl->m_workerThread.removeStreamBuffer(static_cast<AudioStreamBuffer*>(iter->second));
        }

        delete iter->second;
        m_impl->m_audioBuffers.erase(iter);
    }
}

void AudioSystem::destroySource(AudioSourceHandle handle)
{
    if (!handle) {
        return;
    }

    auto iter = m_impl->m_audioSources.find(handle.getId());
    if (iter != m_impl->m_audioSources.end()) {
        // The audio thread may be rendering the source until it is removed from the mixer
        m_impl->detachSource(iter->second);
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

        // Set up the source before the other threads see it: play() may reset its buffer
        source->setLooping(loop);
        source->setVolume(volume);
        source->play();

        // If the source is using a stream buffer, add it to the worker thread for decoding
        AudioBuffer* buffer = source->getBuffer();
        if (buffer != nullptr && buffer->getType() == AudioBuffer::Type::Stream) {
            m_impl->m_workerThread.addStreamBuffer(static_cast<AudioStreamBuffer*>(buffer));
        }

        m_impl->m_audioMixer->addSource(source);
    }
}

void AudioSystem::stop(AudioSourceHandle handle)
{
    if (!handle) {
        return;
    }

    auto iter = m_impl->m_audioSources.find(handle.getId());
    if (iter != m_impl->m_audioSources.end()) {
        // Detached first, since stop() resets the buffer the other threads read
        m_impl->detachSource(iter->second);
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

AudioSourceHandle AudioSystem::createTrackerSource()
{
    AudioTrackerSource* source = new AudioTrackerSource();
    AudioSourceHandle handle = AudioSourceHandle(m_HandleCounter++);
    m_impl->m_audioSources[handle.getId()] = source;

    return handle;
}

void AudioSystem::tracker_setSong(AudioSourceHandle handle,
                                  std::shared_ptr<const tracker::Song> song)
{
    if (AudioTrackerSource* source = m_impl->findTrackerSource(handle)) {
        source->setSong(std::move(song));
    }
}

void AudioSystem::tracker_play(AudioSourceHandle handle, uint16_t frame, uint16_t row)
{
    if (AudioTrackerSource* source = m_impl->findTrackerSource(handle)) {
        source->playSong(frame, row);
    }
}

void AudioSystem::tracker_stop(AudioSourceHandle handle)
{
    if (AudioTrackerSource* source = m_impl->findTrackerSource(handle)) {
        source->stopSong();
    }
}

tracker::PlaybackStatus AudioSystem::tracker_getStatus(AudioSourceHandle handle) const
{
    if (AudioTrackerSource* source = m_impl->findTrackerSource(handle)) {
        return source->getStatus();
    }
    return tracker::PlaybackStatus{};
}

void AudioSystem::tracker_playNote(AudioSourceHandle handle, tracker::ChannelType channel,
                                   uint8_t note, uint8_t instrument)
{
    if (AudioTrackerSource* source = m_impl->findTrackerSource(handle)) {
        source->playNote(channel, note, instrument);
    }
}

void AudioSystem::tracker_stopNote(AudioSourceHandle handle, tracker::ChannelType channel)
{
    if (AudioTrackerSource* source = m_impl->findTrackerSource(handle)) {
        source->stopNote(channel);
    }
}

void AudioSystem::tracker_playDpcmSample(AudioSourceHandle handle, const tracker::DpcmKey& key)
{
    if (AudioTrackerSource* source = m_impl->findTrackerSource(handle)) {
        source->playDpcmSample(key);
    }
}

void AudioSystem::tracker_setChannelMuted(AudioSourceHandle handle, tracker::ChannelType channel,
                                          bool muted)
{
    if (AudioTrackerSource* source = m_impl->findTrackerSource(handle)) {
        source->setChannelMuted(channel, muted);
    }
}

} // namespace audio
} // namespace ocf
