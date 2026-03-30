namespace ocf {
namespace math {

// Constructors

template <typename T>
inline mat<3, 3, T>::mat(T scalar)
{
    this->value[0] = col_type(scalar, T(0), T(0));
    this->value[1] = col_type(T(0), scalar, T(0));
    this->value[2] = col_type(T(0), T(0), scalar);
}

template <typename T>
inline mat<3, 3, T>::mat(T x0, T y0, T z0,
                         T x1, T y1, T z1,
                         T x2, T y2, T z2)
{
    this->value[0] = col_type(x0, y0, z0);
    this->value[1] = col_type(x1, y1, z1);
    this->value[2] = col_type(x2, y2, z2);
}

template <typename T>
inline mat<3, 3, T>::mat(const col_type& v0, const col_type& v1, const col_type& v2)
{
    this->value[0] = v0;
    this->value[1] = v1;
    this->value[2] = v2;
}

// Array indexer

template <typename T>
inline typename mat<3, 3, T>::col_type& mat<3, 3, T>::operator[](length_t i)
{
    return this->value[i];
}

template <typename T>
inline const typename mat<3, 3, T>::col_type& mat<3, 3, T>::operator[](length_t i) const
{
    return this->value[i];
}

// Assignment operators

template <typename T>
template <typename U>
inline mat<3, 3, T>& mat<3, 3, T>::operator=(const mat<3, 3, U>& m)
{
    this->value[0] = m[0];
    this->value[1] = m[1];
    this->value[2] = m[2];
    return *this;
}

template <typename T>
template <typename U>
inline mat<3, 3, T>& mat<3, 3, T>::operator+=(const mat<3, 3, U>& m)
{
    this->value[0] += m[0];
    this->value[1] += m[1];
    this->value[2] += m[2];
    return *this;
}

template <typename T>
template <typename U>
inline mat<3, 3, T>& mat<3, 3, T>::operator-=(const mat<3, 3, U>& m)
{
    this->value[0] -= m[0];
    this->value[1] -= m[1];
    this->value[2] -= m[2];
    return *this;
}

template <typename T>
template <typename U>
inline mat<3, 3, T>& mat<3, 3, T>::operator*=(const mat<3, 3, U>& m)
{
    return (*this = *this * m);
}

template <typename T>
template <typename U>
inline mat<3, 3, T>& mat<3, 3, T>::operator*=(const U& scalar)
{
    this->value[0] *= scalar;
    this->value[1] *= scalar;
    this->value[2] *= scalar;
    return *this;
}

template <typename T>
template <typename U>
inline mat<3, 3, T>& mat<3, 3, T>::operator/=(const U& scalar)
{
    this->value[0] /= scalar;
    this->value[1] /= scalar;
    this->value[2] /= scalar;
    return *this;
}

// Arithmetic operators

template <typename T>
inline mat<3, 3, T> operator+(const mat<3, 3, T>& m1, const mat<3, 3, T>& m2)
{
    return mat<3, 3, T>(
        m1[0] + m2[0],
        m1[1] + m2[1],
        m1[2] + m2[2]);
}

template <typename T>
inline mat<3, 3, T> operator-(const mat<3, 3, T>& m1, const mat<3, 3, T>& m2)
{
    return mat<3, 3, T>(
        m1[0] - m2[0],
        m1[1] - m2[1],
        m1[2] - m2[2]);
}

template <typename T>
inline mat<3, 3, T> operator*(const mat<3, 3, T>& m1, const mat<3, 3, T>& m2)
{
    return mat<3, 3, T>(
        m1[0].x * m2[0].x + m1[1].x * m2[0].y + m1[2].x * m2[0].z,
        m1[0].y * m2[0].x + m1[1].y * m2[0].y + m1[2].y * m2[0].z,
        m1[0].z * m2[0].x + m1[1].z * m2[0].y + m1[2].z * m2[0].z,
        m1[0].x * m2[1].x + m1[1].x * m2[1].y + m1[2].x * m2[1].z,
        m1[0].y * m2[1].x + m1[1].y * m2[1].y + m1[2].y * m2[1].z,
        m1[0].z * m2[1].x + m1[1].z * m2[1].y + m1[2].z * m2[1].z,
        m1[0].x * m2[2].x + m1[1].x * m2[2].y + m1[2].x * m2[2].z,
        m1[0].y * m2[2].x + m1[1].y * m2[2].y + m1[2].y * m2[2].z,
        m1[0].z * m2[2].x + m1[1].z * m2[2].y + m1[2].z * m2[2].z);
}

template <typename T>
inline vec<3, T> operator*(const mat<3, 3, T>& m, const vec<3, T>& v)
{
    return vec<3, T>(
        m[0].x * v.x + m[1].x * v.y + m[2].x * v.z,
        m[0].y * v.x + m[1].y * v.y + m[2].y * v.z,
        m[0].z * v.x + m[1].z * v.y + m[2].z * v.z);
}

template <typename T>
inline mat<3, 3, T> operator*(const mat<3, 3, T>& m, const T& scalar)
{
    return mat<3, 3, T>(
        m[0] * scalar,
        m[1] * scalar,
        m[2] * scalar);
}

template <typename T>
inline mat<3, 3, T> operator*(const T& scalar, const mat<3, 3, T>& m)
{
    return mat<3, 3, T>(
        m[0] * scalar,
        m[1] * scalar,
        m[2] * scalar);
}

template <typename T>
inline mat<3, 3, T> operator/(const mat<3, 3, T>& m, const T& scalar)
{
    return mat<3, 3, T>(
        m[0] / scalar,
        m[1] / scalar,
        m[2] / scalar);
}

// Boolean operators

template <typename T>
inline bool operator==(const mat<3, 3, T>& m1, const mat<3, 3, T>& m2)
{
    return (m1[0] == m2[0]) && (m1[1] == m2[1]) && (m1[2] == m2[2]);
}

template <typename T>
inline bool operator!=(const mat<3, 3, T>& m1, const mat<3, 3, T>& m2)
{
    return !(m1 == m2);
}

// Matrix functions

template <typename T>
inline mat<3, 3, T> transpose(const mat<3, 3, T>& m)
{
    mat<3, 3, T> Result;
    Result[0].x = m[0].x;
    Result[0].y = m[1].x;
    Result[0].z = m[2].x;
    Result[1].x = m[0].y;
    Result[1].y = m[1].y;
    Result[1].z = m[2].y;
    Result[2].x = m[0].z;
    Result[2].y = m[1].z;
    Result[2].z = m[2].z;
    return Result;
}

template <typename T>
inline T determinant(const mat<3, 3, T>& m)
{
    return
        + m[0].x * (m[1].y * m[2].z - m[2].y * m[1].z)
        - m[1].x * (m[0].y * m[2].z - m[2].y * m[0].z)
        + m[2].x * (m[0].y * m[1].z - m[1].y * m[0].z);
}

template <typename T>
inline mat<3, 3, T> inverse(const mat<3, 3, T>& m)
{
    T OneOverDeterminant = static_cast<T>(1) / determinant(m);
    
    mat<3, 3, T> Inverse;
    Inverse[0].x = + (m[1].y * m[2].z - m[2].y * m[1].z) * OneOverDeterminant;
    Inverse[1].x = - (m[1].x * m[2].z - m[2].x * m[1].z) * OneOverDeterminant;
    Inverse[2].x = + (m[1].x * m[2].y - m[2].x * m[1].y) * OneOverDeterminant;
    Inverse[0].y = - (m[0].y * m[2].z - m[2].y * m[0].z) * OneOverDeterminant;
    Inverse[1].y = + (m[0].x * m[2].z - m[2].x * m[0].z) * OneOverDeterminant;
    Inverse[2].y = - (m[0].x * m[2].y - m[2].x * m[0].y) * OneOverDeterminant;
    Inverse[0].z = + (m[0].y * m[1].z - m[1].y * m[0].z) * OneOverDeterminant;
    Inverse[1].z = - (m[0].x * m[1].z - m[1].x * m[0].z) * OneOverDeterminant;
    Inverse[2].z = + (m[0].x * m[1].y - m[1].x * m[0].y) * OneOverDeterminant;
    
    return Inverse;
}

} // namespace math
} // namespace ocf
