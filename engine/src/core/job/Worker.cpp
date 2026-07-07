// SPDX-License-Identifier: MIT
#include "ocf/core/job/Worker.h"
#include "ocf/core/job/JobSystem.h"

#include <chrono>

namespace ocf {
namespace job {

Worker::Worker(JobSystem& jobSystem, uint32_t workerId)
    : m_jobSystem(jobSystem)
    , m_workerId(workerId)
{
}

Worker::~Worker()
{
    stop();
}

void Worker::start()
{
    m_shouldStop.store(false, std::memory_order_relaxed);
    m_thread = std::thread(&Worker::threadFunc, this);
    m_running.store(true, std::memory_order_relaxed);
}

void Worker::stop()
{
    m_shouldStop.store(true, std::memory_order_release);
    wakeUp();

    if (m_thread.joinable()) {
        m_thread.join();
    }

    m_running.store(false, std::memory_order_relaxed);
}

bool Worker::pushJob(uint32_t jobIndex)
{
    return m_localQueue.push(jobIndex);
}

std::optional<uint32_t> Worker::steal()
{
    return m_localQueue.steal();
}

void Worker::wakeUp()
{
    // Lock is required to ensure proper memory ordering when signaling
    // the condition variable. Without the lock, the notify may be lost
    // if the worker is between the predicate check and the actual wait.
    {
        std::lock_guard<std::mutex> lock(m_sleepMutex);
    }
    m_sleepCondition.notify_one();
}

void Worker::threadFunc()
{
    // Set the thread-local worker ID so JobSystem can identify which worker
    // thread is running
    JobSystem::s_currentWorkerId = m_workerId;

    while (!m_shouldStop.load(std::memory_order_acquire)) {
        auto jobIndex = getJob();

        if (jobIndex.has_value()) {
            Job* job = m_jobSystem.getJob(jobIndex.value());
            if (job && job->function) {
                job->function(job->data);
                m_jobSystem.finishJob(job);
            }
        }
        else {
            // No work available, sleep
            std::unique_lock<std::mutex> lock(m_sleepMutex);
            m_sleepCondition.wait_for(lock, std::chrono::milliseconds(1), [this] {
                return m_shouldStop.load(std::memory_order_acquire) || !m_localQueue.empty();
            });
        }
    }
}

std::optional<uint32_t> Worker::getJob()
{
    // First try our own queue
    auto job = m_localQueue.pop();
    if (job.has_value()) {
        return job;
    }

    // Try the global queue
    job = m_jobSystem.popFromGlobalQueue();
    if (job.has_value()) {
        return job;
    }

    // Try to steal from other workers
    return m_jobSystem.stealFromOthers(m_workerId);
}

}  // namespace job
}  // namespace ocf
