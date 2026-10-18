// SPDX-License-Identifier: MIT
#include "audio/AudioWorkerThread.h"

#include "audio/AudioStreamBuffer.h"

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

void AudioWorkerThread::addStreamBuffer(AudioStreamBuffer* buffer)
{
    std::lock_guard<std::mutex> lock(m_streamsMutex);
    m_streams.push_back(buffer);
    m_cv.notify_one();
}

void AudioWorkerThread::removeStreamBuffer(AudioStreamBuffer* buffer)
{
    std::lock_guard<std::mutex> lock(m_streamsMutex);
    m_streams.erase(
        std::remove(m_streams.begin(), m_streams.end(), buffer),
        m_streams.end()
    );
}

void AudioWorkerThread::threadMain()
{
    while (m_running.load(std::memory_order_relaxed)) {
        // Stream decode and fill buffer. The lock is held while decoding, so that a buffer is
        // no longer in use once removeStreamBuffer() returns
        {
            std::lock_guard<std::mutex> lock(m_streamsMutex);
            for (auto* buffer : m_streams) {
                if (buffer->needsMoreData()) {
                    buffer->decodeTask();
                }
            }
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
