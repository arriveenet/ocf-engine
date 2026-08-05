// SPDX-License-Identifier: MIT
#include <cmath>
#include <gtest/gtest.h>

#include <ocf/math/constants.inl>
#include <ocf/math/geometric.h>
#include <ocf/math/mat3.h>
#include <ocf/math/mat4.h>
#include <ocf/math/quat.h>
#include <ocf/math/vec3.h>

using namespace ocf::math;

namespace {
constexpr float kEpsilon = 1e-6f;

bool IsSameRotation(const quat& a, const quat& b, float epsilon = 1e-4f)
{
    const quat na = normalize(a);
    const quat nb = normalize(b);
    return std::abs(dot(na, nb)) >= (1.0f - epsilon);
}
}

// quatのデフォルトコンストラクタのテスト
TEST(QuatTest, DefaultConstructor)
{
    quat q;
    // デフォルトコンストラクタで初期化された値をテスト
    EXPECT_FLOAT_EQ(q.x, 0.0f);
    EXPECT_FLOAT_EQ(q.y, 0.0f);
    EXPECT_FLOAT_EQ(q.z, 0.0f);
    EXPECT_FLOAT_EQ(q.w, 1.0f);
}

// quatのスカラーコンストラクタのテスト（単位クォータニオン）
TEST(QuatTest, ScalarConstructorIdentity)
{
    quat q(1.0f);
    
    // スカラー部が1、ベクトル部が0であることを確認
    EXPECT_FLOAT_EQ(q.x, 0.0f);
    EXPECT_FLOAT_EQ(q.y, 0.0f);
    EXPECT_FLOAT_EQ(q.z, 0.0f);
    EXPECT_FLOAT_EQ(q.w, 1.0f);
}

// quatの成分指定コンストラクタのテスト
TEST(QuatTest, ComponentConstructor)
{
    quat q(1.0f, 2.0f, 3.0f, 4.0f);
    
    EXPECT_FLOAT_EQ(q.x, 1.0f);
    EXPECT_FLOAT_EQ(q.y, 2.0f);
    EXPECT_FLOAT_EQ(q.z, 3.0f);
    EXPECT_FLOAT_EQ(q.w, 4.0f);
}

TEST(QuatTest, EulerAnglesZeroCreatesIdentityQuaternion)
{
    quat q(vec3(0.0f, 0.0f, 0.0f));

    EXPECT_NEAR(q.x, 0.0f, kEpsilon);
    EXPECT_NEAR(q.y, 0.0f, kEpsilon);
    EXPECT_NEAR(q.z, 0.0f, kEpsilon);
    EXPECT_NEAR(q.w, 1.0f, kEpsilon);
}

TEST(QuatTest, EulerAnglesZ90CreatesExpectedQuaternion)
{
    quat q(vec3(0.0f, 0.0f, pi<float>() * 0.5f));

    EXPECT_NEAR(q.x, 0.0f, kEpsilon);
    EXPECT_NEAR(q.y, 0.0f, kEpsilon);
    EXPECT_NEAR(q.z, std::sin(pi<float>() * 0.25f), kEpsilon);
    EXPECT_NEAR(q.w, std::cos(pi<float>() * 0.25f), kEpsilon);
}

TEST(QuatTest, Mat3IdentityCreatesIdentityQuaternion)
{
    mat3 m(1.0f);
    quat q(m);

    EXPECT_NEAR(q.x, 0.0f, kEpsilon);
    EXPECT_NEAR(q.y, 0.0f, kEpsilon);
    EXPECT_NEAR(q.z, 0.0f, kEpsilon);
    EXPECT_NEAR(q.w, 1.0f, kEpsilon);
}

TEST(QuatTest, Mat4IdentityCreatesIdentityQuaternion)
{
    mat4 m(1.0f);
    quat q(m);

    EXPECT_NEAR(q.x, 0.0f, kEpsilon);
    EXPECT_NEAR(q.y, 0.0f, kEpsilon);
    EXPECT_NEAR(q.z, 0.0f, kEpsilon);
    EXPECT_NEAR(q.w, 1.0f, kEpsilon);
}

TEST(QuatTest, Mat3CastZ90CreatesExpectedRotationMatrix)
{
    const quat q = angleAxis(pi<float>() * 0.5f, vec3(0.0f, 0.0f, 1.0f));
    const mat3 m = mat3_cast(q);

    EXPECT_NEAR(m[0].x, 0.0f, kEpsilon);
    EXPECT_NEAR(m[0].y, 1.0f, kEpsilon);
    EXPECT_NEAR(m[0].z, 0.0f, kEpsilon);

    EXPECT_NEAR(m[1].x, -1.0f, kEpsilon);
    EXPECT_NEAR(m[1].y, 0.0f, kEpsilon);
    EXPECT_NEAR(m[1].z, 0.0f, kEpsilon);

    EXPECT_NEAR(m[2].x, 0.0f, kEpsilon);
    EXPECT_NEAR(m[2].y, 0.0f, kEpsilon);
    EXPECT_NEAR(m[2].z, 1.0f, kEpsilon);
}

