// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace ocf {
namespace job {

/**
 * @brief Priority levels for jobs
 *
 * Higher priority jobs are executed before lower priority jobs.
 */
enum class JobPriority : uint8_t {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

/**
 * @brief Handle to a job, used for dependency tracking
 */
struct JobHandle {
    uint32_t id = 0;
    uint32_t generation = 0;

    bool operator==(const JobHandle& other) const
    {
        return id == other.id && generation == other.generation;
    }

    bool operator!=(const JobHandle& other) const { return !(*this == other); }

    bool isValid() const { return id != 0; }
};

/**
 * @brief Invalid job handle constant
 */
constexpr JobHandle INVALID_JOB_HANDLE = {0, 0};

/**
 * @brief Function type for job execution
 *
 * @param data User-provided data pointer
 */
using JobFunction = std::function<void(void*)>;

/**
 * @brief Job structure representing a unit of work
 *
 * A Job is the smallest unit of work that can be scheduled and executed
 * by worker threads. Each job contains:
 * - An execution function
 * - A data pointer for input/output
 * - Dependency information
 * - Priority level
 */
struct alignas(64) Job {
    JobFunction function;               ///< The function to execute
    void* data = nullptr;               ///< User data pointer
    JobPriority priority = JobPriority::Normal;  ///< Job priority
    JobHandle handle;                   ///< Handle to this job
    JobHandle parent;                   ///< Parent job for hierarchical dependencies
    std::atomic<int32_t> unfinishedJobs{1};      ///< Count of unfinished child jobs + 1 (for self)

    Job() = default;

    Job(JobFunction func, void* userData = nullptr, JobPriority prio = JobPriority::Normal)
        : function(std::move(func))
        , data(userData)
        , priority(prio)
    {
    }

    // Non-copyable due to atomic
    Job(const Job&) = delete;
    Job& operator=(const Job&) = delete;

    // Movable
    Job(Job&& other) noexcept
        : function(std::move(other.function))
        , data(other.data)
        , priority(other.priority)
        , handle(other.handle)
        , parent(other.parent)
        , unfinishedJobs(other.unfinishedJobs.load(std::memory_order_relaxed))
    {
        other.data = nullptr;
    }

    Job& operator=(Job&& other) noexcept
    {
        if (this != &other) {
            function = std::move(other.function);
            data = other.data;
            priority = other.priority;
            handle = other.handle;
            parent = other.parent;
            unfinishedJobs.store(other.unfinishedJobs.load(std::memory_order_relaxed),
                                 std::memory_order_relaxed);
            other.data = nullptr;
        }
        return *this;
    }
};

}  // namespace job
}  // namespace ocf
