#pragma once
#include "ocf/math/qualifie.h"
#include "ocf/math/vec3.h"

namespace ocf {
namespace math {

template <typename T> struct mat<3, 3, T> {
    typedef vec<3, T> col_type;
    typedef vec<3, T> row_type;
    typedef mat<3, 3, T> type;

private:
    col_type value[3];

public:
    static constexpr length_t length() { return 3; }

    mat() = default;

    // Identity matrix constructor
    explicit mat(T scalar);

    // Component constructor
    mat(T x0, T y0, T z0,
        T x1, T y1, T z1,
        T x2, T y2, T z2);

    // Column constructor
    mat(const col_type& v0, const col_type& v1, const col_type& v2);

    // Array indexer
    col_type& operator[](length_t i);
    const col_type& operator[](length_t i) const;

    // Assignment operators
    template <typename U> mat<3, 3, T>& operator=(const mat<3, 3, U>& m);
    template <typename U> mat<3, 3, T>& operator+=(const mat<3, 3, U>& m);
    template <typename U> mat<3, 3, T>& operator-=(const mat<3, 3, U>& m);
    template <typename U> mat<3, 3, T>& operator*=(const mat<3, 3, U>& m);
    template <typename U> mat<3, 3, T>& operator*=(const U& scalar);
    template <typename U> mat<3, 3, T>& operator/=(const U& scalar);
};

// Arithmetic operators
template <typename T>
mat<3, 3, T> operator+(const mat<3, 3, T>& m1, const mat<3, 3, T>& m2);

template <typename T>
mat<3, 3, T> operator-(const mat<3, 3, T>& m1, const mat<3, 3, T>& m2);

template <typename T>
mat<3, 3, T> operator*(const mat<3, 3, T>& m1, const mat<3, 3, T>& m2);

template <typename T>
vec<3, T> operator*(const mat<3, 3, T>& m, const vec<3, T>& v);

template <typename T>
mat<3, 3, T> operator*(const mat<3, 3, T>& m, const T& scalar);

template <typename T>
mat<3, 3, T> operator*(const T& scalar, const mat<3, 3, T>& m);

template <typename T>
mat<3, 3, T> operator/(const mat<3, 3, T>& m, const T& scalar);

// Boolean operators
template <typename T>
bool operator==(const mat<3, 3, T>& m1, const mat<3, 3, T>& m2);

template <typename T>
bool operator!=(const mat<3, 3, T>& m1, const mat<3, 3, T>& m2);

// Matrix functions
template <typename T>
mat<3, 3, T> transpose(const mat<3, 3, T>& m);

template <typename T>
T determinant(const mat<3, 3, T>& m);

template <typename T>
mat<3, 3, T> inverse(const mat<3, 3, T>& m);

using mat3 = mat<3, 3, float>;
using dmat3 = mat<3, 3, double>;
using imat3 = mat<3, 3, int>;

} // namespace math
} // namespace ocf

#include "ocf/math/mat3.inl"
