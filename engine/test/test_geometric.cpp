// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include <ocf/math/geometric.h>

using namespace ocf::math;

// Test dot product for vec2
TEST(GeometricTest, DotProductVec2)
{
    vec<2, float> v1(3.0f, 4.0f);
    vec<2, float> v2(2.0f, 1.0f);
    
    float result = dot(v1, v2);
    EXPECT_FLOAT_EQ(result, 10.0f); // 3*2 + 4*1 = 10
}

// Test dot product for vec3
TEST(GeometricTest, DotProductVec3)
{
    vec<3, float> v1(1.0f, 2.0f, 3.0f);
    vec<3, float> v2(4.0f, 5.0f, 6.0f);
    
    float result = dot(v1, v2);
    EXPECT_FLOAT_EQ(result, 32.0f); // 1*4 + 2*5 + 3*6 = 32
}

// Test length for vec2
TEST(GeometricTest, LengthVec2)
{
    vec<2, float> v(3.0f, 4.0f);
    
    float result = length(v);
    EXPECT_FLOAT_EQ(result, 5.0f); // sqrt(3*3 + 4*4) = 5
}

// Test length for vec3  
TEST(GeometricTest, LengthVec3)
{
    vec<3, float> v(1.0f, 2.0f, 2.0f);
    
    float result = length(v);
    EXPECT_FLOAT_EQ(result, 3.0f); // sqrt(1*1 + 2*2 + 2*2) = 3
}

// Test length for vec4
TEST(GeometricTest, LengthVec4)
{
    vec<4, float> v(1.0f, 2.0f, 2.0f, 0.0f);
    
    float result = length(v);
    EXPECT_FLOAT_EQ(result, 3.0f); // sqrt(1*1 + 2*2 + 2*2 + 0*0) = 3
}

// Test normalize for vec2
TEST(GeometricTest, NormalizeVec2)
{
    vec<2, float> v(3.0f, 4.0f);
    
    vec<2, float> result = normalize(v);
    EXPECT_FLOAT_EQ(result.x, 0.6f);  // 3/5
    EXPECT_FLOAT_EQ(result.y, 0.8f);  // 4/5
    EXPECT_NEAR(length(result), 1.0f, 0.001f);
}

// Test normalize for vec3
TEST(GeometricTest, NormalizeVec3)
{
    vec<3, float> v(1.0f, 2.0f, 2.0f);
    
    vec<3, float> result = normalize(v);
    EXPECT_FLOAT_EQ(result.x, 1.0f/3.0f);  // 1/3
    EXPECT_FLOAT_EQ(result.y, 2.0f/3.0f);  // 2/3  
    EXPECT_FLOAT_EQ(result.z, 2.0f/3.0f);  // 2/3
    EXPECT_NEAR(length(result), 1.0f, 0.001f);
}

// Test normalize for vec4
TEST(GeometricTest, NormalizeVec4)
{
    vec<4, float> v(1.0f, 2.0f, 2.0f, 0.0f);
    
    vec<4, float> result = normalize(v);
    EXPECT_FLOAT_EQ(result.x, 1.0f/3.0f);  // 1/3
    EXPECT_FLOAT_EQ(result.y, 2.0f/3.0f);  // 2/3
    EXPECT_FLOAT_EQ(result.z, 2.0f/3.0f);  // 2/3
    EXPECT_FLOAT_EQ(result.w, 0.0f);
    EXPECT_NEAR(length(result), 1.0f, 0.001f);
}

// Test normalize zero vector
TEST(GeometricTest, NormalizeZeroVector)
{
    vec<3, float> v(0.0f, 0.0f, 0.0f);
    
    vec<3, float> result = normalize(v);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_FLOAT_EQ(result.z, 0.0f);
}

// Test cross product for vec3
TEST(GeometricTest, CrossProductVec3)
{
    vec<3, float> v1(1.0f, 0.0f, 0.0f);
    vec<3, float> v2(0.0f, 1.0f, 0.0f);
    
    vec<3, float> result = cross(v1, v2);
    EXPECT_FLOAT_EQ(result.x, 0.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
    EXPECT_FLOAT_EQ(result.z, 1.0f);
}

// Test cross product orthogonality
TEST(GeometricTest, CrossProductOrthogonality)
{
    vec<3, float> v1(1.0f, 2.0f, 3.0f);
    vec<3, float> v2(4.0f, 5.0f, 6.0f);
    
    vec<3, float> result = cross(v1, v2);
    
    // Cross product should be orthogonal to both input vectors
    EXPECT_NEAR(dot(result, v1), 0.0f, 0.001f);
    EXPECT_NEAR(dot(result, v2), 0.0f, 0.001f);
}

// Test cross product right-hand rule
TEST(GeometricTest, CrossProductRightHandRule)
{
    vec<3, float> v1(1.0f, 2.0f, 3.0f);
    vec<3, float> v2(2.0f, 3.0f, 4.0f);
    
    vec<3, float> result = cross(v1, v2);
    EXPECT_FLOAT_EQ(result.x, -1.0f);  // 2*4 - 3*3 = -1
    EXPECT_FLOAT_EQ(result.y, 2.0f);   // 3*2 - 1*4 = 2
    EXPECT_FLOAT_EQ(result.z, -1.0f);  // 1*3 - 2*2 = -1
}
TEST(GeometricTest, LerpScalar)
{
    EXPECT_FLOAT_EQ(lerp(2.0f, 6.0f, 0.0f), 2.0f);
    EXPECT_FLOAT_EQ(lerp(2.0f, 6.0f, 0.25f), 3.0f);
    EXPECT_FLOAT_EQ(lerp(2.0f, 6.0f, 1.0f), 6.0f);
    EXPECT_FLOAT_EQ(lerp(2.0f, 6.0f, 1.5f), 8.0f); // Not clamped
}

TEST(GeometricTest, LerpVectors)
{
    const vec2 v2 = lerp(vec2(0.0f, 10.0f), vec2(4.0f, 20.0f), 0.5f);
    EXPECT_FLOAT_EQ(v2.x, 2.0f);
    EXPECT_FLOAT_EQ(v2.y, 15.0f);

    const vec3 v3 = lerp(vec3(1.0f, 2.0f, 3.0f), vec3(3.0f, 2.0f, -1.0f), 0.5f);
    EXPECT_FLOAT_EQ(v3.x, 2.0f);
    EXPECT_FLOAT_EQ(v3.y, 2.0f);
    EXPECT_FLOAT_EQ(v3.z, 1.0f);

    const vec4 v4 = lerp(vec4(0.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f), 0.25f);
    EXPECT_FLOAT_EQ(v4.x, 0.25f);
    EXPECT_FLOAT_EQ(v4.y, 0.5f);
    EXPECT_FLOAT_EQ(v4.z, 0.75f);
    EXPECT_FLOAT_EQ(v4.w, 1.0f);
}
