// SPDX-License-Identifier: MIT
#include "ocf/core/job/JobSystem.h"

#include "ocf/core/Logger.h"

#include <algorithm>
#include <random>
#include <thread>

namespace ocf {
namespace job {

thread_local uint32_t JobSystem::s_currentWorkerId = UINT32_MAX;

JobSystem& JobSystem::getInstance()
{
    static JobSystem instance;
    return instance;
}

void JobSystem::initialize(const JobSystemConfig& config)
{
    if (m_initialized.load(std::memory_order_relaxed)) {
        return;
    }

    // Determine number of workers
    uint32_t numWorkers = config.numWorkers;
    if (numWorkers == 0) {
        unsigned int hwConcurrency = std::thread::hardware_concurrency();
        hwConcurrency = (hwConcurrency == 0) ? 1 : hwConcurrency - 1;
        numWorkers = std::max(1u, hwConcurrency);
    }

    // Initialize job pool
    m_maxJobs = config.maxJobs;
    m_jobs.resize(m_maxJobs);
    m_jobAllocated = std::make_unique<std::atomic<bool>[]>(m_maxJobs);
    for (size_t i = 0; i < m_maxJobs; ++i) {
        m_jobs[i] = std::make_unique<Job>();
        m_jobAllocated[i].store(false, std::memory_order_relaxed);
    }

    // Create workers
    m_workers.reserve(numWorkers);
    for (uint32_t i = 0; i < numWorkers; ++i) {
        m_workers.push_back(std::make_unique<Worker>(*this, i));
    }

    // Start workers
    for (auto& worker : m_workers) {
        worker->start();
    }

    m_initialized.store(true, std::memory_order_release);
   
    OCF_LOG_DEBUG("JobSystem initialized with {} workers and max {} jobs", numWorkers, m_maxJobs);
}

void JobSystem::shutdown()
{
    if (!m_initialized.load(std::memory_order_relaxed)) {
        return;
    }

    m_shuttingDown.store(true, std::memory_order_release);

    // Wait for all pending jobs
    waitAll();

    // Stop workers
    for (auto& worker : m_workers) {
        worker->stop();
    }
    m_workers.clear();

    // Clear job pool
    m_jobs.clear();
    m_jobAllocated.reset();
    m_maxJobs = 0;

    m_shuttingDown.store(false, std::memory_order_relaxed);
    m_initialized.store(false, std::memory_order_release);
}

JobHandle JobSystem::createJob(JobFunction function, void* data, JobPriority priority)
{
    if (m_shuttingDown.load(std::memory_order_relaxed)) {
        return INVALID_JOB_HANDLE;
    }

    uint32_t index = allocateJob();
    if (index == UINT32_MAX) {
        return INVALID_JOB_HANDLE;
    }

    std::atomic_thread_fence(std::memory_order_release);

    Job* job = m_jobs[index].get();
    job->function = std::move(function);
    job->data = data;
    job->priority = priority;
    job->parent = INVALID_JOB_HANDLE;
    job->unfinishedJobs.store(1, std::memory_order_relaxed);
    job->handle.id = index + 1;  // 1-based ID
    job->handle.generation =
        (m_generation.fetch_add(1, std::memory_order_relaxed)) % UINT32_MAX + 1; // Avoid 0 generation

    // @TODO: Handle priority (for now, all jobs are treated equally)
    return job->handle;
}

JobHandle JobSystem::createJobAsChild(JobHandle parent, JobFunction function, void* data,
                                      JobPriority priority)
{
    if (!parent.isValid()) {
        return createJob(std::move(function), data, priority);
    }

    // Find parent job
    uint32_t parentIndex = parent.id - 1;
    if (parentIndex >= m_jobs.size()) {
        return INVALID_JOB_HANDLE;
    }

    Job* parentJob = m_jobs[parentIndex].get();
    if (parentJob->handle != parent) {
        return INVALID_JOB_HANDLE;  // Parent job has been recycled
    }

    // Increment parent's unfinished count
    parentJob->unfinishedJobs.fetch_add(1, std::memory_order_relaxed);

    // Create child job
    JobHandle childHandle = createJob(std::move(function), data, priority);
    if (!childHandle.isValid()) {
        // Failed to create, decrement parent's count
        parentJob->unfinishedJobs.fetch_sub(1, std::memory_order_relaxed);
        return INVALID_JOB_HANDLE;
    }

    // Set parent reference
    uint32_t childIndex = childHandle.id - 1;
    m_jobs[childIndex]->parent = parent;

    return childHandle;
}

void JobSystem::run(JobHandle handle)
{
    if (!handle.isValid() || m_shuttingDown.load(std::memory_order_relaxed)) {
        return;
    }

    uint32_t index = handle.id - 1;
    if (index >= m_jobs.size()) {
        return;
    }

    m_pendingJobs.fetch_add(1, std::memory_order_relaxed);

    // Try to push to current worker's queue if we're on a worker thread
    uint32_t workerId = getCurrentWorkerId();
    if (workerId < m_workers.size()) {
        if (m_workers[workerId]->pushJob(index)) {
            return;
        }
    }

    // Push to global queue
    if (!m_globalQueue.push(index)) {
        // Queue full, execute synchronously
        Job* job = m_jobs[index].get();
        if (job->function) {
            job->function(job->data);
            finishJob(job);
        }
        return;
    }

    // Wake up workers
    wakeAllWorkers();
}

void JobSystem::runAndWait(JobHandle handle)
{
    run(handle);
    wait(handle);
}

void JobSystem::wait(JobHandle handle)
{
    if (!handle.isValid()) {
        return;
    }

    uint32_t index = handle.id - 1;
    if (index >= m_jobs.size()) {
        return;
    }

    // Help with work while waiting
    while (!isComplete(handle)) {
        helpWithJob();
    }
}

void JobSystem::waitAll()
{
    while (m_pendingJobs.load(std::memory_order_acquire) > 0) {
        helpWithJob();
    }
}

bool JobSystem::isComplete(JobHandle handle) const
{
    if (!handle.isValid()) {
        return true;
    }

    uint32_t index = handle.id - 1;
    if (index >= m_jobs.size()) {
        return true;
    }

    const Job* job = m_jobs[index].get();
    if (job->handle != handle) {
        return true;  // Job has been recycled
    }

    return job->unfinishedJobs.load(std::memory_order_acquire) == 0;
}

uint32_t JobSystem::getCurrentWorkerId() const
{
    return s_currentWorkerId;
}

Job* JobSystem::getJob(uint32_t index)
{
    if (index >= m_jobs.size()) {
        return nullptr;
    }
    return m_jobs[index].get();
}

void JobSystem::finishJob(Job* job)
{
    int32_t remaining = job->unfinishedJobs.fetch_sub(1, std::memory_order_acq_rel) - 1;

    if (remaining == 0) {
        // Job is complete, check parent
        uint32_t jobIndex = job->handle.id - 1;
        if (job->parent.isValid()) {
            uint32_t parentIndex = job->parent.id - 1;
            if (parentIndex < m_jobs.size()) {
                finishJob(m_jobs[parentIndex].get());
            }
        }

        // Free the job slot
        freeJob(jobIndex);

        // Decrement pending count
        m_pendingJobs.fetch_sub(1, std::memory_order_relaxed);
    }
}

std::optional<uint32_t> JobSystem::stealFromOthers(uint32_t currentWorkerId)
{
    size_t numWorkers = m_workers.size();
    if (numWorkers <= 1) {
        return std::nullopt;
    }

    // Start from a random worker to avoid contention
    // Use thread ID hash as seed for better performance than std::random_device
    static thread_local std::mt19937 rng(
        static_cast<unsigned int>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
    std::uniform_int_distribution<uint32_t> dist(0, static_cast<uint32_t>(numWorkers - 1));
    uint32_t startIndex = dist(rng);
    uint32_t numWorkersU32 = static_cast<uint32_t>(numWorkers);

    for (size_t i = 0; i < numWorkers; ++i) {
        uint32_t victimIndex = (startIndex + static_cast<uint32_t>(i)) % numWorkersU32;
        if (victimIndex == currentWorkerId) {
            continue;
        }

        auto stolen = m_workers[victimIndex]->steal();
        if (stolen.has_value()) {
            return stolen;
        }
    }

    return std::nullopt;
}

std::optional<uint32_t> JobSystem::popFromGlobalQueue()
{
    return m_globalQueue.steal();
}

void JobSystem::wakeAllWorkers()
{
    for (auto& worker : m_workers) {
        worker->wakeUp();
    }
}

uint32_t JobSystem::allocateJob()
{
    size_t poolSize = m_maxJobs;
    uint32_t startIndex = m_nextJobIndex.fetch_add(1, std::memory_order_relaxed) %
                          static_cast<uint32_t>(poolSize);

    for (size_t i = 0; i < poolSize; ++i) {
        uint32_t index = (startIndex + static_cast<uint32_t>(i)) % static_cast<uint32_t>(poolSize);
        bool expected = false;
        if (m_jobAllocated[index].compare_exchange_strong(expected, true,
                                                          std::memory_order_acq_rel)) {
            return index;
        }
    }

    return UINT32_MAX;  // Pool exhausted
}

void JobSystem::freeJob(uint32_t index)
{
    if (index < m_maxJobs) {
        m_jobAllocated[index].store(false, std::memory_order_release);
    }
}

void JobSystem::helpWithJob()
{
    // Try to get a job from global queue or steal
    auto jobIndex = m_globalQueue.steal();
    if (!jobIndex.has_value()) {
        // Try stealing from workers
        for (auto& worker : m_workers) {
            jobIndex = worker->steal();
            if (jobIndex.has_value()) {
                break;
            }
        }
    }

    if (jobIndex.has_value()) {
        Job* job = getJob(jobIndex.value());
        if (job && job->function) {
            job->function(job->data);
            finishJob(job);
        }
    }
    else {
        // No work available, yield
        std::this_thread::yield();
    }
}

}  // namespace job
}  // namespace ocf
