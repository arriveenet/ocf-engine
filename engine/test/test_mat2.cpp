#include <gtest/gtest.h>

#include <ocf/math/mat2.h>
#include <ocf/math/vec2.h>

using namespace ocf::math;

// mat2のデフォルトコンストラクタのテスト
TEST(Mat2Test, DefaultConstructor)
{
    mat2 m;
    // デフォルトコンストラクタで初期化された値をテスト
}

// mat2のスカラーコンストラクタのテスト（単位行列）
TEST(Mat2Test, ScalarConstructorIdentity)
{
    mat2 m(1.0f);
    
    // 対角成分が1であることを確認
    EXPECT_FLOAT_EQ(m[0].x, 1.0f);
    EXPECT_FLOAT_EQ(m[1].y, 1.0f);
    
    // 非対角成分が0であることを確認
    EXPECT_FLOAT_EQ(m[0].y, 0.0f);
    EXPECT_FLOAT_EQ(m[1].x, 0.0f);
}

// mat2のスカラーコンストラクタのテスト（任意の値）
TEST(Mat2Test, ScalarConstructorArbitraryValue)
{
    mat2 m(2.5f);
    
    // 対角成分が2.5であることを確認
    EXPECT_FLOAT_EQ(m[0].x, 2.5f);
    EXPECT_FLOAT_EQ(m[1].y, 2.5f);
    
    // 非対角成分が0であることを確認
    EXPECT_FLOAT_EQ(m[0].y, 0.0f);
    EXPECT_FLOAT_EQ(m[1].x, 0.0f);
}

// mat2の成分指定コンストラクタのテスト
TEST(Mat2Test, ComponentConstructor)
{
    mat2 m(1.0f, 2.0f,
           3.0f, 4.0f);
    
    EXPECT_FLOAT_EQ(m[0].x, 1.0f);
    EXPECT_FLOAT_EQ(m[0].y, 2.0f);
    EXPECT_FLOAT_EQ(m[1].x, 3.0f);
    EXPECT_FLOAT_EQ(m[1].y, 4.0f);
}

// mat2のカラムベクターコンストラクタのテスト
TEST(Mat2Test, ColumnConstructor)
{
    vec2 col0(1.0f, 2.0f);
    vec2 col1(3.0f, 4.0f);
    mat2 m(col0, col1);
    
    EXPECT_FLOAT_EQ(m[0].x, 1.0f);
    EXPECT_FLOAT_EQ(m[0].y, 2.0f);
    EXPECT_FLOAT_EQ(m[1].x, 3.0f);
    EXPECT_FLOAT_EQ(m[1].y, 4.0f);
}

// 配列アクセス演算子のテスト
TEST(Mat2Test, ArrayAccessOperator)
{
    mat2 m(1.0f, 2.0f,
           3.0f, 4.0f);
    
    EXPECT_FLOAT_EQ(m[0].x, 1.0f);
    EXPECT_FLOAT_EQ(m[0].y, 2.0f);
    EXPECT_FLOAT_EQ(m[1].x, 3.0f);
    EXPECT_FLOAT_EQ(m[1].y, 4.0f);
}

// 行列加算のテスト
TEST(Mat2Test, MatrixAddition)
{
    mat2 m1(1.0f, 2.0f,
            3.0f, 4.0f);
    
    mat2 m2(1.0f, 1.0f,
            1.0f, 1.0f);
    
    mat2 result = m1 + m2;
    
    EXPECT_FLOAT_EQ(result[0].x, 2.0f);
    EXPECT_FLOAT_EQ(result[0].y, 3.0f);
    EXPECT_FLOAT_EQ(result[1].x, 4.0f);
    EXPECT_FLOAT_EQ(result[1].y, 5.0f);
}

// 行列減算のテスト
TEST(Mat2Test, MatrixSubtraction)
{
    mat2 m1(3.0f, 4.0f,
            5.0f, 6.0f);
    
    mat2 m2(1.0f, 1.0f,
            1.0f, 1.0f);
    
    mat2 result = m1 - m2;
    
    EXPECT_FLOAT_EQ(result[0].x, 2.0f);
    EXPECT_FLOAT_EQ(result[0].y, 3.0f);
    EXPECT_FLOAT_EQ(result[1].x, 4.0f);
    EXPECT_FLOAT_EQ(result[1].y, 5.0f);
}