TEST(QuatTest, Mat4CastZ90PreservesRotationAndHomogeneousPart)
{
    const quat q = angleAxis(pi<float>() * 0.5f, vec3(0.0f, 0.0f, 1.0f));
    const mat4 m = mat4_cast(q);

    EXPECT_NEAR(m[0].x, 0.0f, kEpsilon);
    EXPECT_NEAR(m[0].y, 1.0f, kEpsilon);
    EXPECT_NEAR(m[0].z, 0.0f, kEpsilon);
    EXPECT_NEAR(m[0].w, 0.0f, kEpsilon);

    EXPECT_NEAR(m[1].x, -1.0f, kEpsilon);
    EXPECT_NEAR(m[1].y, 0.0f, kEpsilon);
    EXPECT_NEAR(m[1].z, 0.0f, kEpsilon);
    EXPECT_NEAR(m[1].w, 0.0f, kEpsilon);

    EXPECT_NEAR(m[2].x, 0.0f, kEpsilon);
    EXPECT_NEAR(m[2].y, 0.0f, kEpsilon);
    EXPECT_NEAR(m[2].z, 1.0f, kEpsilon);
    EXPECT_NEAR(m[2].w, 0.0f, kEpsilon);

    EXPECT_NEAR(m[3].x, 0.0f, kEpsilon);
    EXPECT_NEAR(m[3].y, 0.0f, kEpsilon);
    EXPECT_NEAR(m[3].z, 0.0f, kEpsilon);
    EXPECT_NEAR(m[3].w, 1.0f, kEpsilon);
}

TEST(QuatTest, QuatCastFromMat3RoundTripPreservesRotation)
{
    const quat q = angleAxis(pi<float>() * 0.5f, vec3(0.0f, 0.0f, 1.0f));
    const quat roundTrip = quat_cast(mat3_cast(q));

    EXPECT_TRUE(IsSameRotation(q, roundTrip));
}

TEST(QuatTest, QuatCastFromMat4RoundTripPreservesRotation)
{
    const quat q = angleAxis(pi<float>() * 0.5f, vec3(0.0f, 0.0f, 1.0f));
    const quat roundTrip = quat_cast(mat4_cast(q));

    EXPECT_TRUE(IsSameRotation(q, roundTrip));
}

// 配列アクセス演算子のテスト
TEST(QuatTest, ArrayAccessOperator)
{
    quat q(1.0f, 2.0f, 3.0f, 4.0f);
    
    EXPECT_FLOAT_EQ(q[0], 1.0f);
    EXPECT_FLOAT_EQ(q[1], 2.0f);
    EXPECT_FLOAT_EQ(q[2], 3.0f);
    EXPECT_FLOAT_EQ(q[3], 4.0f);
}

// クォータニオン加算のテスト
TEST(QuatTest, QuaternionAddition)
{
    quat q1(1.0f, 2.0f, 3.0f, 4.0f);
    quat q2(1.0f, 1.0f, 1.0f, 1.0f);
    
    quat result = q1 + q2;
    
    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 3.0f);
    EXPECT_FLOAT_EQ(result.z, 4.0f);
    EXPECT_FLOAT_EQ(result.w, 5.0f);
}

// クォータニオン減算のテスト
TEST(QuatTest, QuaternionSubtraction)
{
    quat q1(3.0f, 4.0f, 5.0f, 6.0f);
    quat q2(1.0f, 1.0f, 1.0f, 1.0f);
    
    quat result = q1 - q2;
    
    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 3.0f);
    EXPECT_FLOAT_EQ(result.z, 4.0f);
    EXPECT_FLOAT_EQ(result.w, 5.0f);
}

// クォータニオンとスカラーの乗算のテスト
TEST(QuatTest, QuaternionScalarMultiplication)
{
    quat q(1.0f, 2.0f, 3.0f, 4.0f);
    
    quat result = q * 2.0f;
    
    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
    EXPECT_FLOAT_EQ(result.z, 6.0f);
    EXPECT_FLOAT_EQ(result.w, 8.0f);
}

// クォータニオン乗算のテスト（単位クォータニオン）
TEST(QuatTest, QuaternionMultiplicationIdentity)
{
    quat q(1.0f, 2.0f, 3.0f, 4.0f);
    quat identity(1.0f);  // 単位クォータニオン
    
    quat result = q * identity;
    
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
    EXPECT_FLOAT_EQ(result.w, 4.0f);
}

// 等価比較演算子のテスト
TEST(QuatTest, EqualityOperator)
{
    quat q1(1.0f, 2.0f, 3.0f, 4.0f);
    quat q2(1.0f, 2.0f, 3.0f, 4.0f);
    
    EXPECT_TRUE(q1 == q2);
}

