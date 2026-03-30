#pragma once
#include "ocf/math/qualifie.h"
#include "ocf/math/vec2.h"

namespace ocf {
namespace math {

template <typename T> struct mat<2, 2, T> {
    typedef vec<2, T> col_type;
    typedef vec<2, T> row_type;
    typedef mat<2, 2, T> type;

private:
    col_type value[2];

public:
    static constexpr length_t length() { return 2; }

    mat() = default;

    // Identity matrix constructor
    explicit mat(T scalar);

    // Component constructor
    mat(T x0, T y0,
        T x1, T y1);

    // Column constructor
    mat(const col_type& v0, const col_type& v1);

    // Array indexer
    col_type& operator[](length_t i);
    const col_type& operator[](length_t i) const;

    // Assignment operators
    template <typename U> mat<2, 2, T>& operator=(const mat<2, 2, U>& m);
    template <typename U> mat<2, 2, T>& operator+=(const mat<2, 2, U>& m);
    template <typename U> mat<2, 2, T>& operator-=(const mat<2, 2, U>& m);
    template <typename U> mat<2, 2, T>& operator*=(const mat<2, 2, U>& m);
    template <typename U> mat<2, 2, T>& operator*=(const U& scalar);
    template <typename U> mat<2, 2, T>& operator/=(const U& scalar);
};

// Arithmetic operators
template <typename T>
mat<2, 2, T> operator+(const mat<2, 2, T>& m1, const mat<2, 2, T>& m2);

template <typename T>
mat<2, 2, T> operator-(const mat<2, 2, T>& m1, const mat<2, 2, T>& m2);

template <typename T>
mat<2, 2, T> operator*(const mat<2, 2, T>& m1, const mat<2, 2, T>& m2);

template <typename T>
vec<2, T> operator*(const mat<2, 2, T>& m, const vec<2, T>& v);

template <typename T>
mat<2, 2, T> operator*(const mat<2, 2, T>& m, const T& scalar);

template <typename T>
mat<2, 2, T> operator*(const T& scalar, const mat<2, 2, T>& m);

template <typename T>
mat<2, 2, T> operator/(const mat<2, 2, T>& m, const T& scalar);

// Boolean operators
template <typename T>
bool operator==(const mat<2, 2, T>& m1, const mat<2, 2, T>& m2);

template <typename T>
bool operator!=(const mat<2, 2, T>& m1, const mat<2, 2, T>& m2);

// Matrix functions
template <typename T>
mat<2, 2, T> transpose(const mat<2, 2, T>& m);

template <typename T>
T determinant(const mat<2, 2, T>& m);

template <typename T>
mat<2, 2, T> inverse(const mat<2, 2, T>& m);

using mat2 = mat<2, 2, float>;
using dmat2 = mat<2, 2, double>;
using imat2 = mat<2, 2, int>;

} // namespace math
} // namespace ocf

#include "ocf/math/mat2.inl"
