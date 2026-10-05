// SPDX-License-Identifier: MIT
#include "ocf/core/job/Job.h"
#include "ocf/core/job/JobSystem.h"
#include "ocf/core/job/WorkStealingQueue.h"

#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace ocf::job;

// ===========================================================================
// Job Tests
// ===========================================================================

TEST(JobTest, DefaultConstruction)
{
    Job job;
    EXPECT_EQ(job.data, nullptr);
    EXPECT_EQ(job.priority, JobPriority::Normal);
    EXPECT_FALSE(job.handle.isValid());
}

TEST(JobTest, ConstructionWithParameters)
{
    int userData = 42;
    Job job([](void*) {}, &userData, JobPriority::High);

    EXPECT_EQ(job.data, &userData);
    EXPECT_EQ(job.priority, JobPriority::High);
    EXPECT_TRUE(job.function);
}

TEST(JobTest, MoveConstruction)
{
    int userData = 42;
    Job job1([](void*) {}, &userData, JobPriority::High);

    Job job2(std::move(job1));

    EXPECT_EQ(job2.data, &userData);
    EXPECT_EQ(job2.priority, JobPriority::High);
    EXPECT_EQ(job1.data, nullptr);
}

TEST(JobTest, JobPriorities)
{
    EXPECT_LT(static_cast<uint8_t>(JobPriority::Low), static_cast<uint8_t>(JobPriority::Normal));
    EXPECT_LT(static_cast<uint8_t>(JobPriority::Normal), static_cast<uint8_t>(JobPriority::High));
    EXPECT_LT(static_cast<uint8_t>(JobPriority::High), static_cast<uint8_t>(JobPriority::Critical));
}

TEST(JobHandleTest, InvalidHandle)
{
    JobHandle handle;
    EXPECT_FALSE(handle.isValid());
    EXPECT_EQ(handle.id, 0u);
    EXPECT_EQ(handle.generation, 0u);
}

TEST(JobHandleTest, ValidHandle)
{
    JobHandle handle{1, 1};
    EXPECT_TRUE(handle.isValid());
}

TEST(JobHandleTest, Equality)
{
    JobHandle h1{1, 1};
    JobHandle h2{1, 1};
    JobHandle h3{2, 1};
    JobHandle h4{1, 2};

    EXPECT_EQ(h1, h2);
    EXPECT_NE(h1, h3);
    EXPECT_NE(h1, h4);
}

// ===========================================================================
// WorkStealingQueue Tests
// ===========================================================================

TEST(WorkStealingQueueTest, PushPop)
{
    WorkStealingQueue<int, 16> queue;

    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0u);

    EXPECT_TRUE(queue.push(1));
    EXPECT_TRUE(queue.push(2));
    EXPECT_TRUE(queue.push(3));

    EXPECT_EQ(queue.size(), 3u);

    // Pop returns LIFO order
    auto item = queue.pop();
    EXPECT_TRUE(item.has_value());
    EXPECT_EQ(item.value(), 3);

    item = queue.pop();
    EXPECT_TRUE(item.has_value());
    EXPECT_EQ(item.value(), 2);

    item = queue.pop();
    EXPECT_TRUE(item.has_value());
    EXPECT_EQ(item.value(), 1);

    item = queue.pop();
    EXPECT_FALSE(item.has_value());
}

TEST(WorkStealingQueueTest, Steal)
{
    WorkStealingQueue<int, 16> queue;

    queue.push(1);
    queue.push(2);
    queue.push(3);

    // Steal returns FIFO order (from the top)
    auto item = queue.steal();
    EXPECT_TRUE(item.has_value());
    EXPECT_EQ(item.value(), 1);

    item = queue.steal();
    EXPECT_TRUE(item.has_value());
    EXPECT_EQ(item.value(), 2);
}

TEST(WorkStealingQueueTest, Full)
{
    WorkStealingQueue<int, 4> queue;

    EXPECT_TRUE(queue.push(1));
    EXPECT_TRUE(queue.push(2));
    EXPECT_TRUE(queue.push(3));
    EXPECT_TRUE(queue.push(4));
    EXPECT_FALSE(queue.push(5));  // Queue is full
}

TEST(WorkStealingQueueTest, Empty)
{
    WorkStealingQueue<int, 4> queue;

    auto item = queue.pop();
    EXPECT_FALSE(item.has_value());

    item = queue.steal();
    EXPECT_FALSE(item.has_value());
}

// ===========================================================================
// JobSystem Tests
// ===========================================================================

class JobSystemTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        JobSystemConfig config;
        config.numWorkers = 2;
        config.maxJobs = 256;
        JobSystem::getInstance().initialize(config);
    }

    void TearDown() override { JobSystem::getInstance().shutdown(); }
};

TEST_F(JobSystemTest, Initialize)
{
    EXPECT_TRUE(JobSystem::getInstance().isInitialized());
    EXPECT_EQ(JobSystem::getInstance().getWorkerCount(), 2u);
}

TEST_F(JobSystemTest, CreateJob)
{
    auto handle = JobSystem::getInstance().createJob([](void*) {});
    EXPECT_TRUE(handle.isValid());
}

