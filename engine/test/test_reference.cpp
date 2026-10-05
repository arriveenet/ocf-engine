// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include <ocf/core/Reference.h>

using namespace ocf;

// Test class that derives from RefCounted
class TestRefCounted : public RefCounted {
public:
    TestRefCounted() : value(0) {}
    int value;
};

// RefCountedのデフォルトコンストラクタのテスト
TEST(RefCountedTest, DefaultConstructor)
{
    TestRefCounted* obj = new TestRefCounted();
    
    // 初期参照カウントは0
    EXPECT_EQ(obj->getReferenceCount(), 0U);
    
    delete obj;
}

// RefCountedのretainのテスト
TEST(RefCountedTest, Retain)
{
    TestRefCounted* obj = new TestRefCounted();
    
    obj->retain();
    EXPECT_EQ(obj->getReferenceCount(), 1U);
    
    obj->retain();
    EXPECT_EQ(obj->getReferenceCount(), 2U);
    
    obj->release();
    obj->release();
}

// RefCountedのreleaseのテスト
TEST(RefCountedTest, Release)
{
    TestRefCounted* obj = new TestRefCounted();
    
    obj->retain();
    EXPECT_EQ(obj->getReferenceCount(), 1U);
    
    obj->release();
    // オブジェクトは削除されているので、これ以上のテストはできない
}

// RefCountedの複数回のretain/releaseのテスト
TEST(RefCountedTest, MultipleRetainRelease)
{
    TestRefCounted* obj = new TestRefCounted();
    
    obj->retain();
    obj->retain();
    obj->retain();
    EXPECT_EQ(obj->getReferenceCount(), 3U);
    
    obj->release();
    EXPECT_EQ(obj->getReferenceCount(), 2U);
    
    obj->release();
    EXPECT_EQ(obj->getReferenceCount(), 1U);
    
    obj->release();
    // オブジェクトは削除されている
}

// Refのデフォルトコンストラクタのテスト
TEST(RefTest, DefaultConstructor)
{
    Ref<TestRefCounted> ref;
    
    EXPECT_EQ(ref.ptr(), nullptr);
}

// Refのポインタコンストラクタのテスト
TEST(RefTest, PointerConstructor)
{
    TestRefCounted* obj = new TestRefCounted();
    
    {
        Ref<TestRefCounted> ref(obj);
        EXPECT_EQ(ref.ptr(), obj);
        EXPECT_EQ(obj->getReferenceCount(), 1U);
    }
    // refがスコープを抜けた後、オブジェクトは削除される
}

// Refのコピーコンストラクタのテスト
TEST(RefTest, CopyConstructor)
{
    TestRefCounted* obj = new TestRefCounted();
    
    {
        Ref<TestRefCounted> ref1(obj);
        EXPECT_EQ(obj->getReferenceCount(), 1U);
        
        {
            Ref<TestRefCounted> ref2(obj);
            EXPECT_EQ(ref2.ptr(), obj);
            EXPECT_EQ(obj->getReferenceCount(), 2U);
        }
        
        EXPECT_EQ(obj->getReferenceCount(), 1U);
    }
}

// Refのムーブコンストラクタのテスト
TEST(RefTest, MoveConstructor)
{
    TestRefCounted* obj = new TestRefCounted();
    
    {
        Ref<TestRefCounted> ref1(obj);
        EXPECT_EQ(obj->getReferenceCount(), 1U);
        
        Ref<TestRefCounted> ref2(std::move(ref1));
        EXPECT_EQ(ref2.ptr(), obj);
        EXPECT_EQ(ref1.ptr(), nullptr);
        EXPECT_EQ(obj->getReferenceCount(), 1U);
    }
}

// Refのデストラクタのテスト
TEST(RefTest, Destructor)
{
    TestRefCounted* obj = new TestRefCounted();
    
    {
        Ref<TestRefCounted> ref(obj);
        EXPECT_EQ(obj->getReferenceCount(), 1U);
    }
    // refがスコープを抜けた後、オブジェクトは自動的に削除される
}

// Refの等価比較演算子のテスト
TEST(RefTest, EqualityOperator)
{
    TestRefCounted* obj1 = new TestRefCounted();
    TestRefCounted* obj2 = new TestRefCounted();
    
    {
        Ref<TestRefCounted> ref1(obj1);
        Ref<TestRefCounted> ref2(obj2);
        
        EXPECT_TRUE(ref1 == obj1);
        EXPECT_FALSE(ref1 == obj2);
    }
}

// Refの非等価比較演算子のテスト
TEST(RefTest, InequalityOperator)
{
    TestRefCounted* obj1 = new TestRefCounted();
    TestRefCounted* obj2 = new TestRefCounted();
    
    {
        Ref<TestRefCounted> ref1(obj1);
        Ref<TestRefCounted> ref2(obj2);
        
        EXPECT_FALSE(ref1 != obj1);
        EXPECT_TRUE(ref1 != obj2);
    }
}

// Refのデリファレンス演算子のテスト
TEST(RefTest, DereferenceOperator)
{
    TestRefCounted* obj = new TestRefCounted();
    obj->value = 42;
    
    {
        Ref<TestRefCounted> ref(obj);
        EXPECT_EQ((*ref)->value, 42);
    }
}

// Refのアロー演算子のテスト
TEST(RefTest, ArrowOperator)
{
    TestRefCounted* obj = new TestRefCounted();
    obj->value = 123;
    
    {
        Ref<TestRefCounted> ref(obj);
        EXPECT_EQ(ref->value, 123);
        
        ref->value = 456;
        EXPECT_EQ(ref->value, 456);
    }
}

// Refのptr()メソッドのテスト
TEST(RefTest, PtrMethod)
{
    TestRefCounted* obj = new TestRefCounted();
    
    {
        Ref<TestRefCounted> ref(obj);
        EXPECT_EQ(ref.ptr(), obj);
    }
}

// Refのinstantiate()メソッドのテスト
TEST(RefTest, InstantiateMethod)
{
    Ref<TestRefCounted> ref;
    ref.instantiate();
    
    EXPECT_NE(ref.ptr(), nullptr);
    EXPECT_EQ(ref.ptr()->getReferenceCount(), 1U);
}

// Refの複数の参照を持つテスト
TEST(RefTest, MultipleReferences)
{
    TestRefCounted* obj = new TestRefCounted();
    
    {
        Ref<TestRefCounted> ref1(obj);
        Ref<TestRefCounted> ref2(obj);
        Ref<TestRefCounted> ref3(obj);
        
        EXPECT_EQ(obj->getReferenceCount(), 3U);
    }
    // すべての参照がスコープを抜けた後、オブジェクトは削除される
}

// スレッド安全性のテスト
TEST(RefTest, ThreadSafety)
{
    TestRefCounted* obj = new TestRefCounted();

    const int numThreads = 10;
    const int numIterations = 1000;

    Ref<TestRefCounted> initialRef(obj);

    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([obj, numIterations]() {
            for (int j = 0; j < numIterations; ++j) {
                Ref<TestRefCounted> ref(obj);
                EXPECT_EQ(ref.ptr(), obj);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 最後に参照カウントが1であることを確認
    EXPECT_EQ(obj->getReferenceCount(), 1U);
}
