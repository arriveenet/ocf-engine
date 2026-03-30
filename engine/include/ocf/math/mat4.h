#pragma once
#include "ocf/math/qualifie.h"
#include "ocf/math/vec4.h"

namespace ocf {
namespace math {

template <typename T> struct mat<4, 4, T> {
    typedef vec<4, T> col_type;
    typedef vec<4, T> row_type;
    typedef mat<4, 4, T> type;

private:
    col_type value[4];

public:
    static constexpr length_t length() { return 4; }

    mat() = default;

    // Identity matrix constructor
    explicit mat(T scalar);

    // Component constructor
    mat(T x0, T y0, T z0, T w0,
        T x1, T y1, T z1, T w1,
        T x2, T y2, T z2, T w2,
        T x3, T y3, T z3, T w3);

    // Column constructor
    mat(const col_type& v0, const col_type& v1, const col_type& v2, const col_type& v3);

    // Array indexer
    col_type& operator[](length_t i);
    const col_type& operator[](length_t i) const;

    // Assignment operators
    template <typename U> mat<4, 4, T>& operator=(const mat<4, 4, U>& m);
    template <typename U> mat<4, 4, T>& operator+=(const mat<4, 4, U>& m);
    template <typename U> mat<4, 4, T>& operator-=(const mat<4, 4, U>& m);
    template <typename U> mat<4, 4, T>& operator*=(const mat<4, 4, U>& m);
    template <typename U> mat<4, 4, T>& operator*=(const U& scalar);
    template <typename U> mat<4, 4, T>& operator/=(const U& scalar);
};

// Arithmetic operators
template <typename T>
mat<4, 4, T> operator+(const mat<4, 4, T>& m1, const mat<4, 4, T>& m2);

template <typename T>
mat<4, 4, T> operator-(const mat<4, 4, T>& m1, const mat<4, 4, T>& m2);

template <typename T>
mat<4, 4, T> operator*(const mat<4, 4, T>& m1, const mat<4, 4, T>& m2);

template <typename T>
vec<4, T> operator*(const mat<4, 4, T>& m, const vec<4, T>& v);

template <typename T>
mat<4, 4, T> operator*(const mat<4, 4, T>& m, const T& scalar);

template <typename T>
mat<4, 4, T> operator*(const T& scalar, const mat<4, 4, T>& m);

template <typename T>
mat<4, 4, T> operator/(const mat<4, 4, T>& m, const T& scalar);

// Boolean operators
template <typename T>
bool operator==(const mat<4, 4, T>& m1, const mat<4, 4, T>& m2);

template <typename T>
bool operator!=(const mat<4, 4, T>& m1, const mat<4, 4, T>& m2);

// Matrix functions
template <typename T>
mat<4, 4, T> transpose(const mat<4, 4, T>& m);

template <typename T>
T determinant(const mat<4, 4, T>& m);

template <typename T>
mat<4, 4, T> inverse(const mat<4, 4, T>& m);

using mat4 = mat<4, 4, float>;
using dmat4 = mat<4, 4, double>;
using imat4 = mat<4, 4, int>;

} // namespace math
} // namespace ocf

#include "ocf/math/mat4.inl"