TEST_F(JobSystemTest, CreateJobWithData)
{
    int value = 0;
    auto handle = JobSystem::getInstance().createJob(
        [](void* data) {
            int* v = static_cast<int*>(data);
            *v = 42;
        },
        &value);

    EXPECT_TRUE(handle.isValid());
    JobSystem::getInstance().runAndWait(handle);
    EXPECT_EQ(value, 42);
}

TEST_F(JobSystemTest, RunAndWait)
{
    std::atomic<bool> executed{false};
    auto handle = JobSystem::getInstance().createJob([](void* data) {
        auto* flag = static_cast<std::atomic<bool>*>(data);
        flag->store(true, std::memory_order_release);
    }, &executed);

    JobSystem::getInstance().runAndWait(handle);
    EXPECT_TRUE(executed.load(std::memory_order_acquire));
}

TEST_F(JobSystemTest, MultipleJobs)
{
    constexpr int NUM_JOBS = 100;
    std::atomic<int> counter{0};

    std::vector<JobHandle> handles;
    handles.reserve(NUM_JOBS);

    for (int i = 0; i < NUM_JOBS; ++i) {
        auto handle = JobSystem::getInstance().createJob(
            [](void* data) {
                auto* c = static_cast<std::atomic<int>*>(data);
                c->fetch_add(1, std::memory_order_relaxed);
            },
            &counter);
        handles.push_back(handle);
        JobSystem::getInstance().run(handle);
    }

    // Wait for all jobs
    JobSystem::getInstance().waitAll();
    EXPECT_EQ(counter.load(), NUM_JOBS);
}

// Helper struct for job dependency test
// Note: These are non-owning pointers - the test manages lifetime of pointed-to objects
struct OrderData {
    std::atomic<int>* orderCounter;  // Pointer to shared order counter
    int* executionOrder;             // Pointer to store execution order value
};

TEST_F(JobSystemTest, JobDependencies)
{
    std::atomic<int> order{0};
    int parentOrder = -1;
    int childOrder = -1;

    // Use stack-allocated structs with named members for clarity
    OrderData parentData{&order, &parentOrder};
    OrderData childData{&order, &childOrder};

    auto parent = JobSystem::getInstance().createJob(
        [](void* data) {
            auto* d = static_cast<OrderData*>(data);
            *d->executionOrder = d->orderCounter->fetch_add(1, std::memory_order_seq_cst);
        },
        &parentData);

    auto child = JobSystem::getInstance().createJobAsChild(
        parent,
        [](void* data) {
            auto* d = static_cast<OrderData*>(data);
            *d->executionOrder = d->orderCounter->fetch_add(1, std::memory_order_seq_cst);
        },
        &childData);

    // Run child first, then parent
    JobSystem::getInstance().run(child);
    JobSystem::getInstance().run(parent);
    JobSystem::getInstance().wait(parent);

    // Both should have executed
    EXPECT_GE(parentOrder, 0);
    EXPECT_GE(childOrder, 0);
}

TEST_F(JobSystemTest, IsComplete)
{
    std::atomic<bool> proceed{false};
    auto handle = JobSystem::getInstance().createJob([](void* data) {
        auto* p = static_cast<std::atomic<bool>*>(data);
        while (!p->load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }, &proceed);

    EXPECT_FALSE(JobSystem::getInstance().isComplete(handle));

    JobSystem::getInstance().run(handle);

    // Allow job to complete
    proceed.store(true, std::memory_order_release);
    JobSystem::getInstance().wait(handle);

    EXPECT_TRUE(JobSystem::getInstance().isComplete(handle));
}

TEST_F(JobSystemTest, InvalidHandle)
{
    JobHandle invalid;
    EXPECT_TRUE(JobSystem::getInstance().isComplete(invalid));

    // Should not crash
    JobSystem::getInstance().run(invalid);
    JobSystem::getInstance().wait(invalid);
}

TEST_F(JobSystemTest, JobPriorities)
{
    auto lowPriority =
        JobSystem::getInstance().createJob([](void*) {}, nullptr, JobPriority::Low);
    auto highPriority =
        JobSystem::getInstance().createJob([](void*) {}, nullptr, JobPriority::High);

    EXPECT_TRUE(lowPriority.isValid());
    EXPECT_TRUE(highPriority.isValid());
}

// Test concurrent access to the queue
TEST(WorkStealingQueueConcurrentTest, ConcurrentPushPop)
{
    WorkStealingQueue<int, 1024> queue;
    constexpr int NUM_ITEMS = 500;
    std::atomic<int> popCount{0};
    std::atomic<int> stealCount{0};

    // Owner thread pushes and pops
    std::thread owner([&]() {
        for (int i = 0; i < NUM_ITEMS; ++i) {
            queue.push(i);
        }
        for (int i = 0; i < NUM_ITEMS / 2; ++i) {
            if (queue.pop().has_value()) {
                popCount.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    // Stealer threads
    std::vector<std::thread> stealers;
    for (int t = 0; t < 3; ++t) {
        stealers.emplace_back([&]() {
            for (int i = 0; i < NUM_ITEMS; ++i) {
                if (queue.steal().has_value()) {
                    stealCount.fetch_add(1, std::memory_order_relaxed);
                }
                std::this_thread::yield();
            }
        });
    }

    owner.join();
    for (auto& t : stealers) {
        t.join();
    }

    // Total retrieved should not exceed total pushed
    EXPECT_LE(popCount.load() + stealCount.load(), NUM_ITEMS);
}
