namespace ocf {
namespace math {

// Constructors

template <typename T>
inline mat<2, 2, T>::mat(T scalar)
{
    this->value[0] = col_type(scalar, T(0));
    this->value[1] = col_type(T(0), scalar);
}

template <typename T>
inline mat<2, 2, T>::mat(T x0, T y0,
                         T x1, T y1)
{
    this->value[0] = col_type(x0, y0);
    this->value[1] = col_type(x1, y1);
}

template <typename T>
inline mat<2, 2, T>::mat(const col_type& v0, const col_type& v1)
{
    this->value[0] = v0;
    this->value[1] = v1;
}

// Array indexer

template <typename T>
inline typename mat<2, 2, T>::col_type& mat<2, 2, T>::operator[](length_t i)
{
    return this->value[i];
}

template <typename T>
inline const typename mat<2, 2, T>::col_type& mat<2, 2, T>::operator[](length_t i) const
{
    return this->value[i];
}

// Assignment operators

template <typename T>
template <typename U>
inline mat<2, 2, T>& mat<2, 2, T>::operator=(const mat<2, 2, U>& m)
{
    this->value[0] = m[0];
    this->value[1] = m[1];
    return *this;
}

template <typename T>
template <typename U>
inline mat<2, 2, T>& mat<2, 2, T>::operator+=(const mat<2, 2, U>& m)
{
    this->value[0] += m[0];
    this->value[1] += m[1];
    return *this;
}

template <typename T>
template <typename U>
inline mat<2, 2, T>& mat<2, 2, T>::operator-=(const mat<2, 2, U>& m)
{
    this->value[0] -= m[0];
    this->value[1] -= m[1];
    return *this;
}

template <typename T>
template <typename U>
inline mat<2, 2, T>& mat<2, 2, T>::operator*=(const mat<2, 2, U>& m)
{
    return (*this = *this * m);
}

template <typename T>
template <typename U>
inline mat<2, 2, T>& mat<2, 2, T>::operator*=(const U& scalar)
{
    this->value[0] *= scalar;
    this->value[1] *= scalar;
    return *this;
}

template <typename T>
template <typename U>
inline mat<2, 2, T>& mat<2, 2, T>::operator/=(const U& scalar)
{
    this->value[0] /= scalar;
    this->value[1] /= scalar;
    return *this;
}

// Arithmetic operators

template <typename T>
inline mat<2, 2, T> operator+(const mat<2, 2, T>& m1, const mat<2, 2, T>& m2)
{
    return mat<2, 2, T>(
        m1[0] + m2[0],
        m1[1] + m2[1]);
}

template <typename T>
inline mat<2, 2, T> operator-(const mat<2, 2, T>& m1, const mat<2, 2, T>& m2)
{
    return mat<2, 2, T>(
        m1[0] - m2[0],
        m1[1] - m2[1]);
}

template <typename T>
inline mat<2, 2, T> operator*(const mat<2, 2, T>& m1, const mat<2, 2, T>& m2)
{
    return mat<2, 2, T>(
        m1[0].x * m2[0].x + m1[1].x * m2[0].y,
        m1[0].y * m2[0].x + m1[1].y * m2[0].y,
        m1[0].x * m2[1].x + m1[1].x * m2[1].y,
        m1[0].y * m2[1].x + m1[1].y * m2[1].y);
}

template <typename T>
inline vec<2, T> operator*(const mat<2, 2, T>& m, const vec<2, T>& v)
{
    return vec<2, T>(
        m[0].x * v.x + m[1].x * v.y,
        m[0].y * v.x + m[1].y * v.y);
}

template <typename T>
inline mat<2, 2, T> operator*(const mat<2, 2, T>& m, const T& scalar)
{
    return mat<2, 2, T>(
        m[0] * scalar,
        m[1] * scalar);
}

template <typename T>
inline mat<2, 2, T> operator*(const T& scalar, const mat<2, 2, T>& m)
{
    return mat<2, 2, T>(
        m[0] * scalar,
        m[1] * scalar);
}

template <typename T>
inline mat<2, 2, T> operator/(const mat<2, 2, T>& m, const T& scalar)
{
    return mat<2, 2, T>(
        m[0] / scalar,
        m[1] / scalar);
}

// Boolean operators

template <typename T>
inline bool operator==(const mat<2, 2, T>& m1, const mat<2, 2, T>& m2)
{
    return (m1[0] == m2[0]) && (m1[1] == m2[1]);
}

template <typename T>
inline bool operator!=(const mat<2, 2, T>& m1, const mat<2, 2, T>& m2)
{
    return !(m1 == m2);
}

// Matrix functions

template <typename T>
inline mat<2, 2, T> transpose(const mat<2, 2, T>& m)
{
    mat<2, 2, T> Result;
    Result[0].x = m[0].x;
    Result[0].y = m[1].x;
    Result[1].x = m[0].y;
    Result[1].y = m[1].y;
    return Result;
}

template <typename T>
inline T determinant(const mat<2, 2, T>& m)
{
    return m[0].x * m[1].y - m[1].x * m[0].y;
}

template <typename T>
inline mat<2, 2, T> inverse(const mat<2, 2, T>& m)
{
    T OneOverDeterminant = static_cast<T>(1) / determinant(m);
    
    return mat<2, 2, T>(
        + m[1].y * OneOverDeterminant,
        - m[0].y * OneOverDeterminant,
        - m[1].x * OneOverDeterminant,
        + m[0].x * OneOverDeterminant);
}

} // namespace math
} // namespace ocf
