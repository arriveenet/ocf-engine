#include <gtest/gtest.h>

#include <ocf/math/constants.h>
#include <ocf/math/matrix_transform.h>
#include <cmath>
using namespace ocf::math;

// Test translation matrix
TEST(MatrixTransformTest, TranslateIdentity)
{
    mat4 identity(1.0f);
    vec3 translation(1.0f, 2.0f, 3.0f);
    
    mat4 result = translate(identity, translation);
    
    EXPECT_FLOAT_EQ(result[3].x, 1.0f);
    EXPECT_FLOAT_EQ(result[3].y, 2.0f);  
    EXPECT_FLOAT_EQ(result[3].z, 3.0f);
    EXPECT_FLOAT_EQ(result[3].w, 1.0f);
}

// Test translation matrix from vector
TEST(MatrixTransformTest, TranslateFromVector)
{
    vec3 translation(5.0f, -2.0f, 10.0f);
    
    mat4 result = translate(translation);
    
    // Should be identity matrix with translation in the last column
    EXPECT_FLOAT_EQ(result[0].x, 1.0f);
    EXPECT_FLOAT_EQ(result[1].y, 1.0f);
    EXPECT_FLOAT_EQ(result[2].z, 1.0f);
    EXPECT_FLOAT_EQ(result[3].x, 5.0f);
    EXPECT_FLOAT_EQ(result[3].y, -2.0f);
    EXPECT_FLOAT_EQ(result[3].z, 10.0f);
    EXPECT_FLOAT_EQ(result[3].w, 1.0f);
}

// Test scale matrix
TEST(MatrixTransformTest, ScaleIdentity)
{
    mat4 identity(1.0f);
    vec3 scaling(2.0f, 3.0f, 4.0f);
    
    mat4 result = scale(identity, scaling);
    
    EXPECT_FLOAT_EQ(result[0].x, 2.0f);
    EXPECT_FLOAT_EQ(result[1].y, 3.0f);
    EXPECT_FLOAT_EQ(result[2].z, 4.0f);
    EXPECT_FLOAT_EQ(result[3].w, 1.0f);
}

// Test scale matrix from vector
TEST(MatrixTransformTest, ScaleFromVector)
{
    vec3 scaling(0.5f, 2.0f, 1.5f);
    
    mat4 result = scale(scaling);
    
    // Should be identity matrix with scaling on diagonal
    EXPECT_FLOAT_EQ(result[0].x, 0.5f);
    EXPECT_FLOAT_EQ(result[1].y, 2.0f);
    EXPECT_FLOAT_EQ(result[2].z, 1.5f);
    EXPECT_FLOAT_EQ(result[3].w, 1.0f);
    
    // Non-diagonal elements should be zero
    EXPECT_FLOAT_EQ(result[0].y, 0.0f);
    EXPECT_FLOAT_EQ(result[1].x, 0.0f);
    EXPECT_FLOAT_EQ(result[2].x, 0.0f);
}

// Test rotation around X axis
TEST(MatrixTransformTest, RotateX)
{
    float angle = pi<float>() / 2.0f; // 90 degrees
    
    mat4 result = rotateX(angle);
    
    // After 90 degree rotation around X, Y becomes Z and Z becomes -Y
    EXPECT_FLOAT_EQ(result[0].x, 1.0f);
    EXPECT_NEAR(result[1].y, 0.0f, 0.001f);  // cos(90) = 0
    EXPECT_NEAR(result[1].z, 1.0f, 0.001f);  // sin(90) = 1
    EXPECT_NEAR(result[2].y, -1.0f, 0.001f); // -sin(90) = -1
    EXPECT_NEAR(result[2].z, 0.0f, 0.001f);  // cos(90) = 0
}

// Test rotation around Y axis
TEST(MatrixTransformTest, RotateY)
{
    float angle = pi<float>() / 2.0f; // 90 degrees
    
    mat4 result = rotateY(angle);
    
    // After 90 degree rotation around Y, Z becomes X and X becomes -Z
    EXPECT_NEAR(result[0].x, 0.0f, 0.001f);  // cos(90) = 0
    EXPECT_NEAR(result[0].z, -1.0f, 0.001f); // -sin(90) = -1
    EXPECT_FLOAT_EQ(result[1].y, 1.0f);
    EXPECT_NEAR(result[2].x, 1.0f, 0.001f);  // sin(90) = 1
    EXPECT_NEAR(result[2].z, 0.0f, 0.001f);  // cos(90) = 0
}

