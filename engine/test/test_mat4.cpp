#include <gtest/gtest.h>

#include <ocf/math/mat4.h>
#include <ocf/math/vec4.h>

using namespace ocf::math;

// mat4のデフォルトコンストラクタのテスト
TEST(Mat4Test, DefaultConstructor)
{
    mat4 m;
    // デフォルトコンストラクタで初期化された値をテスト
}

// mat4のスカラーコンストラクタのテスト（単位行列）
TEST(Mat4Test, ScalarConstructorIdentity)
{
    mat4 m(1.0f);
    
    // 対角成分が1であることを確認
    EXPECT_FLOAT_EQ(m[0].x, 1.0f);
    EXPECT_FLOAT_EQ(m[1].y, 1.0f);
    EXPECT_FLOAT_EQ(m[2].z, 1.0f);
    EXPECT_FLOAT_EQ(m[3].w, 1.0f);
    
    // 非対角成分が0であることを確認
    EXPECT_FLOAT_EQ(m[0].y, 0.0f);
    EXPECT_FLOAT_EQ(m[0].z, 0.0f);
    EXPECT_FLOAT_EQ(m[0].w, 0.0f);
    EXPECT_FLOAT_EQ(m[1].x, 0.0f);
    EXPECT_FLOAT_EQ(m[1].z, 0.0f);
    EXPECT_FLOAT_EQ(m[1].w, 0.0f);
    EXPECT_FLOAT_EQ(m[2].x, 0.0f);
    EXPECT_FLOAT_EQ(m[2].y, 0.0f);
    EXPECT_FLOAT_EQ(m[2].w, 0.0f);
    EXPECT_FLOAT_EQ(m[3].x, 0.0f);
    EXPECT_FLOAT_EQ(m[3].y, 0.0f);
    EXPECT_FLOAT_EQ(m[3].z, 0.0f);
}

// mat4のスカラーコンストラクタのテスト（任意の値）
TEST(Mat4Test, ScalarConstructorArbitraryValue)
{
    mat4 m(2.5f);
    
    // 対角成分が2.5であることを確認
    EXPECT_FLOAT_EQ(m[0].x, 2.5f);
    EXPECT_FLOAT_EQ(m[1].y, 2.5f);
    EXPECT_FLOAT_EQ(m[2].z, 2.5f);
    EXPECT_FLOAT_EQ(m[3].w, 2.5f);
    
    // 非対角成分が0であることを確認
    EXPECT_FLOAT_EQ(m[0].y, 0.0f);
    EXPECT_FLOAT_EQ(m[1].x, 0.0f);
}

// mat4の成分指定コンストラクタのテスト
TEST(Mat4Test, ComponentConstructor)
{
    mat4 m(1.0f, 2.0f, 3.0f, 4.0f,
           5.0f, 6.0f, 7.0f, 8.0f,
           9.0f, 10.0f, 11.0f, 12.0f,
           13.0f, 14.0f, 15.0f, 16.0f);
    
    EXPECT_FLOAT_EQ(m[0].x, 1.0f);
    EXPECT_FLOAT_EQ(m[0].y, 2.0f);
    EXPECT_FLOAT_EQ(m[0].z, 3.0f);
    EXPECT_FLOAT_EQ(m[0].w, 4.0f);
    
    EXPECT_FLOAT_EQ(m[1].x, 5.0f);
    EXPECT_FLOAT_EQ(m[1].y, 6.0f);
    EXPECT_FLOAT_EQ(m[1].z, 7.0f);
    EXPECT_FLOAT_EQ(m[1].w, 8.0f);
    
    EXPECT_FLOAT_EQ(m[2].x, 9.0f);
    EXPECT_FLOAT_EQ(m[2].y, 10.0f);
    EXPECT_FLOAT_EQ(m[2].z, 11.0f);
    EXPECT_FLOAT_EQ(m[2].w, 12.0f);
    
    EXPECT_FLOAT_EQ(m[3].x, 13.0f);
    EXPECT_FLOAT_EQ(m[3].y, 14.0f);
    EXPECT_FLOAT_EQ(m[3].z, 15.0f);
    EXPECT_FLOAT_EQ(m[3].w, 16.0f);
}

