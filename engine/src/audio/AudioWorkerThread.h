// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace ocf::audio {

class AudioStreamBuffer;

class AudioWorkerThread {
public:
    static constexpr std::chrono::milliseconds SLEEP_INTERVAL{10};

    AudioWorkerThread() = default;
    ~AudioWorkerThread();

    void start();

    void stop();

    void addStreamBuffer(AudioStreamBuffer* buffer);

    void removeStreamBuffer(AudioStreamBuffer* buffer);

private:
    void threadMain();

private:
    std::thread m_thread;
    std::atomic<bool> m_running = false;

    std::vector<AudioStreamBuffer*> m_streams;
    std::mutex m_streamsMutex;

    std::condition_variable m_cv;
    std::mutex m_cvMutex;
};

} // namespace ocf::audio
