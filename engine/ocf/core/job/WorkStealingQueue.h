// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace ocf {
namespace job {

/**
 * @brief Lock-free work stealing deque
 *
 * This is a single-producer, multi-consumer queue that supports:
 * - push() and pop() from the owner thread (LIFO)
 * - steal() from other threads (FIFO)
 *
 * The implementation uses atomic operations for thread safety without locks.
 *
 * @tparam T Type of elements stored in the queue
 * @tparam Capacity Maximum number of elements the queue can hold (must be power of 2)
 */
template <typename T, size_t Capacity = 4096>
class WorkStealingQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

public:
    WorkStealingQueue()
        : m_top(0)
        , m_bottom(0)
    {
        // Buffer elements are default-initialized; no need to explicitly initialize
        // since they will be overwritten when pushed
    }

    ~WorkStealingQueue() = default;

    // Non-copyable
    WorkStealingQueue(const WorkStealingQueue&) = delete;
    WorkStealingQueue& operator=(const WorkStealingQueue&) = delete;

    /**
     * @brief Push an element to the bottom of the queue (owner thread only)
     *
     * @param item The item to push
     * @return true if successful, false if queue is full
     */
    bool push(const T& item)
    {
        int64_t bottom = m_bottom.load(std::memory_order_relaxed);
        int64_t top = m_top.load(std::memory_order_acquire);

        if (bottom - top >= static_cast<int64_t>(Capacity)) {
            return false;  // Queue is full
        }

        m_buffer[bottom & MASK] = item;
        std::atomic_thread_fence(std::memory_order_release);
        m_bottom.store(bottom + 1, std::memory_order_relaxed);
        return true;
    }

    /**
     * @brief Pop an element from the bottom of the queue (owner thread only)
     *
     * @return The popped item, or std::nullopt if queue is empty
     */
    std::optional<T> pop()
    {
        int64_t bottom = m_bottom.load(std::memory_order_relaxed) - 1;
        m_bottom.store(bottom, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);

        int64_t top = m_top.load(std::memory_order_relaxed);

        if (top <= bottom) {
            T item = m_buffer[bottom & MASK];

            if (top == bottom) {
                // Last element, need to race with stealers
                if (!m_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst,
                                                   std::memory_order_relaxed)) {
                    // Lost race with stealer
                    m_bottom.store(bottom + 1, std::memory_order_relaxed);
                    return std::nullopt;
                }
                m_bottom.store(bottom + 1, std::memory_order_relaxed);
            }
            return item;
        }

        // Queue was empty
        m_bottom.store(bottom + 1, std::memory_order_relaxed);
        return std::nullopt;
    }

    /**
     * @brief Steal an element from the top of the queue (any thread)
     *
     * @return The stolen item, or std::nullopt if queue is empty
     */
    std::optional<T> steal()
    {
        int64_t top = m_top.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        int64_t bottom = m_bottom.load(std::memory_order_acquire);

        if (top < bottom) {
            T item = m_buffer[top & MASK];

            if (!m_top.compare_exchange_strong(top, top + 1, std::memory_order_seq_cst,
                                               std::memory_order_relaxed)) {
                // Failed to steal (another thread got it)
                return std::nullopt;
            }
            return item;
        }

        return std::nullopt;
    }

    /**
     * @brief Get the current size of the queue
     *
     * @return Approximate number of elements in the queue
     */
    size_t size() const
    {
        int64_t bottom = m_bottom.load(std::memory_order_relaxed);
        int64_t top = m_top.load(std::memory_order_relaxed);
        return static_cast<size_t>(bottom > top ? bottom - top : 0);
    }

    /**
     * @brief Check if the queue is empty
     *
     * @return true if queue appears to be empty
     */
    bool empty() const { return size() == 0; }

private:
    static constexpr size_t MASK = Capacity - 1;

    alignas(64) std::atomic<int64_t> m_top;
    alignas(64) std::atomic<int64_t> m_bottom;
    alignas(64) T m_buffer[Capacity];
};

}  // namespace job
}  // namespace ocf
