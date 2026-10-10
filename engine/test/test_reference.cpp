// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include <ocf/core/Reference.h>

#include <thread>
#include <utility>
#include <vector>

using namespace ocf;

// Test class that derives from RefCounted
class TestRefCounted : public RefCounted {
public:
    TestRefCounted() : value(0) {}
    int value;
};

// 破棄された回数を記録するテスト用クラス
class TrackedRefCounted : public RefCounted {
public:
    explicit TrackedRefCounted(int v = 0) : value(v) { ++s_aliveCount; }
    ~TrackedRefCounted() override { --s_aliveCount; }

    int value;

    static inline int s_aliveCount = 0;
};

// TrackedRefCountedを使うテストのフィクスチャ（リークがないことを確認する）
class RefLifetimeTest : public ::testing::Test {
protected:
    void SetUp() override { TrackedRefCounted::s_aliveCount = 0; }
    void TearDown() override { EXPECT_EQ(TrackedRefCounted::s_aliveCount, 0); }
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

// Refの破棄でオブジェクトが削除されるテスト
TEST_F(RefLifetimeTest, DestructorDeletesObject)
{
    {
        Ref<TrackedRefCounted> ref(new TrackedRefCounted());
        EXPECT_EQ(TrackedRefCounted::s_aliveCount, 1);
    }
    EXPECT_EQ(TrackedRefCounted::s_aliveCount, 0);
}

// Ref同士のコピーコンストラクタのテスト
TEST_F(RefLifetimeTest, CopyConstructorFromRef)
{
    Ref<TrackedRefCounted> ref1(new TrackedRefCounted());
    {
        Ref<TrackedRefCounted> ref2(ref1);
        EXPECT_EQ(ref2.ptr(), ref1.ptr());
        EXPECT_EQ(ref1->getReferenceCount(), 2U);
    }
    EXPECT_EQ(ref1->getReferenceCount(), 1U);
    EXPECT_EQ(TrackedRefCounted::s_aliveCount, 1);
}

// コピー代入で古い参照が解放されるテスト
TEST_F(RefLifetimeTest, CopyAssignmentReleasesOldReference)
{
    Ref<TrackedRefCounted> ref1(new TrackedRefCounted(1));
    Ref<TrackedRefCounted> ref2(new TrackedRefCounted(2));

    ref1 = ref2;
    EXPECT_EQ(TrackedRefCounted::s_aliveCount, 1);
    EXPECT_EQ(ref1->value, 2);
    EXPECT_EQ(ref2->getReferenceCount(), 2U);
}

// 空のRefをコピー代入すると古い参照が解放されるテスト
TEST_F(RefLifetimeTest, CopyAssignmentFromNull)
{
    Ref<TrackedRefCounted> ref(new TrackedRefCounted());
    Ref<TrackedRefCounted> empty;

    ref = empty;
    EXPECT_EQ(ref.ptr(), nullptr);
    EXPECT_EQ(TrackedRefCounted::s_aliveCount, 0);
}

// 同じオブジェクトを指すRefのコピー代入で参照カウントが変わらないテスト
TEST_F(RefLifetimeTest, CopyAssignmentSameObject)
{
    Ref<TrackedRefCounted> ref1(new TrackedRefCounted());
    Ref<TrackedRefCounted> ref2(ref1);

    ref1 = ref2;
    EXPECT_EQ(ref1->getReferenceCount(), 2U);
}

// 自己代入のテスト
TEST_F(RefLifetimeTest, SelfAssignment)
{
    Ref<TrackedRefCounted> ref(new TrackedRefCounted());
    Ref<TrackedRefCounted>& alias = ref;

    ref = alias;
    EXPECT_EQ(ref->getReferenceCount(), 1U);
    EXPECT_EQ(TrackedRefCounted::s_aliveCount, 1);
}

// ムーブ代入で古い参照が解放されるテスト
TEST_F(RefLifetimeTest, MoveAssignmentReleasesOldReference)
{
    Ref<TrackedRefCounted> ref1(new TrackedRefCounted(1));
    Ref<TrackedRefCounted> ref2(new TrackedRefCounted(2));

    ref1 = std::move(ref2);
    EXPECT_EQ(TrackedRefCounted::s_aliveCount, 1);
    EXPECT_EQ(ref2.ptr(), nullptr);
    EXPECT_EQ(ref1->value, 2);
    EXPECT_EQ(ref1->getReferenceCount(), 1U);
}

// 同じオブジェクトを指すRefのムーブ代入で両方の参照が保持されるテスト
TEST_F(RefLifetimeTest, MoveAssignmentSameObject)
{
    Ref<TrackedRefCounted> ref1(new TrackedRefCounted());
    Ref<TrackedRefCounted> ref2(ref1);

    ref1 = std::move(ref2);
    EXPECT_EQ(ref1->getReferenceCount(), 2U);
    EXPECT_EQ(TrackedRefCounted::s_aliveCount, 1);
}

// 生ポインタの代入で古い参照が解放されるテスト
TEST_F(RefLifetimeTest, AssignmentFromRawPointer)
{
    Ref<TrackedRefCounted> ref(new TrackedRefCounted(1));

    ref = new TrackedRefCounted(2);
    EXPECT_EQ(TrackedRefCounted::s_aliveCount, 1);
    EXPECT_EQ(ref->value, 2);
    EXPECT_EQ(ref->getReferenceCount(), 1U);
}

// instantiate()にコンストラクタ引数を渡すテスト
TEST_F(RefLifetimeTest, InstantiateWithArguments)
{
    Ref<TrackedRefCounted> ref;
    ref.instantiate(7);

    EXPECT_EQ(ref->value, 7);
    EXPECT_EQ(ref->getReferenceCount(), 1U);
}

// instantiate()で古い参照が解放されるテスト
TEST_F(RefLifetimeTest, InstantiateReleasesOldReference)
{
    Ref<TrackedRefCounted> ref(new TrackedRefCounted(1));

    ref.instantiate(2);
    EXPECT_EQ(TrackedRefCounted::s_aliveCount, 1);
    EXPECT_EQ(ref->value, 2);
}

// 手動でretainしたオブジェクトがRefの破棄後も生存するテスト
TEST_F(RefLifetimeTest, ManualRetainKeepsObjectAlive)
{
    TrackedRefCounted* obj = nullptr;
    {
        Ref<TrackedRefCounted> ref(new TrackedRefCounted());
        obj = ref.ptr();
        obj->retain();
    }
    EXPECT_EQ(TrackedRefCounted::s_aliveCount, 1);
    EXPECT_EQ(obj->getReferenceCount(), 1U);

    obj->release();
    EXPECT_EQ(TrackedRefCounted::s_aliveCount, 0);
}

// Ref同士の比較演算子とnullptrとの比較のテスト
TEST_F(RefLifetimeTest, ComparisonBetweenRefs)
{
    Ref<TrackedRefCounted> ref1(new TrackedRefCounted());
    Ref<TrackedRefCounted> ref2(ref1);
    Ref<TrackedRefCounted> ref3(new TrackedRefCounted());
    Ref<TrackedRefCounted> empty;

    EXPECT_TRUE(ref1 == ref2);
    EXPECT_FALSE(ref1 != ref2);
    EXPECT_TRUE(ref1 != ref3);
    EXPECT_TRUE(ref1 != nullptr);
    EXPECT_TRUE(empty == nullptr);
}
