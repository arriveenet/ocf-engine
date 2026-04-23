#include <gtest/gtest.h>

#include <ocf/math/mat3.h>
#include <ocf/math/vec3.h>

using namespace ocf::math;

// mat3のデフォルトコンストラクタのテスト
TEST(Mat3Test, DefaultConstructor)
{
    mat3 m;
    // デフォルトコンストラクタで初期化された値をテスト
}

// mat3のスカラーコンストラクタのテスト（単位行列）
TEST(Mat3Test, ScalarConstructorIdentity)
{
    mat3 m(1.0f);
    
    // 対角成分が1であることを確認
    EXPECT_FLOAT_EQ(m[0].x, 1.0f);
    EXPECT_FLOAT_EQ(m[1].y, 1.0f);
    EXPECT_FLOAT_EQ(m[2].z, 1.0f);
    
    // 非対角成分が0であることを確認
    EXPECT_FLOAT_EQ(m[0].y, 0.0f);
    EXPECT_FLOAT_EQ(m[0].z, 0.0f);
    EXPECT_FLOAT_EQ(m[1].x, 0.0f);
    EXPECT_FLOAT_EQ(m[1].z, 0.0f);
    EXPECT_FLOAT_EQ(m[2].x, 0.0f);
    EXPECT_FLOAT_EQ(m[2].y, 0.0f);
}

// mat3のスカラーコンストラクタのテスト（任意の値）
TEST(Mat3Test, ScalarConstructorArbitraryValue)
{
    mat3 m(3.5f);
    
    // 対角成分が3.5であることを確認
    EXPECT_FLOAT_EQ(m[0].x, 3.5f);
    EXPECT_FLOAT_EQ(m[1].y, 3.5f);
    EXPECT_FLOAT_EQ(m[2].z, 3.5f);
    
    // 非対角成分が0であることを確認
    EXPECT_FLOAT_EQ(m[0].y, 0.0f);
    EXPECT_FLOAT_EQ(m[0].z, 0.0f);
    EXPECT_FLOAT_EQ(m[1].x, 0.0f);
    EXPECT_FLOAT_EQ(m[1].z, 0.0f);
    EXPECT_FLOAT_EQ(m[2].x, 0.0f);
    EXPECT_FLOAT_EQ(m[2].y, 0.0f);
}

// mat3の成分指定コンストラクタのテスト
TEST(Mat3Test, ComponentConstructor)
{
    mat3 m(1.0f, 2.0f, 3.0f,
           4.0f, 5.0f, 6.0f,
           7.0f, 8.0f, 9.0f);
    
    EXPECT_FLOAT_EQ(m[0].x, 1.0f);
    EXPECT_FLOAT_EQ(m[0].y, 2.0f);
    EXPECT_FLOAT_EQ(m[0].z, 3.0f);
    EXPECT_FLOAT_EQ(m[1].x, 4.0f);
    EXPECT_FLOAT_EQ(m[1].y, 5.0f);
    EXPECT_FLOAT_EQ(m[1].z, 6.0f);
    EXPECT_FLOAT_EQ(m[2].x, 7.0f);
    EXPECT_FLOAT_EQ(m[2].y, 8.0f);
    EXPECT_FLOAT_EQ(m[2].z, 9.0f);
}

// mat3のカラムベクターコンストラクタのテスト
TEST(Mat3Test, ColumnConstructor)
{
    vec3 col0(1.0f, 2.0f, 3.0f);
    vec3 col1(4.0f, 5.0f, 6.0f);
    vec3 col2(7.0f, 8.0f, 9.0f);
    mat3 m(col0, col1, col2);
    
    EXPECT_FLOAT_EQ(m[0].x, 1.0f);
    EXPECT_FLOAT_EQ(m[0].y, 2.0f);
    EXPECT_FLOAT_EQ(m[0].z, 3.0f);
    EXPECT_FLOAT_EQ(m[1].x, 4.0f);
    EXPECT_FLOAT_EQ(m[1].y, 5.0f);
    EXPECT_FLOAT_EQ(m[1].z, 6.0f);
    EXPECT_FLOAT_EQ(m[2].x, 7.0f);
    EXPECT_FLOAT_EQ(m[2].y, 8.0f);
    EXPECT_FLOAT_EQ(m[2].z, 9.0f);
}