// mat4のカラムベクターコンストラクタのテスト
TEST(Mat4Test, ColumnConstructor)
{
    vec4 c0(1.0f, 2.0f, 3.0f, 4.0f);
    vec4 c1(5.0f, 6.0f, 7.0f, 8.0f);
    vec4 c2(9.0f, 10.0f, 11.0f, 12.0f);
    vec4 c3(13.0f, 14.0f, 15.0f, 16.0f);
    
    mat4 m(c0, c1, c2, c3);
    
    EXPECT_FLOAT_EQ(m[0].x, 1.0f);
    EXPECT_FLOAT_EQ(m[0].y, 2.0f);
    EXPECT_FLOAT_EQ(m[0].z, 3.0f);
    EXPECT_FLOAT_EQ(m[0].w, 4.0f);
    
    EXPECT_FLOAT_EQ(m[1].x, 5.0f);
    EXPECT_FLOAT_EQ(m[1].y, 6.0f);
    EXPECT_FLOAT_EQ(m[1].z, 7.0f);
    EXPECT_FLOAT_EQ(m[1].w, 8.0f);
}

// 配列アクセス演算子のテスト
TEST(Mat4Test, ArrayAccessOperator)
{
    mat4 m(1.0f);
    
    // 読み込みテスト
    EXPECT_FLOAT_EQ(m[0].x, 1.0f);
    EXPECT_FLOAT_EQ(m[1].y, 1.0f);
    
    // 書き込みテスト
    m[0].x = 5.0f;
    m[1].y = 6.0f;
    
    EXPECT_FLOAT_EQ(m[0].x, 5.0f);
    EXPECT_FLOAT_EQ(m[1].y, 6.0f);
}

// 行列加算のテスト
TEST(Mat4Test, MatrixAddition)
{
    mat4 m1(1.0f, 2.0f, 3.0f, 4.0f,
            5.0f, 6.0f, 7.0f, 8.0f,
            9.0f, 10.0f, 11.0f, 12.0f,
            13.0f, 14.0f, 15.0f, 16.0f);
    
    mat4 m2(1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f);
    
    mat4 result = m1 + m2;
    
    EXPECT_FLOAT_EQ(result[0].x, 2.0f);
    EXPECT_FLOAT_EQ(result[0].y, 3.0f);
    EXPECT_FLOAT_EQ(result[1].x, 6.0f);
    EXPECT_FLOAT_EQ(result[3].w, 17.0f);
}

// 行列減算のテスト
TEST(Mat4Test, MatrixSubtraction)
{
    mat4 m1(2.0f, 3.0f, 4.0f, 5.0f,
            6.0f, 7.0f, 8.0f, 9.0f,
            10.0f, 11.0f, 12.0f, 13.0f,
            14.0f, 15.0f, 16.0f, 17.0f);
    
    mat4 m2(1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f);
    
    mat4 result = m1 - m2;
    
    EXPECT_FLOAT_EQ(result[0].x, 1.0f);
    EXPECT_FLOAT_EQ(result[0].y, 2.0f);
    EXPECT_FLOAT_EQ(result[1].x, 5.0f);
    EXPECT_FLOAT_EQ(result[3].w, 16.0f);
}

// 行列とスカラーの乗算のテスト
TEST(Mat4Test, MatrixScalarMultiplication)
{
    mat4 m(1.0f, 2.0f, 3.0f, 4.0f,
           5.0f, 6.0f, 7.0f, 8.0f,
           9.0f, 10.0f, 11.0f, 12.0f,
           13.0f, 14.0f, 15.0f, 16.0f);
    
    mat4 result = m * 2.0f;
    
    EXPECT_FLOAT_EQ(result[0].x, 2.0f);
    EXPECT_FLOAT_EQ(result[0].y, 4.0f);
    EXPECT_FLOAT_EQ(result[1].x, 10.0f);
    EXPECT_FLOAT_EQ(result[3].w, 32.0f);
}

