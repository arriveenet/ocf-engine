// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include <ocf/math/noise.h>

#include <cmath>

using namespace ocf::math;

namespace {

// Deterministic sample coordinates in [-64, 64)
float samplePos(int i, int axis)
{
    const uint32_t h = static_cast<uint32_t>(i) * 2654435761u + static_cast<uint32_t>(axis) * 40503u;
    return static_cast<float>(h % 128000u) / 1000.0f - 64.0f;
}

} // namespace

TEST(NoiseTest, MatchesKenPerlinReference3D)
{
    // Value of Ken Perlin's reference ImprovedNoise.noise(3.14, 42, 7)
    EXPECT_NEAR(perlin(vec<3, double>(3.14, 42.0, 7.0)), 0.13691995878400012, 1e-12);
}

TEST(NoiseTest, ZeroAtLatticePoints)
{
    for (int i = -5; i <= 5; ++i) {
        const float f = static_cast<float>(i);
        EXPECT_FLOAT_EQ(perlin(f), 0.0f);
        EXPECT_FLOAT_EQ(perlin(vec2(f, f * 3.0f)), 0.0f);
        EXPECT_FLOAT_EQ(perlin(vec3(f, -f, f * 2.0f)), 0.0f);
        EXPECT_FLOAT_EQ(perlin(vec4(f, 1.0f, -f, 7.0f)), 0.0f);
    }
}

TEST(NoiseTest, RangeAndVariation)
{
    float min1 = 0, max1 = 0, min2 = 0, max2 = 0, min3 = 0, max3 = 0, min4 = 0, max4 = 0;
    for (int i = 0; i < 20000; ++i) {
        const float x = samplePos(i, 0), y = samplePos(i, 1), z = samplePos(i, 2), w = samplePos(i, 3);
        const float n1 = perlin(x);
        const float n2 = perlin(vec2(x, y));
        const float n3 = perlin(vec3(x, y, z));
        const float n4 = perlin(vec4(x, y, z, w));
        min1 = std::min(min1, n1); max1 = std::max(max1, n1);
        min2 = std::min(min2, n2); max2 = std::max(max2, n2);
        min3 = std::min(min3, n3); max3 = std::max(max3, n3);
        min4 = std::min(min4, n4); max4 = std::max(max4, n4);
    }
    for (float v : {min1, min2, min3, min4}) {
        EXPECT_GE(v, -1.0f);
        EXPECT_LT(v, -0.4f);
    }
    for (float v : {max1, max2, max3, max4}) {
        EXPECT_LE(v, 1.0f);
        EXPECT_GT(v, 0.4f);
    }
}

TEST(NoiseTest, Continuity)
{
    const float eps = 1e-3f;
    for (int i = 0; i < 1000; ++i) {
        const vec3 p(samplePos(i, 0), samplePos(i, 1), samplePos(i, 2));
        const vec3 q(p.x + eps, p.y - eps, p.z + eps);
        EXPECT_LT(std::abs(perlin(p) - perlin(q)), 0.05f);

        const float x = samplePos(i, 3);
        EXPECT_LT(std::abs(perlin(x) - perlin(x + eps)), 0.05f);
    }
}

TEST(NoiseTest, Periodic)
{
    const vec4 rep(4.0f, 5.0f, 3.0f, 7.0f);
    for (int i = 0; i < 200; ++i) {
        const float x = samplePos(i, 0), y = samplePos(i, 1), z = samplePos(i, 2), w = samplePos(i, 3);

        EXPECT_NEAR(perlin(x, 6.0f), perlin(x + 6.0f, 6.0f), 1e-4f);
        EXPECT_NEAR(perlin(x, 6.0f), perlin(x - 12.0f, 6.0f), 1e-4f);

        const vec2 r2(rep.x, rep.y);
        EXPECT_NEAR(perlin(vec2(x, y), r2), perlin(vec2(x + rep.x, y), r2), 1e-4f);
        EXPECT_NEAR(perlin(vec2(x, y), r2), perlin(vec2(x, y - rep.y), r2), 1e-4f);

        const vec3 r3(rep.x, rep.y, rep.z);
        EXPECT_NEAR(perlin(vec3(x, y, z), r3), perlin(vec3(x, y, z + rep.z * 2.0f), r3), 1e-4f);

        EXPECT_NEAR(perlin(vec4(x, y, z, w), rep), perlin(vec4(x + rep.x, y, z, w + rep.w), rep),
                    1e-4f);
    }
}