// 非等価比較演算子のテスト
TEST(QuatTest, InequalityOperator)
{
    quat q1(1.0f, 2.0f, 3.0f, 4.0f);
    quat q2(1.0f, 2.0f, 3.0f, 5.0f);
    
    EXPECT_TRUE(q1 != q2);
}

// 長さ（ノルム）のテスト
TEST(QuatTest, Length)
{
    quat q(1.0f, 2.0f, 3.0f, 4.0f);
    
    float len = length(q);
    float expected = std::sqrt(1.0f + 4.0f + 9.0f + 16.0f);  // sqrt(30)
    
    EXPECT_NEAR(len, expected, 1e-6f);
}

// 正規化のテスト
TEST(QuatTest, Normalize)
{
    quat q(1.0f, 2.0f, 3.0f, 4.0f);
    
    quat normalized = normalize(q);
    float len = length(normalized);
    
    EXPECT_NEAR(len, 1.0f, 1e-6f);
}

// 内積のテスト
TEST(QuatTest, DotProduct)
{
    quat q1(1.0f, 2.0f, 3.0f, 4.0f);
    quat q2(2.0f, 3.0f, 4.0f, 5.0f);
    
    float dotProduct = dot(q1, q2);
    float expected = 1.0f * 2.0f + 2.0f * 3.0f + 3.0f * 4.0f + 4.0f * 5.0f;  // 40
    
    EXPECT_FLOAT_EQ(dotProduct, expected);
}

// 共役のテスト
TEST(QuatTest, Conjugate)
{
    quat q(1.0f, 2.0f, 3.0f, 4.0f);
    
    quat conj = conjugate(q);
    
    EXPECT_FLOAT_EQ(conj.x, -1.0f);
    EXPECT_FLOAT_EQ(conj.y, -2.0f);
    EXPECT_FLOAT_EQ(conj.z, -3.0f);
    EXPECT_FLOAT_EQ(conj.w, 4.0f);
}

// 逆クォータニオンのテスト
TEST(QuatTest, Inverse)
{
    quat q(0.0f, 0.0f, 0.0f, 1.0f);  // 単位クォータニオン
    
    quat inv = inverse(q);
    quat identity = q * inv;
    
    // 逆クォータニオンとの乗算が単位クォータニオンになることを確認
    EXPECT_NEAR(identity.x, 0.0f, kEpsilon);
    EXPECT_NEAR(identity.y, 0.0f, kEpsilon);
    EXPECT_NEAR(identity.z, 0.0f, kEpsilon);
    EXPECT_NEAR(identity.w, 1.0f, kEpsilon);
}

// ベクトル回転のテスト
TEST(QuatTest, RotateVector)
{
    // Z軸周りの90度回転
    vec3 axis(0.0f, 0.0f, 1.0f);
    quat q = angleAxis(pi<float>() * 0.5f, axis);
    
    vec3 v(1.0f, 0.0f, 0.0f);  // X軸ベクトル
    vec3 rotated = rotate(q, v);
    
    // 90度回転後はY軸方向になるはず
    EXPECT_NEAR(rotated.x, 0.0f, kEpsilon);
    EXPECT_NEAR(rotated.y, 1.0f, kEpsilon);
    EXPECT_NEAR(rotated.z, 0.0f, kEpsilon);
}

// angleAxis関数のテスト
TEST(QuatTest, AngleAxis)
{
    vec3 axis(0.0f, 1.0f, 0.0f);  // Y軸
    constexpr float angle = pi<float>() / 3.0f; // 60度
    
    quat q = angleAxis(angle, axis);
    
    EXPECT_NEAR(q.x, 0.0f, kEpsilon);
    EXPECT_NEAR(q.y, std::sin(pi<float>() / 6.0f), kEpsilon);  // sin(30°)
    EXPECT_NEAR(q.z, 0.0f, kEpsilon);
    EXPECT_NEAR(q.w, std::cos(pi<float>() / 6.0f), kEpsilon);  // cos(30°)
}

// slerp（球面線形補間）のテスト
TEST(QuatTest, Slerp)
{
    quat q1(0.0f, 0.0f, 0.0f, 1.0f);  // 単位クォータニオン
    quat q2(0.0f, 0.0f, 0.707107f, 0.707107f);  // Z軸周り90度回転
    
    quat result = slerp(q1, q2, 0.5f);  // 中間点
    
    // 45度回転になっているはず
    EXPECT_NEAR(result.x, 0.0f, kEpsilon);
    EXPECT_NEAR(result.y, 0.0f, kEpsilon);
    EXPECT_NEAR(result.z, 0.382683f, kEpsilon);  // sin(22.5°)
    EXPECT_NEAR(result.w, 0.92388f, kEpsilon);   // cos(22.5°)
}

// 型エイリアスのテスト
TEST(QuatTest, TypeAliases)
{
    quat q_float(1.0f);
    dquat q_double(1.0);
    
    EXPECT_FLOAT_EQ(q_float.w, 1.0f);
    EXPECT_DOUBLE_EQ(q_double.w, 1.0);
}
