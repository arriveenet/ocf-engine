// SPDX-License-Identifier: MIT
#pragma once
#include "ocf/math/vec2.h"
#include "ocf/math/vec3.h"
#include "ocf/math/vec4.h"
#include <array>
#include <cstdint>

namespace ocf {
namespace math {

// Perlin noise (Ken Perlin's "Improved Noise", 2002).
//
// The signatures follow glm/gtc/noise.hpp (perlin(p) and perlin(p, rep)), but the values are
// NOT bit-identical to glm: glm uses a GLSL-style mod289 permutation polynomial, whereas this
// implementation uses a permutation table so that it can be seeded (see PerlinNoise).
//
// - The result is in [-1, 1] and is 0 at every integer lattice point.
// - T must be a floating point type.
// - The periodic overloads repeat every rep units on each axis (rep is rounded to the nearest
//   integer and must be >= 1), which is useful for tileable textures.
// - The free functions use Ken Perlin's reference permutation table and give the same result
//   as PerlinNoise with seed 0.

// Classic Perlin noise
template <typename T>
T perlin(T p);

template <typename T>
T perlin(const vec<2, T>& p);

template <typename T>
T perlin(const vec<3, T>& p);

template <typename T>
T perlin(const vec<4, T>& p);

// Periodic Perlin noise
template <typename T>
T perlin(T p, T rep);

template <typename T>
T perlin(const vec<2, T>& p, const vec<2, T>& rep);

template <typename T>
T perlin(const vec<3, T>& p, const vec<3, T>& rep);

template <typename T>
T perlin(const vec<4, T>& p, const vec<4, T>& rep);

// Seeded Perlin noise generator. The same seed always gives the same noise on every platform
// and compiler. Seed 0 uses the reference permutation table (same result as math::perlin()).
class PerlinNoise {
public:
    PerlinNoise();
    explicit PerlinNoise(uint32_t seed);

    void reseed(uint32_t seed);
    uint32_t seed() const { return m_seed; }

    template <typename T>
    T noise(T p) const;

    template <typename T>
    T noise(const vec<2, T>& p) const;

    template <typename T>
    T noise(const vec<3, T>& p) const;

    template <typename T>
    T noise(const vec<4, T>& p) const;

    template <typename T>
    T noise(T p, T rep) const;

    template <typename T>
    T noise(const vec<2, T>& p, const vec<2, T>& rep) const;

    template <typename T>
    T noise(const vec<3, T>& p, const vec<3, T>& rep) const;

    template <typename T>
    T noise(const vec<4, T>& p, const vec<4, T>& rep) const;

private:
    // 256 entries stored twice so that perm[perm[x] + y] needs no wrap
    std::array<uint8_t, 512> m_perm;
    uint32_t m_seed = 0;
};

} // namespace math
} // namespace ocf

#include "ocf/math/noise.inl"
