// SPDX-License-Identifier: MIT
#include "audio/AudioWorkerThread.h"

#include "ocf/audio/AudioSource.h"

#include <algorithm>
#include <atomic>
#include <mutex>

namespace ocf::audio {

AudioWorkerThread::~AudioWorkerThread()
{
    stop();
}

void AudioWorkerThread::start()
{
    if (m_running.load()) return;

    m_running.store(true);
    m_thread = std::thread(&AudioWorkerThread::threadMain, this);
}

void AudioWorkerThread::stop()
{
    if (!m_running.load()) return;

    m_running.store(false);

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void AudioWorkerThread::addSource(AudioSource* source)
{
    std::lock_guard<std::mutex> lock(m_streamsMutex);
    m_streams.push_back(source);
    m_cv.notify_one();
}

void AudioWorkerThread::removeSource(AudioSource* source)
{
    std::lock_guard<std::mutex> lock(m_streamsMutex);
    m_streams.erase(
        std::remove(m_streams.begin(), m_streams.end(), source),
        m_streams.end()
    );
}

void AudioWorkerThread::threadMain()
{
    std::vector<AudioSource*> localStreams;

    while (m_running.load(std::memory_order_relaxed)) {
        // Copying the managed list
        {
            std::lock_guard<std::mutex> lock(m_streamsMutex);
            localStreams = m_streams;
        }

        // Stream decode and fill buffer
       for (auto* source : localStreams) {
            source->update();
       }

        // Sleep
        {
            std::unique_lock<std::mutex> lock(m_cvMutex);
            m_cv.wait_for(lock, SLEEP_INTERVAL, [this] {
                return !m_running.load(std::memory_order_relaxed);
            });
        }
    }
}

} // namespace ocf::audio
