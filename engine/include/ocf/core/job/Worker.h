// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/job/Job.h"
#include "ocf/core/job/WorkStealingQueue.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace ocf {
namespace job {

class JobSystem;

/**
 * @brief Worker thread that executes jobs
 *
 * Each worker has its own local queue and can steal jobs from other workers
 * when its local queue is empty.
 */
class Worker {
public:
    /**
     * @brief Construct a new Worker
     *
     * @param jobSystem Reference to the owning job system
     * @param workerId Unique identifier for this worker
     */
    Worker(JobSystem& jobSystem, uint32_t workerId);
    ~Worker();

    // Non-copyable
    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;

    // Non-movable (due to thread)
    Worker(Worker&&) = delete;
    Worker& operator=(Worker&&) = delete;

    /**
     * @brief Start the worker thread
     */
    void start();

    /**
     * @brief Stop the worker thread
     */
    void stop();

    /**
     * @brief Push a job index to this worker's local queue
     *
     * @param jobIndex Index of the job in the job pool
     * @return true if successful, false if queue is full
     */
    bool pushJob(uint32_t jobIndex);

    /**
     * @brief Try to steal a job from this worker
     *
     * @return Job index if successful, empty optional otherwise
     */
    std::optional<uint32_t> steal();

    /**
     * @brief Wake up this worker if it's sleeping
     *
     * This method is thread-safe and can be called from any thread.
     * It uses a condition variable to signal the worker thread to wake up
     * and check for new work.
     */
    void wakeUp();

    /**
     * @brief Get the worker ID
     *
     * @return Worker ID
     */
    uint32_t getId() const { return m_workerId; }

    /**
     * @brief Check if the worker is running
     *
     * @return true if running
     */
    bool isRunning() const { return m_running.load(std::memory_order_relaxed); }

private:
    void threadFunc();
    std::optional<uint32_t> getJob();

    JobSystem& m_jobSystem;
    uint32_t m_workerId;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_shouldStop{false};

    WorkStealingQueue<uint32_t, 4096> m_localQueue;

    std::mutex m_sleepMutex;
    std::condition_variable m_sleepCondition;
};

}  // namespace job
}  // namespace ocf