TEST(NoiseTest, PeriodicDiffersFromNonPeriodicOutsideFirstPeriod)
{
    // Without rep, the noise does not repeat every 6 units
    int differences = 0;
    for (int i = 0; i < 100; ++i) {
        const float x = samplePos(i, 0);
        if (std::abs(perlin(x) - perlin(x + 6.0f)) > 1e-3f) {
            ++differences;
        }
    }
    EXPECT_GT(differences, 50);
}

TEST(NoiseTest, DefaultGeneratorMatchesFreeFunctions)
{
    const PerlinNoise noise;
    const PerlinNoise seedZero(0);
    EXPECT_EQ(noise.seed(), 0u);
    for (int i = 0; i < 200; ++i) {
        const float x = samplePos(i, 0), y = samplePos(i, 1), z = samplePos(i, 2), w = samplePos(i, 3);
        EXPECT_FLOAT_EQ(noise.noise(x), perlin(x));
        EXPECT_FLOAT_EQ(noise.noise(vec2(x, y)), perlin(vec2(x, y)));
        EXPECT_FLOAT_EQ(noise.noise(vec3(x, y, z)), perlin(vec3(x, y, z)));
        EXPECT_FLOAT_EQ(noise.noise(vec4(x, y, z, w)), perlin(vec4(x, y, z, w)));
        EXPECT_FLOAT_EQ(seedZero.noise(vec3(x, y, z)), perlin(vec3(x, y, z)));
        EXPECT_FLOAT_EQ(noise.noise(vec2(x, y), vec2(8.0f, 8.0f)),
                        perlin(vec2(x, y), vec2(8.0f, 8.0f)));
    }
}

TEST(NoiseTest, SeedDeterminism)
{
    const PerlinNoise a(12345);
    const PerlinNoise b(12345);
    const PerlinNoise c(54321);
    EXPECT_EQ(a.seed(), 12345u);

    int differences = 0;
    for (int i = 0; i < 200; ++i) {
        const vec3 p(samplePos(i, 0), samplePos(i, 1), samplePos(i, 2));
        EXPECT_FLOAT_EQ(a.noise(p), b.noise(p));
        if (std::abs(a.noise(p) - c.noise(p)) > 1e-3f) {
            ++differences;
        }
    }
    EXPECT_GT(differences, 150);
}

TEST(NoiseTest, Reseed)
{
    PerlinNoise noise(1);
    const PerlinNoise other(2);
    noise.reseed(2);
    EXPECT_EQ(noise.seed(), 2u);
    for (int i = 0; i < 100; ++i) {
        const vec2 p(samplePos(i, 0), samplePos(i, 1));
        EXPECT_FLOAT_EQ(noise.noise(p), other.noise(p));
    }

    noise.reseed(0);
    const vec2 p(1.3f, 2.7f);
    EXPECT_FLOAT_EQ(noise.noise(p), perlin(p));
}

TEST(NoiseTest, SeededGeneratorIsZeroAtLatticePointsAndInRange)
{
    const PerlinNoise noise(777);
    EXPECT_FLOAT_EQ(noise.noise(vec3(2.0f, -3.0f, 5.0f)), 0.0f);
    for (int i = 0; i < 2000; ++i) {
        const float v = noise.noise(vec4(samplePos(i, 0), samplePos(i, 1), samplePos(i, 2),
                                         samplePos(i, 3)));
        EXPECT_GE(v, -1.0f);
        EXPECT_LE(v, 1.0f);
    }
}