// 配列アクセス演算子のテスト
TEST(Mat3Test, ArrayAccessOperator)
{
    mat3 m(1.0f, 2.0f, 3.0f,
           4.0f, 5.0f, 6.0f,
           7.0f, 8.0f, 9.0f);
    
    EXPECT_FLOAT_EQ(m[0].x, 1.0f);
    EXPECT_FLOAT_EQ(m[0].y, 2.0f);
    EXPECT_FLOAT_EQ(m[0].z, 3.0f);
    EXPECT_FLOAT_EQ(m[1].x, 4.0f);
    EXPECT_FLOAT_EQ(m[1].y, 5.0f);
    EXPECT_FLOAT_EQ(m[1].z, 6.0f);
    EXPECT_FLOAT_EQ(m[2].x, 7.0f);
    EXPECT_FLOAT_EQ(m[2].y, 8.0f);
    EXPECT_FLOAT_EQ(m[2].z, 9.0f);
}

// 行列加算のテスト
TEST(Mat3Test, MatrixAddition)
{
    mat3 m1(1.0f, 2.0f, 3.0f,
            4.0f, 5.0f, 6.0f,
            7.0f, 8.0f, 9.0f);
    
    mat3 m2(1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f);
    
    mat3 result = m1 + m2;
    
    EXPECT_FLOAT_EQ(result[0].x, 2.0f);
    EXPECT_FLOAT_EQ(result[0].y, 3.0f);
    EXPECT_FLOAT_EQ(result[0].z, 4.0f);
    EXPECT_FLOAT_EQ(result[1].x, 5.0f);
    EXPECT_FLOAT_EQ(result[1].y, 6.0f);
    EXPECT_FLOAT_EQ(result[1].z, 7.0f);
    EXPECT_FLOAT_EQ(result[2].x, 8.0f);
    EXPECT_FLOAT_EQ(result[2].y, 9.0f);
    EXPECT_FLOAT_EQ(result[2].z, 10.0f);
}

// 行列減算のテスト
TEST(Mat3Test, MatrixSubtraction)
{
    mat3 m1(3.0f, 4.0f, 5.0f,
            6.0f, 7.0f, 8.0f,
            9.0f, 10.0f, 11.0f);
    
    mat3 m2(1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f);
    
    mat3 result = m1 - m2;
    
    EXPECT_FLOAT_EQ(result[0].x, 2.0f);
    EXPECT_FLOAT_EQ(result[0].y, 3.0f);
    EXPECT_FLOAT_EQ(result[0].z, 4.0f);
    EXPECT_FLOAT_EQ(result[1].x, 5.0f);
    EXPECT_FLOAT_EQ(result[1].y, 6.0f);
    EXPECT_FLOAT_EQ(result[1].z, 7.0f);
    EXPECT_FLOAT_EQ(result[2].x, 8.0f);
    EXPECT_FLOAT_EQ(result[2].y, 9.0f);
    EXPECT_FLOAT_EQ(result[2].z, 10.0f);
}

// 行列とスカラーの乗算のテスト
TEST(Mat3Test, MatrixScalarMultiplication)
{
    mat3 m(1.0f, 2.0f, 3.0f,
           4.0f, 5.0f, 6.0f,
           7.0f, 8.0f, 9.0f);
    
    mat3 result = m * 2.0f;
    
    EXPECT_FLOAT_EQ(result[0].x, 2.0f);
    EXPECT_FLOAT_EQ(result[0].y, 4.0f);
    EXPECT_FLOAT_EQ(result[0].z, 6.0f);
    EXPECT_FLOAT_EQ(result[1].x, 8.0f);
    EXPECT_FLOAT_EQ(result[1].y, 10.0f);
    EXPECT_FLOAT_EQ(result[1].z, 12.0f);
    EXPECT_FLOAT_EQ(result[2].x, 14.0f);
    EXPECT_FLOAT_EQ(result[2].y, 16.0f);
    EXPECT_FLOAT_EQ(result[2].z, 18.0f);
}

// 行列乗算のテスト（単位行列）
TEST(Mat3Test, MatrixMultiplicationIdentity)
{
    mat3 m(2.0f, 3.0f, 4.0f,
           5.0f, 6.0f, 7.0f,
           8.0f, 9.0f, 10.0f);
    
    mat3 identity(1.0f);
    
    mat3 result = m * identity;
    
    EXPECT_FLOAT_EQ(result[0].x, 2.0f);
    EXPECT_FLOAT_EQ(result[0].y, 3.0f);
    EXPECT_FLOAT_EQ(result[0].z, 4.0f);
    EXPECT_FLOAT_EQ(result[1].x, 5.0f);
    EXPECT_FLOAT_EQ(result[1].y, 6.0f);
    EXPECT_FLOAT_EQ(result[1].z, 7.0f);
    EXPECT_FLOAT_EQ(result[2].x, 8.0f);
    EXPECT_FLOAT_EQ(result[2].y, 9.0f);
    EXPECT_FLOAT_EQ(result[2].z, 10.0f);
}

