#include <gtest/gtest.h>

#include <ocf/math/Rect.h>
#include <ocf/math/vec2.h>

using namespace ocf::math;

// Rectのデフォルトコンストラクタのテスト
TEST(RectTest, DefaultConstructor)
{
    Rect rect;
    
    EXPECT_FLOAT_EQ(rect.m_position.x, 0.0f);
    EXPECT_FLOAT_EQ(rect.m_position.y, 0.0f);
    EXPECT_FLOAT_EQ(rect.m_size.x, 0.0f);
    EXPECT_FLOAT_EQ(rect.m_size.y, 0.0f);
}

// Rectのパラメータ指定コンストラクタのテスト
TEST(RectTest, ParameterConstructor)
{
    Rect rect(10.0f, 20.0f, 30.0f, 40.0f);
    
    EXPECT_FLOAT_EQ(rect.m_position.x, 10.0f);
    EXPECT_FLOAT_EQ(rect.m_position.y, 20.0f);
    EXPECT_FLOAT_EQ(rect.m_size.x, 30.0f);
    EXPECT_FLOAT_EQ(rect.m_size.y, 40.0f);
}

// Rectのベクトル指定コンストラクタのテスト
TEST(RectTest, VectorConstructor)
{
    vec2 position(5.0f, 10.0f);
    vec2 size(15.0f, 20.0f);
    
    Rect rect(position, size);
    
    EXPECT_FLOAT_EQ(rect.m_position.x, 5.0f);
    EXPECT_FLOAT_EQ(rect.m_position.y, 10.0f);
    EXPECT_FLOAT_EQ(rect.m_size.x, 15.0f);
    EXPECT_FLOAT_EQ(rect.m_size.y, 20.0f);
}

// setRectメソッドのテスト
TEST(RectTest, SetRect)
{
    Rect rect;
    rect.setRect(1.0f, 2.0f, 3.0f, 4.0f);
    
    EXPECT_FLOAT_EQ(rect.m_position.x, 1.0f);
    EXPECT_FLOAT_EQ(rect.m_position.y, 2.0f);
    EXPECT_FLOAT_EQ(rect.m_size.x, 3.0f);
    EXPECT_FLOAT_EQ(rect.m_size.y, 4.0f);
}

// getMaxXメソッドのテスト
TEST(RectTest, GetMaxX)
{
    Rect rect(10.0f, 20.0f, 30.0f, 40.0f);
    
    EXPECT_FLOAT_EQ(rect.getMaxX(), 40.0f); // 10 + 30 = 40
}

// getMaxYメソッドのテスト
TEST(RectTest, GetMaxY)
{
    Rect rect(10.0f, 20.0f, 30.0f, 40.0f);
    
    EXPECT_FLOAT_EQ(rect.getMaxY(), 60.0f); // 20 + 40 = 60
}

// getMinXメソッドのテスト
TEST(RectTest, GetMinX)
{
    Rect rect(10.0f, 20.0f, 30.0f, 40.0f);
    
    EXPECT_FLOAT_EQ(rect.getMinX(), 10.0f);
}

// getMinYメソッドのテスト
TEST(RectTest, GetMinY)
{
    Rect rect(10.0f, 20.0f, 30.0f, 40.0f);
    
    EXPECT_FLOAT_EQ(rect.getMinY(), 20.0f);
}

// equalsメソッドのテスト（等しい場合）
TEST(RectTest, EqualsTrue)
{
    Rect rect1(10.0f, 20.0f, 30.0f, 40.0f);
    Rect rect2(10.0f, 20.0f, 30.0f, 40.0f);
    
    EXPECT_TRUE(rect1.equals(rect2));
}

// equalsメソッドのテスト（等しくない場合）
TEST(RectTest, EqualsFalse)
{
    Rect rect1(10.0f, 20.0f, 30.0f, 40.0f);
    Rect rect2(11.0f, 20.0f, 30.0f, 40.0f);
    
    EXPECT_FALSE(rect1.equals(rect2));
}

// intersect(vec2)メソッドのテスト（点が矩形内にある場合）
TEST(RectTest, IntersectPointInside)
{
    Rect rect(10.0f, 20.0f, 30.0f, 40.0f);
    vec2 point(25.0f, 35.0f);
    
    EXPECT_TRUE(rect.intersect(point));
}