// 行列とスカラーの乗算のテスト
TEST(Mat2Test, MatrixScalarMultiplication)
{
    mat2 m(1.0f, 2.0f,
           3.0f, 4.0f);
    
    mat2 result = m * 2.0f;
    
    EXPECT_FLOAT_EQ(result[0].x, 2.0f);
    EXPECT_FLOAT_EQ(result[0].y, 4.0f);
    EXPECT_FLOAT_EQ(result[1].x, 6.0f);
    EXPECT_FLOAT_EQ(result[1].y, 8.0f);
}

// 行列乗算のテスト（単位行列）
TEST(Mat2Test, MatrixMultiplicationIdentity)
{
    mat2 m(2.0f, 3.0f,
           4.0f, 5.0f);
    
    mat2 identity(1.0f);
    
    mat2 result = m * identity;
    
    EXPECT_FLOAT_EQ(result[0].x, 2.0f);
    EXPECT_FLOAT_EQ(result[0].y, 3.0f);
    EXPECT_FLOAT_EQ(result[1].x, 4.0f);
    EXPECT_FLOAT_EQ(result[1].y, 5.0f);
}

// 行列とベクトルの乗算のテスト
TEST(Mat2Test, MatrixVectorMultiplication)
{
    mat2 m(1.0f, 0.0f,
           0.0f, 2.0f);
    
    vec2 v(3.0f, 4.0f);
    
    vec2 result = m * v;
    
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 8.0f);
}

// 等価比較演算子のテスト
TEST(Mat2Test, EqualityOperator)
{
    mat2 m1(1.0f, 2.0f,
            3.0f, 4.0f);
    
    mat2 m2(1.0f, 2.0f,
            3.0f, 4.0f);
    
    EXPECT_TRUE(m1 == m2);
}

// 非等価比較演算子のテスト
TEST(Mat2Test, InequalityOperator)
{
    mat2 m1(1.0f, 2.0f,
            3.0f, 4.0f);
    
    mat2 m2(1.0f, 2.0f,
            3.0f, 5.0f);
    
    EXPECT_TRUE(m1 != m2);
}

// 転置行列のテスト
TEST(Mat2Test, Transpose)
{
    mat2 m(1.0f, 2.0f,
           3.0f, 4.0f);
    
    mat2 result = transpose(m);
    
    EXPECT_FLOAT_EQ(result[0].x, 1.0f);
    EXPECT_FLOAT_EQ(result[0].y, 3.0f);
    EXPECT_FLOAT_EQ(result[1].x, 2.0f);
    EXPECT_FLOAT_EQ(result[1].y, 4.0f);
}

// 行列式のテスト
TEST(Mat2Test, Determinant)
{
    mat2 m(1.0f, 2.0f,
           3.0f, 4.0f);
    
    float det = determinant(m);
    
    EXPECT_FLOAT_EQ(det, -2.0f);  // 1*4 - 2*3 = -2
}

// 逆行列のテスト
TEST(Mat2Test, Inverse)
{
    mat2 m(1.0f, 2.0f,
           3.0f, 4.0f);
    
    mat2 inv = inverse(m);
    mat2 identity = m * inv;
    
    // 逆行列との乗算が単位行列になることを確認（許容誤差あり）
    EXPECT_NEAR(identity[0].x, 1.0f, 1e-6f);
    EXPECT_NEAR(identity[0].y, 0.0f, 1e-6f);
    EXPECT_NEAR(identity[1].x, 0.0f, 1e-6f);
    EXPECT_NEAR(identity[1].y, 1.0f, 1e-6f);
}

// 型エイリアスのテスト
TEST(Mat2Test, TypeAliases)
{
    mat2 m_float(1.0f);
    dmat2 m_double(1.0);
    imat2 m_int(1);
    
    EXPECT_FLOAT_EQ(m_float[0].x, 1.0f);
    EXPECT_DOUBLE_EQ(m_double[0].x, 1.0);
    EXPECT_EQ(m_int[0].x, 1);
}