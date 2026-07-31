// SPDX-License-Identifier: MIT
#include "ocf/core/Allocator.h"
#include <algorithm>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace ocf;

// pointermath::add のテスト
TEST(PointerMathTest, Add)
{
    int arr[10] = {};
    int* p = arr;
    int* p2 = pointermath::add(p, 2 * sizeof(int));
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p2), reinterpret_cast<uintptr_t>(p) + 2 * sizeof(int));
}

// pointermath::align のテスト
TEST(PointerMathTest, Align)
{
    alignas(16) char buffer[64] = {};
    void* p = buffer + 1;
    void* aligned = pointermath::align(static_cast<void*>(p), 8);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(aligned) % 8, 0u);
}

// FreeList の push/pop テスト
TEST(FreeListTest, PushPop)
{
    constexpr size_t elementSize = 16;
    constexpr size_t alignment = alignof(std::max_align_t);
    char buffer[elementSize * 4] = {};
    FreeList list(buffer, buffer + sizeof(buffer), elementSize, alignment, 0);

    void* nodes[4] = {};
    for (int i = 0; i < 4; ++i) {
        nodes[i] = list.pop();
        EXPECT_NE(nodes[i], nullptr);
    }
    // もう空なので pop で nullptr
    EXPECT_EQ(list.pop(), nullptr);

    // push して再利用できるか
    list.push(nodes[0]);
    void* p = list.pop();
    EXPECT_EQ(p, nodes[0]);
}

TEST(FreeListTest, Offset)
{
    constexpr size_t elementSize = 32;
    constexpr size_t alignment = alignof(std::max_align_t);
    constexpr size_t offset = 1;
    // alignment の倍数に丸める
    constexpr size_t bufferSize = ((elementSize + offset + alignment - 1) / alignment) * alignment;
    char buffer[bufferSize * 4] = {};
    FreeList list(buffer, buffer + sizeof(buffer), elementSize, alignment, offset);
    void* nodes[4] = {};
    for (int i = 0; i < 3; ++i) {
        nodes[i] = list.pop();
        EXPECT_NE(nodes[i], nullptr);
        EXPECT_EQ(reinterpret_cast<uintptr_t>(nodes[i]) % alignment, 0u);
    }
    // もう空なので pop で nullptr
    EXPECT_EQ(list.pop(), nullptr);
    // push して再利用できるか
    list.push(nodes[0]);
    void* p = list.pop();
    EXPECT_EQ(p, nodes[0]);
}

TEST(FreeListTest, OffsetValue)
{
    struct Node {
        uint8_t value;
    };

    constexpr size_t elementSize = 32;
    constexpr size_t alignment = alignof(std::max_align_t);
    constexpr size_t offset = sizeof(Node);

    // alignment の倍数に丸める
    constexpr size_t bufferSize = ((elementSize + offset + alignment - 1) / alignment) * alignment;
    char buffer[bufferSize * 4] = {};

    memset(buffer, 0, sizeof(buffer));

    FreeList list(buffer, buffer + sizeof(buffer), elementSize, alignment, offset);

    void* nodes[4] = {};
    for (int i = 0; i < 3; ++i) {
        nodes[i] = list.pop();
        Node* n = static_cast<Node*>(nodes[i]);
        uint8_t value = n[-1].value;
        EXPECT_EQ(value, 0u);
    }

    list.pop(); // もう空なので pop で nullptr
    EXPECT_EQ(list.pop(), nullptr);

    Node* node =static_cast<Node*>(nodes[2]);
    uint8_t& value = node[-1].value;
    value = 42 & 0xFF;
    list.push(nodes[2]);

    void* p = list.pop();
    Node* n = static_cast<Node*>(p);
    EXPECT_EQ(n[-1].value, 42u);
}

// MemoryPool の alloc/free テスト
TEST(MemoryPoolTest, AllocFree)
{
    constexpr size_t elementSize = 32;
    constexpr size_t alignment = alignof(std::max_align_t);
    char buffer[elementSize * 2];
    MemoryPool<elementSize, alignment> pool(buffer, sizeof(buffer));

    void* p1 = pool.alloc();
    void* p2 = pool.alloc();
    EXPECT_NE(p1, nullptr);
    EXPECT_NE(p2, nullptr);
    EXPECT_NE(p1, p2);

    // もう空なので nullptr
    void* p3 = pool.alloc();
    EXPECT_EQ(p3, nullptr);

    // free して再利用
    pool.free(p1);
    void* p4 = pool.alloc();
    EXPECT_EQ(p4, p1);
}

// HeapArea のサイズとデータポインタのテスト
TEST(HeapAreaTest, AllocFree)
{
    constexpr size_t size = 128;
    {
        AreaPolicy::HeapArea area(size);
        EXPECT_NE(area.data(), nullptr);
        EXPECT_EQ(area.size(), size);
    } // 自動的に解放される
    {
        AreaPolicy::HeapArea area(0);
        EXPECT_EQ(area.data(), nullptr);
        EXPECT_EQ(area.size(), 0u);
    }
}

TEST(LinearAllocatorTest, AllocFree)
{
    constexpr size_t POOL_SIZE = 256;
    LinearAllocator<MemoryPool<32>> allocator("TestAllocator", POOL_SIZE);
    std::vector<void*> allocations;
    for (size_t i = 0; i < POOL_SIZE / 32; ++i) {
        void* p = allocator.alloc();
        EXPECT_NE(p, nullptr);
        allocations.push_back(p);
    }
    // もう空なので nullptr
    void* p = allocator.alloc();
    EXPECT_EQ(p, nullptr);
    // free して再利用
    for (void* ptr : allocations) {
        allocator.free(ptr);
    }
    // 再度全て割り当て可能か
    allocations.clear();
    for (size_t i = 0; i < POOL_SIZE / 32; ++i) {
        void* p = allocator.alloc();
        EXPECT_NE(p, nullptr);
        allocations.push_back(p);
    }
}