// Test rotation around Z axis
TEST(MatrixTransformTest, RotateZ)
{
    float angle = pi<float>() / 2.0f; // 90 degrees
    
    mat4 result = rotateZ(angle);
    
    // After 90 degree rotation around Z, X becomes Y and Y becomes -X
    EXPECT_NEAR(result[0].x, 0.0f, 0.001f);  // cos(90) = 0
    EXPECT_NEAR(result[0].y, 1.0f, 0.001f);  // sin(90) = 1
    EXPECT_NEAR(result[1].x, -1.0f, 0.001f); // -sin(90) = -1
    EXPECT_NEAR(result[1].y, 0.0f, 0.001f);  // cos(90) = 0
    EXPECT_FLOAT_EQ(result[2].z, 1.0f);
}

// Test rotation around arbitrary axis
TEST(MatrixTransformTest, RotateArbitraryAxis)
{
    float angle = pi<float>();   // 180 degrees
    vec3 axis(0.0f, 0.0f, 1.0f); // Z axis
    
    mat4 result = rotate(angle, axis);
    
    // 180 degree rotation around Z should flip X and Y
    EXPECT_NEAR(result[0].x, -1.0f, 0.001f);
    EXPECT_NEAR(result[0].y, 0.0f, 0.001f);
    EXPECT_NEAR(result[1].x, 0.0f, 0.001f);
    EXPECT_NEAR(result[1].y, -1.0f, 0.001f);
    EXPECT_NEAR(result[2].z, 1.0f, 0.001f);
}

// Test perspective projection
TEST(MatrixTransformTest, Perspective)
{
    float fovy = pi<float>() / 4.0f; // 45 degrees
    float aspect = 16.0f / 9.0f;
    float zNear = 0.1f;
    float zFar = 100.0f;
    
    mat4 result = perspective(fovy, aspect, zNear, zFar);
    
    // Check that it's a perspective projection matrix
    EXPECT_GT(result[0].x, 0.0f); // X scaling should be positive
    EXPECT_GT(result[1].y, 0.0f); // Y scaling should be positive
    EXPECT_LT(result[2].z, 0.0f); // Z transformation should be negative
    EXPECT_FLOAT_EQ(result[2].w, -1.0f); // Perspective divide
}

// Test orthographic projection
TEST(MatrixTransformTest, Orthographic)
{
    float left = -10.0f;
    float right = 10.0f;
    float bottom = -5.0f;
    float top = 5.0f;
    float zNear = 0.1f;
    float zFar = 100.0f;
    
    mat4 result = ortho(left, right, bottom, top, zNear, zFar);
    
    // Check orthographic projection properties
    EXPECT_FLOAT_EQ(result[0].x, 2.0f / (right - left));  // X scaling
    EXPECT_FLOAT_EQ(result[1].y, 2.0f / (top - bottom));  // Y scaling
    EXPECT_FLOAT_EQ(result[2].z, -2.0f / (zFar - zNear)); // Z scaling
    EXPECT_FLOAT_EQ(result[3].w, 1.0f); // No perspective divide
}

// Test lookAt matrix
TEST(MatrixTransformTest, LookAt)
{
    vec3 eye(0.0f, 0.0f, 5.0f);
    vec3 center(0.0f, 0.0f, 0.0f);
    vec3 up(0.0f, 1.0f, 0.0f);
    
    mat4 result = lookAt(eye, center, up);
    
    // Looking down negative Z axis should result in identity-like rotation
    // The view matrix transforms world coordinates to camera coordinates
    EXPECT_NEAR(result[0].x, 1.0f, 0.001f);  // Right vector X
    EXPECT_NEAR(result[1].y, 1.0f, 0.001f);  // Up vector Y 
    EXPECT_NEAR(result[2].z, 1.0f, 0.001f);  // Forward in camera space is +Z
    EXPECT_NEAR(result[3].z, -5.0f, 0.001f); // Eye Z position negated
}

// Test that transformation matrices can be combined
TEST(MatrixTransformTest, CombinedTransformations)
{
    vec3 translation(1.0f, 2.0f, 3.0f);
    vec3 scaling(2.0f, 2.0f, 2.0f);
    
    mat4 T = translate(translation);
    mat4 S = scale(scaling);
    mat4 combined = T * S;
    
    // Apply combined transformation to a point
    vec4 point(1.0f, 1.0f, 1.0f, 1.0f);
    vec4 result = combined * point;
    
    // Should first scale (2,2,2) then translate (1,2,3)
    EXPECT_FLOAT_EQ(result.x, 3.0f); // 1*2 + 1 = 3
    EXPECT_FLOAT_EQ(result.y, 4.0f); // 1*2 + 2 = 4  
    EXPECT_FLOAT_EQ(result.z, 5.0f); // 1*2 + 3 = 5
    EXPECT_FLOAT_EQ(result.w, 1.0f);
}