// intersect(vec2)メソッドのテスト（点が矩形外にある場合）
TEST(RectTest, IntersectPointOutside)
{
    Rect rect(10.0f, 20.0f, 30.0f, 40.0f);
    vec2 point(5.0f, 5.0f);
    
    EXPECT_FALSE(rect.intersect(point));
}

// intersect(vec2)メソッドのテスト（点が矩形の境界上にある場合）
TEST(RectTest, IntersectPointOnBoundary)
{
    Rect rect(10.0f, 20.0f, 30.0f, 40.0f);
    vec2 point(10.0f, 20.0f); // 左下の角
    
    EXPECT_TRUE(rect.intersect(point));
    
    vec2 point2(40.0f, 60.0f); // 右上の角
    EXPECT_TRUE(rect.intersect(point2));
}

// intersect(Rect)メソッドのテスト（矩形が交差する場合）
TEST(RectTest, IntersectRectOverlapping)
{
    Rect rect1(10.0f, 10.0f, 30.0f, 30.0f);
    Rect rect2(20.0f, 20.0f, 30.0f, 30.0f);
    
    EXPECT_TRUE(rect1.intersect(rect2));
    EXPECT_TRUE(rect2.intersect(rect1));
}

// intersect(Rect)メソッドのテスト（矩形が交差しない場合）
TEST(RectTest, IntersectRectNotOverlapping)
{
    Rect rect1(10.0f, 10.0f, 10.0f, 10.0f);
    Rect rect2(30.0f, 30.0f, 10.0f, 10.0f);
    
    EXPECT_FALSE(rect1.intersect(rect2));
    EXPECT_FALSE(rect2.intersect(rect1));
}

// intersect(Rect)メソッドのテスト（矩形が接している場合）
TEST(RectTest, IntersectRectTouching)
{
    Rect rect1(10.0f, 10.0f, 10.0f, 10.0f);
    Rect rect2(20.0f, 10.0f, 10.0f, 10.0f);
    
    // 境界が接しているだけの場合は交差していないとみなす
    EXPECT_FALSE(rect1.intersect(rect2));
}

// containメソッドのテスト（矩形が完全に含まれる場合）
TEST(RectTest, ContainRectInside)
{
    Rect rect1(10.0f, 10.0f, 50.0f, 50.0f);
    Rect rect2(20.0f, 20.0f, 20.0f, 20.0f);
    
    EXPECT_TRUE(rect1.contain(rect2));
}

// containメソッドのテスト（矩形が含まれない場合）
TEST(RectTest, ContainRectOutside)
{
    Rect rect1(10.0f, 10.0f, 20.0f, 20.0f);
    Rect rect2(40.0f, 40.0f, 20.0f, 20.0f);
    
    EXPECT_FALSE(rect1.contain(rect2));
}

// containメソッドのテスト（矩形が部分的に重なる場合）
TEST(RectTest, ContainRectPartialOverlap)
{
    Rect rect1(10.0f, 10.0f, 30.0f, 30.0f);
    Rect rect2(20.0f, 20.0f, 30.0f, 30.0f);
    
    EXPECT_FALSE(rect1.contain(rect2));
}

// containメソッドのテスト（矩形が同じ場合）
TEST(RectTest, ContainRectSame)
{
    Rect rect1(10.0f, 10.0f, 30.0f, 30.0f);
    Rect rect2(10.0f, 10.0f, 30.0f, 30.0f);
    
    EXPECT_TRUE(rect1.contain(rect2));
}

// 負の座標を持つ矩形のテスト
TEST(RectTest, NegativeCoordinates)
{
    Rect rect(-10.0f, -20.0f, 30.0f, 40.0f);
    
    EXPECT_FLOAT_EQ(rect.getMinX(), -10.0f);
    EXPECT_FLOAT_EQ(rect.getMinY(), -20.0f);
    EXPECT_FLOAT_EQ(rect.getMaxX(), 20.0f);  // -10 + 30 = 20
    EXPECT_FLOAT_EQ(rect.getMaxY(), 20.0f);  // -20 + 40 = 20
}

// ゼロサイズの矩形のテスト
TEST(RectTest, ZeroSizeRect)
{
    Rect rect(10.0f, 20.0f, 0.0f, 0.0f);
    
    EXPECT_FLOAT_EQ(rect.getMinX(), rect.getMaxX());
    EXPECT_FLOAT_EQ(rect.getMinY(), rect.getMaxY());
    
    vec2 point(10.0f, 20.0f);
    EXPECT_TRUE(rect.intersect(point));
}