// 行列とベクトルの乗算のテスト
TEST(Mat3Test, MatrixVectorMultiplication)
{
    mat3 m(1.0f, 0.0f, 0.0f,
           0.0f, 2.0f, 0.0f,
           0.0f, 0.0f, 3.0f);
    
    vec3 v(4.0f, 5.0f, 6.0f);
    
    vec3 result = m * v;
    
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 10.0f);
    EXPECT_FLOAT_EQ(result.z, 18.0f);
}

// 等価比較演算子のテスト
TEST(Mat3Test, EqualityOperator)
{
    mat3 m1(1.0f, 2.0f, 3.0f,
            4.0f, 5.0f, 6.0f,
            7.0f, 8.0f, 9.0f);
    
    mat3 m2(1.0f, 2.0f, 3.0f,
            4.0f, 5.0f, 6.0f,
            7.0f, 8.0f, 9.0f);
    
    EXPECT_TRUE(m1 == m2);
}

// 非等価比較演算子のテスト
TEST(Mat3Test, InequalityOperator)
{
    mat3 m1(1.0f, 2.0f, 3.0f,
            4.0f, 5.0f, 6.0f,
            7.0f, 8.0f, 9.0f);
    
    mat3 m2(1.0f, 2.0f, 3.0f,
            4.0f, 5.0f, 6.0f,
            7.0f, 8.0f, 10.0f);
    
    EXPECT_TRUE(m1 != m2);
}

// 転置行列のテスト
TEST(Mat3Test, Transpose)
{
    mat3 m(1.0f, 2.0f, 3.0f,
           4.0f, 5.0f, 6.0f,
           7.0f, 8.0f, 9.0f);
    
    mat3 result = transpose(m);
    
    EXPECT_FLOAT_EQ(result[0].x, 1.0f);
    EXPECT_FLOAT_EQ(result[0].y, 4.0f);
    EXPECT_FLOAT_EQ(result[0].z, 7.0f);
    EXPECT_FLOAT_EQ(result[1].x, 2.0f);
    EXPECT_FLOAT_EQ(result[1].y, 5.0f);
    EXPECT_FLOAT_EQ(result[1].z, 8.0f);
    EXPECT_FLOAT_EQ(result[2].x, 3.0f);
    EXPECT_FLOAT_EQ(result[2].y, 6.0f);
    EXPECT_FLOAT_EQ(result[2].z, 9.0f);
}

// 行列式のテスト
TEST(Mat3Test, Determinant)
{
    mat3 m(1.0f, 2.0f, 3.0f,
           0.0f, 1.0f, 4.0f,
           5.0f, 6.0f, 0.0f);
    
    float det = determinant(m);
    
    EXPECT_FLOAT_EQ(det, 1.0f);  // 1*(1*0 - 4*6) - 0 + 5*(2*4 - 3*1) = -24 + 0 + 25 = 1
}

// 逆行列のテスト
TEST(Mat3Test, Inverse)
{
    mat3 m(1.0f, 2.0f, 3.0f,
           0.0f, 1.0f, 4.0f,
           5.0f, 6.0f, 0.0f);
    
    mat3 inv = inverse(m);
    mat3 identity = m * inv;
    
    // 逆行列との乗算が単位行列になることを確認（許容誤差あり）
    EXPECT_NEAR(identity[0].x, 1.0f, 1e-5f);
    EXPECT_NEAR(identity[0].y, 0.0f, 1e-5f);
    EXPECT_NEAR(identity[0].z, 0.0f, 1e-5f);
    EXPECT_NEAR(identity[1].x, 0.0f, 1e-5f);
    EXPECT_NEAR(identity[1].y, 1.0f, 1e-5f);
    EXPECT_NEAR(identity[1].z, 0.0f, 1e-5f);
    EXPECT_NEAR(identity[2].x, 0.0f, 1e-5f);
    EXPECT_NEAR(identity[2].y, 0.0f, 1e-5f);
    EXPECT_NEAR(identity[2].z, 1.0f, 1e-5f);
}

// 型エイリアスのテスト
TEST(Mat3Test, TypeAliases)
{
    mat3 m_float(1.0f);
    dmat3 m_double(1.0);
    imat3 m_int(1);
    
    EXPECT_FLOAT_EQ(m_float[0].x, 1.0f);
    EXPECT_DOUBLE_EQ(m_double[0].x, 1.0);
    EXPECT_EQ(m_int[0].x, 1);
}