// 行列乗算のテスト（単位行列）
TEST(Mat4Test, MatrixMultiplicationIdentity)
{
    mat4 identity(1.0f);
    mat4 m(1.0f, 2.0f, 3.0f, 4.0f,
           5.0f, 6.0f, 7.0f, 8.0f,
           9.0f, 10.0f, 11.0f, 12.0f,
           13.0f, 14.0f, 15.0f, 16.0f);
    
    mat4 result = identity * m;
    
    // 単位行列との乗算は元の行列と同じであるべき
    EXPECT_FLOAT_EQ(result[0].x, 1.0f);
    EXPECT_FLOAT_EQ(result[0].y, 2.0f);
    EXPECT_FLOAT_EQ(result[1].x, 5.0f);
    EXPECT_FLOAT_EQ(result[3].w, 16.0f);
}

// 行列とベクトルの乗算のテスト
TEST(Mat4Test, MatrixVectorMultiplication)
{
    mat4 identity(1.0f);
    vec4 v(1.0f, 2.0f, 3.0f, 4.0f);
    
    vec4 result = identity * v;
    
    // 単位行列との乗算は元のベクトルと同じであるべき
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
    EXPECT_FLOAT_EQ(result.w, 4.0f);
}

// 等価比較演算子のテスト
TEST(Mat4Test, EqualityOperator)
{
    mat4 m1(1.0f);
    mat4 m2(1.0f);
    mat4 m3(2.0f);
    
    EXPECT_TRUE(m1 == m2);
    EXPECT_FALSE(m1 == m3);
}

// 非等価比較演算子のテスト
TEST(Mat4Test, InequalityOperator)
{
    mat4 m1(1.0f);
    mat4 m2(1.0f);
    mat4 m3(2.0f);
    
    EXPECT_FALSE(m1 != m2);
    EXPECT_TRUE(m1 != m3);
}

// 転置行列のテスト
TEST(Mat4Test, Transpose)
{
    mat4 m(1.0f, 2.0f, 3.0f, 4.0f,
           5.0f, 6.0f, 7.0f, 8.0f,
           9.0f, 10.0f, 11.0f, 12.0f,
           13.0f, 14.0f, 15.0f, 16.0f);
    
    mat4 result = transpose(m);
    
    // 転置後の値をテスト
    EXPECT_FLOAT_EQ(result[0].x, 1.0f);  // m[0][0]
    EXPECT_FLOAT_EQ(result[1].x, 2.0f);  // m[0][1]
    EXPECT_FLOAT_EQ(result[2].x, 3.0f);  // m[0][2]
    EXPECT_FLOAT_EQ(result[3].x, 4.0f);  // m[0][3]
    
    EXPECT_FLOAT_EQ(result[0].y, 5.0f);  // m[1][0]
    EXPECT_FLOAT_EQ(result[1].y, 6.0f);  // m[1][1]
}

// 行列式のテスト（単位行列）
TEST(Mat4Test, DeterminantIdentity)
{
    mat4 identity(1.0f);
    float det = determinant(identity);
    
    EXPECT_FLOAT_EQ(det, 1.0f);
}

// 逆行列のテスト（単位行列）
TEST(Mat4Test, InverseIdentity)
{
    mat4 identity(1.0f);
    mat4 result = inverse(identity);
    
    // 単位行列の逆行列は単位行列であるべき
    EXPECT_FLOAT_EQ(result[0].x, 1.0f);
    EXPECT_FLOAT_EQ(result[1].y, 1.0f);
    EXPECT_FLOAT_EQ(result[2].z, 1.0f);
    EXPECT_FLOAT_EQ(result[3].w, 1.0f);
    
    // 非対角成分は0であるべき
    EXPECT_FLOAT_EQ(result[0].y, 0.0f);
    EXPECT_FLOAT_EQ(result[1].x, 0.0f);
}

// 型エイリアスのテスト
TEST(Mat4Test, TypeAliases)
{
    // mat4はmat<4,4,float>と同じであることを確認
    mat4 m1(1.0f);
    mat<4, 4, float> m2(1.0f);
    
    EXPECT_TRUE(m1 == m2);
}

// doubleの行列のテスト
TEST(Mat4Test, DoublePrecisionMatrix)
{
    dmat4 m(1.0);
    
    EXPECT_DOUBLE_EQ(m[0].x, 1.0);
    EXPECT_DOUBLE_EQ(m[1].y, 1.0);
    EXPECT_DOUBLE_EQ(m[2].z, 1.0);
    EXPECT_DOUBLE_EQ(m[3].w, 1.0);
}