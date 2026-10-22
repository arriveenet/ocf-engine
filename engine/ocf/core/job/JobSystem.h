// SPDX-License-Identifier: MIT
#pragma once

#include "ocf/core/job/Job.h"
#include "ocf/core/job/Worker.h"
#include "ocf/core/job/WorkStealingQueue.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

namespace ocf {
namespace job {

/**
 * @brief Configuration for the job system
 */
struct JobSystemConfig {
    uint32_t numWorkers = 0;  ///< Number of worker threads (0 = auto-detect based on hardware)
    size_t maxJobs = 4096;    ///< Maximum number of jobs in the pool
};

/**
 * @brief Job System for parallel task execution
 *
 * The JobSystem manages a pool of worker threads that execute jobs.
 * Key features:
 * - Work stealing for load balancing
 * - Job dependencies (parent-child relationships)
 * - Priority-based scheduling
 * - Wait/synchronization points
 *
 * Usage:
 * @code
 * JobSystem& js = JobSystem::getInstance();
 * js.initialize();
 *
 * // Create and run a simple job
 * JobHandle handle = js.createJob([](void* data) {
 *     // Do work
 * });
 * js.run(handle);
 * js.wait(handle);
 *
 * // Create jobs with dependencies
 * JobHandle parent = js.createJob([](void*) { printf("Parent\n"); });
 * JobHandle child = js.createJobAsChild(parent, [](void*) { printf("Child\n"); });
 * js.run(child);
 * js.run(parent);
 * js.wait(parent);  // Waits for both parent and child
 *
 * js.shutdown();
 * @endcode
 */
class JobSystem {
public:
    /**
     * @brief Get the singleton instance
     *
     * @return Reference to the JobSystem instance
     */
    static JobSystem& getInstance();

    /**
     * @brief Initialize the job system
     *
     * @param config Configuration options
     */
    void initialize(const JobSystemConfig& config = {});

    /**
     * @brief Shutdown the job system
     */
    void shutdown();

    /**
     * @brief Check if the system is initialized
     *
     * @return true if initialized
     */
    bool isInitialized() const { return m_initialized.load(std::memory_order_relaxed); }

    /**
     * @brief Create a new job
     *
     * @param function The function to execute
     * @param data User data pointer
     * @param priority Job priority
     * @return Handle to the created job
     */
    JobHandle createJob(JobFunction function, void* data = nullptr,
                        JobPriority priority = JobPriority::Normal);

    /**
     * @brief Create a job as a child of another job
     *
     * The parent job will not complete until all its children have completed.
     *
     * @param parent Handle to the parent job
     * @param function The function to execute
     * @param data User data pointer
     * @param priority Job priority
     * @return Handle to the created child job
     */
    JobHandle createJobAsChild(JobHandle parent, JobFunction function, void* data = nullptr,
                               JobPriority priority = JobPriority::Normal);

    /**
     * @brief Run a job
     *
     * Adds the job to the queue for execution by worker threads.
     *
     * @param handle Handle to the job
     */
    void run(JobHandle handle);

    /**
     * @brief Run a job and wait for it to complete
     *
     * @param handle Handle to the job
     */
    void runAndWait(JobHandle handle);

    /**
     * @brief Wait for a job to complete
     *
     * Blocks until the job and all its children have completed.
     *
     * @param handle Handle to the job
     */
    void wait(JobHandle handle);

    /**
     * @brief Wait for all jobs to complete
     *
     * Blocks until all submitted jobs have completed.
     */
    void waitAll();

    /**
     * @brief Check if a job has completed
     *
     * @param handle Handle to the job
     * @return true if the job has completed
     */
    bool isComplete(JobHandle handle) const;

    /**
     * @brief Get the number of worker threads
     *
     * @return Number of workers
     */
    uint32_t getWorkerCount() const { return static_cast<uint32_t>(m_workers.size()); }

    /**
     * @brief Get the current thread's worker ID
     *
     * @return Worker ID, or UINT32_MAX if not a worker thread
     */
    uint32_t getCurrentWorkerId() const;

    // Internal methods used by Worker class
    Job* getJob(uint32_t index);
    void finishJob(Job* job);
    std::optional<uint32_t> stealFromOthers(uint32_t currentWorkerId);
    std::optional<uint32_t> popFromGlobalQueue();
    void wakeAllWorkers();

private:
    friend class Worker;

    JobSystem() = default;
    ~JobSystem() = default;
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    uint32_t allocateJob();
    void freeJob(uint32_t index);
    void helpWithJob();

    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_shuttingDown{false};

    // Job pool
    std::vector<std::unique_ptr<Job>> m_jobs;
    std::unique_ptr<std::atomic<bool>[]> m_jobAllocated;
    size_t m_maxJobs{0};
    std::atomic<uint32_t> m_nextJobIndex{0};
    std::atomic<uint32_t> m_generation{1};

    // Workers
    std::vector<std::unique_ptr<Worker>> m_workers;

    // Global queue for distributing work
    WorkStealingQueue<uint32_t, 8192> m_globalQueue;

    // Synchronization for pending jobs count
    std::atomic<uint32_t> m_pendingJobs{0};

    // Thread-local worker ID
    static thread_local uint32_t s_currentWorkerId;
};

}  // namespace job
}  // namespace ocf
