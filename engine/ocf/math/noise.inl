#include "noise.h"
#include "ocf/math/geometric.h"
#include <cmath>
#include <type_traits>

namespace ocf {
namespace math {
namespace detail {

// Ken Perlin's reference permutation table (https://mrl.cs.nyu.edu/~perlin/noise/)
inline constexpr std::array<uint8_t, 256> kPerlinReferencePermutation = {
    151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225,
    140, 36,  103, 30,  69,  142, 8,   99,  37,  240, 21,  10,  23,  190, 6,   148,
    247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203, 117, 35,  11,  32,
    57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136, 171, 168, 68,  175,
    74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122,
    60,  211, 133, 230, 220, 105, 92,  41,  55,  46,  245, 40,  244, 102, 143, 54,
    65,  25,  63,  161, 1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169,
    200, 196, 135, 130, 116, 188, 159, 86,  164, 100, 109, 198, 173, 186, 3,   64,
    52,  217, 226, 250, 124, 123, 5,   202, 38,  147, 118, 126, 255, 82,  85,  212,
    207, 206, 59,  227, 47,  16,  58,  17,  182, 189, 28,  42,  223, 183, 170, 213,
    119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,
    129, 22,  39,  253, 19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104,
    218, 246, 97,  228, 251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162, 241,
    81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157,
    184, 84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,
    222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156, 180,
};

inline constexpr std::array<uint8_t, 512> makeDoubledPermutation(const std::array<uint8_t, 256>& p)
{
    std::array<uint8_t, 512> result{};
    for (int i = 0; i < 512; ++i) {
        result[i] = p[i & 255];
    }
    return result;
}

inline constexpr std::array<uint8_t, 512> kPerlinReferencePermutation512 =
    makeDoubledPermutation(kPerlinReferencePermutation);

// Gradient sets
inline constexpr int8_t kPerlinGrad2[8][2] = {
    {1, 1}, {-1, 1}, {1, -1}, {-1, -1}, {1, 0}, {-1, 0}, {0, 1}, {0, -1},
};

// Ken Perlin's 12 cube edge vectors, padded to 16 for a cheap "& 15" lookup
inline constexpr int8_t kPerlinGrad3[16][3] = {
    {1, 1, 0},  {-1, 1, 0},  {1, -1, 0}, {-1, -1, 0}, {1, 0, 1},  {-1, 0, 1},
    {1, 0, -1}, {-1, 0, -1}, {0, 1, 1},  {0, -1, 1},  {0, 1, -1}, {0, -1, -1},
    {1, 1, 0},  {0, -1, 1},  {-1, 1, 0}, {0, -1, -1},
};

// The 32 edge midpoints of a 4D hypercube
inline constexpr int8_t kPerlinGrad4[32][4] = {
    {0, 1, 1, 1},   {0, 1, 1, -1},   {0, 1, -1, 1},   {0, 1, -1, -1},
    {0, -1, 1, 1},  {0, -1, 1, -1},  {0, -1, -1, 1},  {0, -1, -1, -1},
    {1, 0, 1, 1},   {1, 0, 1, -1},   {1, 0, -1, 1},   {1, 0, -1, -1},
    {-1, 0, 1, 1},  {-1, 0, 1, -1},  {-1, 0, -1, 1},  {-1, 0, -1, -1},
    {1, 1, 0, 1},   {1, 1, 0, -1},   {1, -1, 0, 1},   {1, -1, 0, -1},
    {-1, 1, 0, 1},  {-1, 1, 0, -1},  {-1, -1, 0, 1},  {-1, -1, 0, -1},
    {1, 1, 1, 0},   {1, 1, -1, 0},   {1, -1, 1, 0},   {1, -1, -1, 0},
    {-1, 1, 1, 0},  {-1, 1, -1, 0},  {-1, -1, 1, 0},  {-1, -1, -1, 0},
};

// Scale factors that map the raw noise to about [-1, 1]; the result is clamped afterwards.
// Raw peaks: 1D 4.0 (gradients up to 8), 2D ~1.0, 3D ~1.0, 4D ~1.2.
// 3D is left unscaled so that seed 0 matches Ken Perlin's reference implementation exactly.
template <int N, typename T>
constexpr T perlinScale()
{
    if constexpr (N == 1) {
        return static_cast<T>(0.25);
    }
    else if constexpr (N == 4) {
        return static_cast<T>(0.8);
    }
    else {
        return static_cast<T>(1);
    }
}

template <int N, typename T>
inline T perlinGradDot(int hash, const T (&d)[N])
{
    if constexpr (N == 1) {
        const int h = hash & 15;
        const T g = static_cast<T>(1 + (h & 7));
        return (h & 8) ? -g * d[0] : g * d[0];
    }
    else {
        const int8_t* g = nullptr;
        if constexpr (N == 2) {
            g = kPerlinGrad2[hash & 7];
        }
        else if constexpr (N == 3) {
            g = kPerlinGrad3[hash & 15];
        }
        else {
            static_assert(N == 4, "Perlin noise supports 1 to 4 dimensions");
            g = kPerlinGrad4[hash & 31];
        }
        T result = static_cast<T>(0);
        for (int i = 0; i < N; ++i) {
            result += static_cast<T>(g[i]) * d[i];
        }
        return result;
    }
}

template <typename T>
inline constexpr T perlinFade(T t)
{
    // 6t^5 - 15t^4 + 10t^3
    return t * t * t * (t * (t * static_cast<T>(6) - static_cast<T>(15)) + static_cast<T>(10));
}

// Wraps a lattice coordinate by rep (when rep > 0), then into the 0-255 table range.
inline int perlinWrapLattice(int i, int rep)
{
    if (rep > 0) {
        i %= rep;
        if (i < 0) {
            i += rep;
        }
    }
    return i & 255;
}

template <typename T>
inline int perlinRepeat(T rep)
{
    const int r = static_cast<int>(std::lround(rep));
    return r < 1 ? 1 : r;
}

// N-dimensional Perlin noise. rep[i] == 0 means no repetition on that axis.
template <int N, typename T>
inline T perlinImpl(const uint8_t* perm, const T (&p)[N], const int (&rep)[N])
{
    static_assert(std::is_floating_point_v<T>, "Perlin noise requires a floating point type");

    int lattice[2][N];
    T frac[N];
    T fade[N];
    for (int i = 0; i < N; ++i) {
        const T fl = std::floor(p[i]);
        const int ip = static_cast<int>(fl);
        frac[i] = p[i] - fl;
        fade[i] = perlinFade(frac[i]);
        lattice[0][i] = perlinWrapLattice(ip, rep[i]);
        lattice[1][i] = perlinWrapLattice(ip + 1, rep[i]);
    }

    // Gradient contribution of every corner. Bit i of the corner index selects the axis i side.
    constexpr int kCorners = 1 << N;
    T values[kCorners];
    for (int c = 0; c < kCorners; ++c) {
        int hash = 0;
        T d[N];
        for (int i = 0; i < N; ++i) {
            const int side = (c >> i) & 1;
            hash = perm[hash + lattice[side][i]];
            d[i] = frac[i] - static_cast<T>(side);
        }
        values[c] = perlinGradDot<N>(hash, d);
    }

    // Interpolate along axis 0 first, then axis 1, ... halving the corner count each time.
    for (int i = 0, count = kCorners; i < N; ++i) {
        count >>= 1;
        for (int k = 0; k < count; ++k) {
            values[k] = lerp(values[2 * k], values[2 * k + 1], fade[i]);
        }
    }

    return clamp(values[0] * perlinScale<N, T>(), static_cast<T>(-1), static_cast<T>(1));
}

template <typename T>
inline T perlin1(const uint8_t* perm, T p, int rep)
{
    const T pa[1] = {p};
    const int ra[1] = {rep};
    return perlinImpl<1>(perm, pa, ra);
}

template <typename T>
inline T perlin2(const uint8_t* perm, const vec<2, T>& p, int rx, int ry)
{
    const T pa[2] = {p.x, p.y};
    const int ra[2] = {rx, ry};
    return perlinImpl<2>(perm, pa, ra);
}

template <typename T>
inline T perlin3(const uint8_t* perm, const vec<3, T>& p, int rx, int ry, int rz)
{
    const T pa[3] = {p.x, p.y, p.z};
    const int ra[3] = {rx, ry, rz};
    return perlinImpl<3>(perm, pa, ra);
}

template <typename T>
inline T perlin4(const uint8_t* perm, const vec<4, T>& p, int rx, int ry, int rz, int rw)
{
    const T pa[4] = {p.x, p.y, p.z, p.w};
    const int ra[4] = {rx, ry, rz, rw};
    return perlinImpl<4>(perm, pa, ra);
}

// splitmix32: small deterministic PRNG, so a seed gives the same table on every platform
// (std::shuffle is implementation-defined).
inline uint32_t perlinSplitMix32(uint32_t& state)
{
    uint32_t z = (state += 0x9E3779B9u);
    z = (z ^ (z >> 16)) * 0x85EBCA6Bu;
    z = (z ^ (z >> 13)) * 0xC2B2AE35u;
    return z ^ (z >> 16);
}

} // namespace detail

// ---- Free functions --------------------------------------------------------------------------

template <typename T>
inline T perlin(T p)
{
    return detail::perlin1(detail::kPerlinReferencePermutation512.data(), p, 0);
}

template <typename T>
inline T perlin(const vec<2, T>& p)
{
    return detail::perlin2(detail::kPerlinReferencePermutation512.data(), p, 0, 0);
}

template <typename T>
inline T perlin(const vec<3, T>& p)
{
    return detail::perlin3(detail::kPerlinReferencePermutation512.data(), p, 0, 0, 0);
}

template <typename T>
inline T perlin(const vec<4, T>& p)
{
    return detail::perlin4(detail::kPerlinReferencePermutation512.data(), p, 0, 0, 0, 0);
}

template <typename T>
inline T perlin(T p, T rep)
{
    return detail::perlin1(detail::kPerlinReferencePermutation512.data(), p,
                           detail::perlinRepeat(rep));
}

template <typename T>
inline T perlin(const vec<2, T>& p, const vec<2, T>& rep)
{
    return detail::perlin2(detail::kPerlinReferencePermutation512.data(), p,
                           detail::perlinRepeat(rep.x), detail::perlinRepeat(rep.y));
}

template <typename T>
inline T perlin(const vec<3, T>& p, const vec<3, T>& rep)
{
    return detail::perlin3(detail::kPerlinReferencePermutation512.data(), p,
                           detail::perlinRepeat(rep.x), detail::perlinRepeat(rep.y),
                           detail::perlinRepeat(rep.z));
}

template <typename T>
inline T perlin(const vec<4, T>& p, const vec<4, T>& rep)
{
    return detail::perlin4(detail::kPerlinReferencePermutation512.data(), p,
                           detail::perlinRepeat(rep.x), detail::perlinRepeat(rep.y),
                           detail::perlinRepeat(rep.z), detail::perlinRepeat(rep.w));
}

// ---- PerlinNoise -----------------------------------------------------------------------------

inline PerlinNoise::PerlinNoise()
    : m_perm(detail::kPerlinReferencePermutation512)
    , m_seed(0)
{
}

inline PerlinNoise::PerlinNoise(uint32_t seed)
{
    reseed(seed);
}

inline void PerlinNoise::reseed(uint32_t seed)
{
    m_seed = seed;
    if (seed == 0) {
        m_perm = detail::kPerlinReferencePermutation512;
        return;
    }

    std::array<uint8_t, 256> p{};
    for (int i = 0; i < 256; ++i) {
        p[i] = static_cast<uint8_t>(i);
    }

    // Fisher-Yates shuffle with an unbiased-enough multiply-shift range reduction
    uint32_t state = seed;
    for (uint32_t i = 255; i > 0; --i) {
        const uint64_t r = detail::perlinSplitMix32(state);
        const uint32_t j = static_cast<uint32_t>((r * (i + 1)) >> 32);
        const uint8_t tmp = p[i];
        p[i] = p[j];
        p[j] = tmp;
    }

    m_perm = detail::makeDoubledPermutation(p);
}

template <typename T>
inline T PerlinNoise::noise(T p) const
{
    return detail::perlin1(m_perm.data(), p, 0);
}

template <typename T>
inline T PerlinNoise::noise(const vec<2, T>& p) const
{
    return detail::perlin2(m_perm.data(), p, 0, 0);
}

template <typename T>
inline T PerlinNoise::noise(const vec<3, T>& p) const
{
    return detail::perlin3(m_perm.data(), p, 0, 0, 0);
}

template <typename T>
inline T PerlinNoise::noise(const vec<4, T>& p) const
{
    return detail::perlin4(m_perm.data(), p, 0, 0, 0, 0);
}

template <typename T>
inline T PerlinNoise::noise(T p, T rep) const
{
    return detail::perlin1(m_perm.data(), p, detail::perlinRepeat(rep));
}

template <typename T>
inline T PerlinNoise::noise(const vec<2, T>& p, const vec<2, T>& rep) const
{
    return detail::perlin2(m_perm.data(), p, detail::perlinRepeat(rep.x),
                           detail::perlinRepeat(rep.y));
}

template <typename T>
inline T PerlinNoise::noise(const vec<3, T>& p, const vec<3, T>& rep) const
{
    return detail::perlin3(m_perm.data(), p, detail::perlinRepeat(rep.x),
                           detail::perlinRepeat(rep.y), detail::perlinRepeat(rep.z));
}

template <typename T>
inline T PerlinNoise::noise(const vec<4, T>& p, const vec<4, T>& rep) const
{
    return detail::perlin4(m_perm.data(), p, detail::perlinRepeat(rep.x),
                           detail::perlinRepeat(rep.y), detail::perlinRepeat(rep.z),
                           detail::perlinRepeat(rep.w));
}

} // namespace math
} // namespace ocf
