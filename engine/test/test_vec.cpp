#include <gtest/gtest.h>

#include <ocf/math/vec4.h>

using namespace ocf::math;

// vec<4, T> のコンストラクタのユニットテスト
TEST(Vec4Test, ConstructorInitializesComponents)
{
    vec<4, float> v1(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v1.x, 1.0f);
    EXPECT_FLOAT_EQ(v1.y, 2.0f);
    EXPECT_FLOAT_EQ(v1.z, 3.0f);
    EXPECT_FLOAT_EQ(v1.w, 4.0f);

    vec<4, int> v2(10, 20, 30, 40);
    EXPECT_EQ(v2.x, 10);
    EXPECT_EQ(v2.y, 20);
    EXPECT_EQ(v2.z, 30);
    EXPECT_EQ(v2.w, 40);
}

// 境界値や異常値のテスト
TEST(Vec4Test, ConstructorWithNegativeAndZero)
{
    vec<4, int> v(0, -1, 100, -100);
    EXPECT_EQ(v.x, 0);
    EXPECT_EQ(v.y, -1);
    EXPECT_EQ(v.z, 100);
    EXPECT_EQ(v.w, -100);
}

// 型の違いによるテスト
TEST(Vec4Test, ConstructorWithDouble)
{
    vec<4, double> v(1.1, 2.2, 3.3, 4.4);
    EXPECT_DOUBLE_EQ(v.x, 1.1);
    EXPECT_DOUBLE_EQ(v.y, 2.2);
    EXPECT_DOUBLE_EQ(v.z, 3.3);
    EXPECT_DOUBLE_EQ(v.w, 4.4);
}
