namespace ocf {
namespace math {

// Constructors

template <typename T>
inline mat<4, 4, T>::mat(T scalar)
{
    this->value[0] = col_type(scalar, T(0), T(0), T(0));
    this->value[1] = col_type(T(0), scalar, T(0), T(0));
    this->value[2] = col_type(T(0), T(0), scalar, T(0));
    this->value[3] = col_type(T(0), T(0), T(0), scalar);
}

template <typename T>
inline mat<4, 4, T>::mat(T x0, T y0, T z0, T w0,
                         T x1, T y1, T z1, T w1,
                         T x2, T y2, T z2, T w2,
                         T x3, T y3, T z3, T w3)
{
    this->value[0] = col_type(x0, y0, z0, w0);
    this->value[1] = col_type(x1, y1, z1, w1);
    this->value[2] = col_type(x2, y2, z2, w2);
    this->value[3] = col_type(x3, y3, z3, w3);
}

template <typename T>
inline mat<4, 4, T>::mat(const col_type& v0, const col_type& v1, const col_type& v2, const col_type& v3)
{
    this->value[0] = v0;
    this->value[1] = v1;
    this->value[2] = v2;
    this->value[3] = v3;
}

// Array indexer

template <typename T>
inline typename mat<4, 4, T>::col_type& mat<4, 4, T>::operator[](length_t i)
{
    return this->value[i];
}

template <typename T>
inline const typename mat<4, 4, T>::col_type& mat<4, 4, T>::operator[](length_t i) const
{
    return this->value[i];
}

// Assignment operators

template <typename T>
template <typename U>
inline mat<4, 4, T>& mat<4, 4, T>::operator=(const mat<4, 4, U>& m)
{
    this->value[0] = m[0];
    this->value[1] = m[1];
    this->value[2] = m[2];
    this->value[3] = m[3];
    return *this;
}

template <typename T>
template <typename U>
inline mat<4, 4, T>& mat<4, 4, T>::operator+=(const mat<4, 4, U>& m)
{
    this->value[0] += m[0];
    this->value[1] += m[1];
    this->value[2] += m[2];
    this->value[3] += m[3];
    return *this;
}

template <typename T>
template <typename U>
inline mat<4, 4, T>& mat<4, 4, T>::operator-=(const mat<4, 4, U>& m)
{
    this->value[0] -= m[0];
    this->value[1] -= m[1];
    this->value[2] -= m[2];
    this->value[3] -= m[3];
    return *this;
}

template <typename T>
template <typename U>
inline mat<4, 4, T>& mat<4, 4, T>::operator*=(const mat<4, 4, U>& m)
{
    return *this = *this * m;
}

template <typename T>
template <typename U>
inline mat<4, 4, T>& mat<4, 4, T>::operator*=(const U& scalar)
{
    this->value[0] *= scalar;
    this->value[1] *= scalar;
    this->value[2] *= scalar;
    this->value[3] *= scalar;
    return *this;
}

template <typename T>
template <typename U>
inline mat<4, 4, T>& mat<4, 4, T>::operator/=(const U& scalar)
{
    this->value[0] /= scalar;
    this->value[1] /= scalar;
    this->value[2] /= scalar;
    this->value[3] /= scalar;
    return *this;
}

// Arithmetic operators

template <typename T>
inline mat<4, 4, T> operator+(const mat<4, 4, T>& m1, const mat<4, 4, T>& m2)
{
    return mat<4, 4, T>(
        m1[0] + m2[0],
        m1[1] + m2[1],
        m1[2] + m2[2],
        m1[3] + m2[3]);
}

template <typename T>
inline mat<4, 4, T> operator-(const mat<4, 4, T>& m1, const mat<4, 4, T>& m2)
{
    return mat<4, 4, T>(
        m1[0] - m2[0],
        m1[1] - m2[1],
        m1[2] - m2[2],
        m1[3] - m2[3]);
}

template <typename T>
inline mat<4, 4, T> operator*(const mat<4, 4, T>& m1, const mat<4, 4, T>& m2)
{
    typename mat<4, 4, T>::col_type const SrcA0 = m1[0];
    typename mat<4, 4, T>::col_type const SrcA1 = m1[1];
    typename mat<4, 4, T>::col_type const SrcA2 = m1[2];
    typename mat<4, 4, T>::col_type const SrcA3 = m1[3];

    typename mat<4, 4, T>::col_type const SrcB0 = m2[0];
    typename mat<4, 4, T>::col_type const SrcB1 = m2[1];
    typename mat<4, 4, T>::col_type const SrcB2 = m2[2];
    typename mat<4, 4, T>::col_type const SrcB3 = m2[3];

    mat<4, 4, T> Result;
    Result[0] = SrcA0 * SrcB0.x + SrcA1 * SrcB0.y + SrcA2 * SrcB0.z + SrcA3 * SrcB0.w;
    Result[1] = SrcA0 * SrcB1.x + SrcA1 * SrcB1.y + SrcA2 * SrcB1.z + SrcA3 * SrcB1.w;
    Result[2] = SrcA0 * SrcB2.x + SrcA1 * SrcB2.y + SrcA2 * SrcB2.z + SrcA3 * SrcB2.w;
    Result[3] = SrcA0 * SrcB3.x + SrcA1 * SrcB3.y + SrcA2 * SrcB3.z + SrcA3 * SrcB3.w;
    return Result;
}

template <typename T>
inline vec<4, T> operator*(const mat<4, 4, T>& m, const vec<4, T>& v)
{
    return vec<4, T>(
        m[0].x * v.x + m[1].x * v.y + m[2].x * v.z + m[3].x * v.w,
        m[0].y * v.x + m[1].y * v.y + m[2].y * v.z + m[3].y * v.w,
        m[0].z * v.x + m[1].z * v.y + m[2].z * v.z + m[3].z * v.w,
        m[0].w * v.x + m[1].w * v.y + m[2].w * v.z + m[3].w * v.w
    );
}

template <typename T>
inline mat<4, 4, T> operator*(const mat<4, 4, T>& m, const T& scalar)
{
    return mat<4, 4, T>(
        m[0] * scalar,
        m[1] * scalar,
        m[2] * scalar,
        m[3] * scalar);
}

template <typename T>
inline mat<4, 4, T> operator*(const T& scalar, const mat<4, 4, T>& m)
{
    return mat<4, 4, T>(
        scalar * m[0],
        scalar * m[1],
        scalar * m[2],
        scalar * m[3]);
}

template <typename T>
inline mat<4, 4, T> operator/(const mat<4, 4, T>& m, const T& scalar)
{
    return mat<4, 4, T>(
        m[0] / scalar,
        m[1] / scalar,
        m[2] / scalar,
        m[3] / scalar);
}

// Boolean operators

template <typename T>
inline bool operator==(const mat<4, 4, T>& m1, const mat<4, 4, T>& m2)
{
    return (m1[0] == m2[0]) && (m1[1] == m2[1]) && (m1[2] == m2[2]) && (m1[3] == m2[3]);
}

template <typename T>
inline bool operator!=(const mat<4, 4, T>& m1, const mat<4, 4, T>& m2)
{
    return !(m1 == m2);
}

// Matrix functions

template <typename T>
inline mat<4, 4, T> transpose(const mat<4, 4, T>& m)
{
    mat<4, 4, T> Result;
    Result[0].x = m[0].x;
    Result[0].y = m[1].x;
    Result[0].z = m[2].x;
    Result[0].w = m[3].x;

    Result[1].x = m[0].y;
    Result[1].y = m[1].y;
    Result[1].z = m[2].y;
    Result[1].w = m[3].y;

    Result[2].x = m[0].z;
    Result[2].y = m[1].z;
    Result[2].z = m[2].z;
    Result[2].w = m[3].z;

    Result[3].x = m[0].w;
    Result[3].y = m[1].w;
    Result[3].z = m[2].w;
    Result[3].w = m[3].w;

    return Result;
}

template <typename T>
inline T determinant(const mat<4, 4, T>& m)
{
    T SubFactor00 = m[2].z * m[3].w - m[3].z * m[2].w;
    T SubFactor01 = m[2].y * m[3].w - m[3].y * m[2].w;
    T SubFactor02 = m[2].y * m[3].z - m[3].y * m[2].z;
    T SubFactor03 = m[2].x * m[3].w - m[3].x * m[2].w;
    T SubFactor04 = m[2].x * m[3].z - m[3].x * m[2].z;
    T SubFactor05 = m[2].x * m[3].y - m[3].x * m[2].y;

    vec<4, T> DetCof(
        +(m[1].y * SubFactor00 - m[1].z * SubFactor01 + m[1].w * SubFactor02),
        -(m[1].x * SubFactor00 - m[1].z * SubFactor03 + m[1].w * SubFactor04),
        +(m[1].x * SubFactor01 - m[1].y * SubFactor03 + m[1].w * SubFactor05),
        -(m[1].x * SubFactor02 - m[1].y * SubFactor04 + m[1].z * SubFactor05));

    return m[0].x * DetCof.x + m[0].y * DetCof.y + m[0].z * DetCof.z + m[0].w * DetCof.w;
}

template <typename T>
inline mat<4, 4, T> inverse(const mat<4, 4, T>& m)
{
    T Coef00 = m[2].z * m[3].w - m[3].z * m[2].w;
    T Coef02 = m[1].z * m[3].w - m[3].z * m[1].w;
    T Coef03 = m[1].z * m[2].w - m[2].z * m[1].w;

    T Coef04 = m[2].y * m[3].w - m[3].y * m[2].w;
    T Coef06 = m[1].y * m[3].w - m[3].y * m[1].w;
    T Coef07 = m[1].y * m[2].w - m[2].y * m[1].w;

    T Coef08 = m[2].y * m[3].z - m[3].y * m[2].z;
    T Coef10 = m[1].y * m[3].z - m[3].y * m[1].z;
    T Coef11 = m[1].y * m[2].z - m[2].y * m[1].z;

    T Coef12 = m[2].x * m[3].w - m[3].x * m[2].w;
    T Coef14 = m[1].x * m[3].w - m[3].x * m[1].w;
    T Coef15 = m[1].x * m[2].w - m[2].x * m[1].w;

    T Coef16 = m[2].x * m[3].z - m[3].x * m[2].z;
    T Coef18 = m[1].x * m[3].z - m[3].x * m[1].z;
    T Coef19 = m[1].x * m[2].z - m[2].x * m[1].z;

    T Coef20 = m[2].x * m[3].y - m[3].x * m[2].y;
    T Coef22 = m[1].x * m[3].y - m[3].x * m[1].y;
    T Coef23 = m[1].x * m[2].y - m[2].x * m[1].y;

    vec<4, T> Fac0(Coef00, Coef00, Coef02, Coef03);
    vec<4, T> Fac1(Coef04, Coef04, Coef06, Coef07);
    vec<4, T> Fac2(Coef08, Coef08, Coef10, Coef11);
    vec<4, T> Fac3(Coef12, Coef12, Coef14, Coef15);
    vec<4, T> Fac4(Coef16, Coef16, Coef18, Coef19);
    vec<4, T> Fac5(Coef20, Coef20, Coef22, Coef23);

    vec<4, T> Vec0(m[1].x, m[0].x, m[0].x, m[0].x);
    vec<4, T> Vec1(m[1].y, m[0].y, m[0].y, m[0].y);
    vec<4, T> Vec2(m[1].z, m[0].z, m[0].z, m[0].z);
    vec<4, T> Vec3(m[1].w, m[0].w, m[0].w, m[0].w);

    vec<4, T> Inv0(Vec1 * Fac0 - Vec2 * Fac1 + Vec3 * Fac2);
    vec<4, T> Inv1(Vec0 * Fac0 - Vec2 * Fac3 + Vec3 * Fac4);
    vec<4, T> Inv2(Vec0 * Fac1 - Vec1 * Fac3 + Vec3 * Fac5);
    vec<4, T> Inv3(Vec0 * Fac2 - Vec1 * Fac4 + Vec2 * Fac5);

    vec<4, T> SignA(+1, -1, +1, -1);
    vec<4, T> SignB(-1, +1, -1, +1);
    mat<4, 4, T> Inverse(Inv0 * SignA, Inv1 * SignB, Inv2 * SignA, Inv3 * SignB);

    vec<4, T> Row0(Inverse[0].x, Inverse[1].x, Inverse[2].x, Inverse[3].x);

    T Dot1 = dot(m[0], Row0);

    T OneOverDeterminant = static_cast<T>(1) / Dot1;

    return Inverse * OneOverDeterminant;
}

} // namespace math
} // namespace ocf
