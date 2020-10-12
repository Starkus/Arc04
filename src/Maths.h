#ifndef _MATH
#define _MATH

#include "General.h"
#include <math.h> // MSVC math intrinsics

const f32 PI = 3.1415926535897932384626433832795f;
const f32 HALFPI = 1.5707963267948966192313216916398f;
const f32 PI2 = 6.283185307179586476925286766559f;
const f64 PI_64 = 3.1415926535897932384626433832795;
const f64 HALFPI_64 = 1.5707963267948966192313216916398;
const f64 PI2_64 = 6.283185307179586476925286766559;

inline f32 Fmod(f32 n, f32 d)
{
	return fmodf(n, d);
}

inline f32 Sin(f32 theta)
{
	return sinf(theta);
}

inline f32 Cos(f32 theta)
{
	return cosf(theta);
}

inline f32 Tan(f32 theta)
{
	return tanf(theta);
}

inline f32 Atan2(f32 a, f32 b)
{
	return atan2f(a, b);
}

inline f64 Fmod64(f64 n, f64 d)
{
	return fmod(n, d);
}

inline f64 Sin64(f64 theta)
{
	return sin(theta);
}

inline f64 Cos64(f64 theta)
{
	return cos(theta);
}

inline f64 Tan64(f64 theta)
{
	return tan(theta);
}

inline f32 Sqrt(f32 n)
{
	return sqrtf(n);
}

union v2
{
	struct
	{
		f32 x; f32 y;
	};
	f32 v[2];
};

union v3
{
	struct
	{
		f32 x; f32 y; f32 z;
	};
	struct
	{
		f32 r; f32 g; f32 b;
	};
	f32 v[3];
};

union v4
{
	struct
	{
		f32 x; f32 y; f32 z; f32 w;
	};
	struct
	{
		f32 r; f32 g; f32 b; f32 a;
	};
	f32 v[4];
};

union mat4
{
	struct
	{
		f32 m00; f32 m01; f32 m02; f32 m03;
		f32 m10; f32 m11; f32 m12; f32 m13;
		f32 m20; f32 m21; f32 m22; f32 m23;
		f32 m30; f32 m31; f32 m32; f32 m33;
	};
	f32 m[16];
};

const mat4 MAT4_IDENTITY =
{
	1.0f,	0.0f,	0.0f,	0.0f,
	0.0f,	1.0f,	0.0f,	0.0f,
	0.0f,	0.0f,	1.0f,	0.0f,
	0.0f,	0.0f,	0.0f,	1.0f
};

inline mat4 Mat4Translation(v3 translation)
{
	mat4 result =
	{
		1.0f,	0.0f,	0.0f,	0.0f,
		0.0f,	1.0f,	0.0f,	0.0f,
		0.0f,	0.0f,	1.0f,	0.0f,
		translation.x, translation.y, translation.z, 1.0f
	};
	return result;
}

inline f32 V3Dot(const v3 &a, const v3 &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline v3 V3Cross(const v3 &a, const v3 &b)
{
	const v3 result =
	{
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	};
	return result;
}

inline v3 operator+(const v3 &a, const v3 &b)
{
	const v3 result = { a.x + b.x, a.y + b.y, a.z + b.z };
	return result;
}

inline v3 operator-(const v3 &a, const v3 &b)
{
	const v3 result = { a.x - b.x, a.y - b.y, a.z - b.z };
	return result;
}

inline v3 operator-(const v3 &v)
{
	const v3 result = { -v.x, -v.y, -v.z };
	return result;
}

inline v3 operator*(const v3 &a, f32 b)
{
	const v3 result = { a.x * b, a.y * b, a.z * b };
	return result;
}

inline v3 operator/(const v3 &a, f32 b)
{
	const v3 result = { a.x / b, a.y / b, a.z / b };
	return result;
}

inline v3 operator+=(v3 &a, const v3 &b)
{
	a = { a.x + b.x, a.y + b.y, a.z + b.z };
	return a;
}

inline v3 operator-=(v3 &a, const v3 &b)
{
	a = { a.x - b.x, a.y - b.y, a.z - b.z };
	return a;
}

inline v3 operator*=(v3 &a, f32 b)
{
	a = { a.x * b, a.y * b, a.z * b };
	return a;
}

inline v3 operator/=(v3 &a, f32 b)
{
	a = { a.x / b, a.y / b, a.z / b };
	return a;
}

inline bool operator==(const v3 &a, const v3 &b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

inline f32 V3SqrLen(const v3 &a)
{
	return V3Dot(a, a);
}

inline f32 V3Length(const v3 &a)
{
	return Sqrt(V3SqrLen(a));
}

inline v3 V3Normalize(const v3 &a)
{
	v3 result = a / V3Length(a);
	return result;
}

inline f32 Mat4Determinant(const mat4 &m)
{
	f32 result = m.m00 *
		(m.m11 * m.m22 * m.m33 + m.m12 * m.m23 * m.m31 + m.m13 * m.m21 * m.m32
		- m.m13 * m.m22 * m.m31 - m.m12 * m.m21 * m.m33 - m.m11 * m.m23 * m.m32);
	result -= m.m10 *
		(m.m01 * m.m22 * m.m33 + m.m02 * m.m23 * m.m31 + m.m03 * m.m21 * m.m32
		- m.m03 * m.m22 * m.m31 - m.m02 * m.m21 * m.m33 - m.m01 * m.m23 * m.m32);
	result += m.m20 *
		(m.m01 * m.m12 * m.m33 + m.m02 * m.m13 * m.m31 + m.m03 * m.m11 * m.m32
		- m.m03 * m.m12 * m.m31 - m.m02 * m.m11 * m.m33 - m.m01 * m.m13 * m.m32);
	result -= m.m30 *
		(m.m01 * m.m12 * m.m23 + m.m02 * m.m13 * m.m21 + m.m03 * m.m11 * m.m22
		- m.m03 * m.m12 * m.m21 - m.m02 * m.m11 * m.m23 - m.m01 * m.m13 * m.m22);
	return result;
}

inline mat4 Mat4Adjugate(const mat4 &m)
{
	mat4 result;
	result.m00 = m.m11 * m.m22 * m.m33 + m.m12 * m.m23 * m.m31 + m.m13 * m.m21 * m.m32
				- m.m13 * m.m22 * m.m31 - m.m12 * m.m21 * m.m33 - m.m11 * m.m23 * m.m32;
	result.m01 = -m.m01 * m.m12 * m.m33 - m.m02 * m.m23 * m.m31 - m.m03 * m.m21 * m.m32
				+ m.m03 * m.m12 * m.m31 + m.m02 * m.m21 * m.m33 + m.m01 * m.m23 * m.m32;
	result.m02 = m.m01 * m.m12 * m.m33 + m.m02 * m.m13 * m.m31 + m.m03 * m.m11 * m.m32
				- m.m03 * m.m12 * m.m31 - m.m02 * m.m11 * m.m33 - m.m01 * m.m13 * m.m32;
	result.m03 = -m.m01 * m.m12 * m.m23 - m.m02 * m.m13 * m.m21 - m.m03 * m.m11 * m.m22
				+ m.m03 * m.m12 * m.m21 + m.m02 * m.m11 * m.m23 + m.m01 * m.m13 * m.m22;

	result.m10 = -m.m10 * m.m22 * m.m33 - m.m12 * m.m23 * m.m30 - m.m13 * m.m20 * m.m32
				+ m.m13 * m.m22 * m.m30 + m.m12 * m.m20 * m.m33 + m.m10 * m.m23 * m.m32;
	result.m11 = m.m00 * m.m22 * m.m33 + m.m02 * m.m23 * m.m30 + m.m03 * m.m20 * m.m32
				- m.m03 * m.m22 * m.m30 - m.m02 * m.m20 * m.m33 - m.m00 * m.m23 * m.m32;
	result.m12 = -m.m00 * m.m12 * m.m33 - m.m02 * m.m13 * m.m30 - m.m03 * m.m10 * m.m32
				+ m.m03 * m.m12 * m.m30 + m.m02 * m.m10 * m.m33 + m.m00 * m.m13 * m.m32;
	result.m13 = m.m00 * m.m12 * m.m23 + m.m02 * m.m13 * m.m20 + m.m03 * m.m10 * m.m22
				- m.m03 * m.m12 * m.m20 - m.m02 * m.m10 * m.m23 - m.m00 * m.m13 * m.m22;

	result.m20 = m.m10 * m.m21 * m.m33 + m.m11 * m.m23 * m.m30 + m.m13 * m.m20 * m.m31
				- m.m13 * m.m21 * m.m30 - m.m11 * m.m20 * m.m33 - m.m10 * m.m23 * m.m31;
	result.m21 = -m.m00 * m.m21 * m.m33 - m.m01 * m.m23 * m.m30 - m.m03 * m.m20 * m.m31
				+ m.m03 * m.m21 * m.m30 + m.m01 * m.m20 * m.m33 + m.m00 * m.m23 * m.m31;
	result.m22 = m.m00 * m.m11 * m.m33 + m.m01 * m.m13 * m.m30 + m.m03 * m.m10 * m.m31
				- m.m03 * m.m11 * m.m30 - m.m01 * m.m10 * m.m33 - m.m00 * m.m13 * m.m31;
	result.m23 = -m.m00 * m.m11 * m.m23 - m.m01 * m.m13 * m.m20 - m.m03 * m.m10 * m.m21
				+ m.m03 * m.m11 * m.m20 + m.m01 * m.m10 * m.m23 + m.m00 * m.m13 * m.m21;

	result.m30 = -m.m10 * m.m21 * m.m32 - m.m11 * m.m22 * m.m30 - m.m12 * m.m20 * m.m31
				+ m.m12 * m.m21 * m.m30 + m.m11 * m.m20 * m.m32 + m.m10 * m.m22 * m.m31;
	result.m31 = m.m00 * m.m21 * m.m32 + m.m01 * m.m22 * m.m30 + m.m02 * m.m20 * m.m31
				- m.m02 * m.m21 * m.m30 - m.m01 * m.m20 * m.m32 - m.m00 * m.m22 * m.m31;
	result.m32 = -m.m00 * m.m11 * m.m32 - m.m01 * m.m12 * m.m30 - m.m02 * m.m10 * m.m31
				+ m.m02 * m.m11 * m.m30 + m.m01 * m.m10 * m.m32 + m.m00 * m.m12 * m.m31;
	result.m33 = m.m00 * m.m11 * m.m22 + m.m01 * m.m12 * m.m20 + m.m02 * m.m10 * m.m21
				- m.m02 * m.m11 * m.m20 - m.m01 * m.m10 * m.m22 - m.m00 * m.m12 * m.m21;
	return result;
}

inline mat4 Mat4Transpose(const mat4 &a)
{
	mat4 result;
	result.m00 = a.m00;
	result.m01 = a.m10;
	result.m02 = a.m20;
	result.m03 = a.m30;

	result.m10 = a.m01;
	result.m11 = a.m11;
	result.m12 = a.m21;
	result.m13 = a.m31;

	result.m20 = a.m02;
	result.m21 = a.m12;
	result.m22 = a.m22;
	result.m23 = a.m32;

	result.m30 = a.m03;
	result.m31 = a.m13;
	result.m32 = a.m23;
	result.m33 = a.m33;
	return result;
}

inline mat4 Mat4Multiply(const mat4 &a, const mat4 &b)
{
	mat4 result;
	result.m00 = a.m00 * b.m00 + a.m01 * b.m10 + a.m02 * b.m20 + a.m03 * b.m30;
	result.m01 = a.m00 * b.m01 + a.m01 * b.m11 + a.m02 * b.m21 + a.m03 * b.m31;
	result.m02 = a.m00 * b.m02 + a.m01 * b.m12 + a.m02 * b.m22 + a.m03 * b.m32;
	result.m03 = a.m00 * b.m03 + a.m01 * b.m13 + a.m02 * b.m23 + a.m03 * b.m33;

	result.m10 = a.m10 * b.m00 + a.m11 * b.m10 + a.m12 * b.m20 + a.m13 * b.m30;
	result.m11 = a.m10 * b.m01 + a.m11 * b.m11 + a.m12 * b.m21 + a.m13 * b.m31;
	result.m12 = a.m10 * b.m02 + a.m11 * b.m12 + a.m12 * b.m22 + a.m13 * b.m32;
	result.m13 = a.m10 * b.m03 + a.m11 * b.m13 + a.m12 * b.m23 + a.m13 * b.m33;

	result.m20 = a.m20 * b.m00 + a.m21 * b.m10 + a.m22 * b.m20 + a.m23 * b.m30;
	result.m21 = a.m20 * b.m01 + a.m21 * b.m11 + a.m22 * b.m21 + a.m23 * b.m31;
	result.m22 = a.m20 * b.m02 + a.m21 * b.m12 + a.m22 * b.m22 + a.m23 * b.m32;
	result.m23 = a.m20 * b.m03 + a.m21 * b.m13 + a.m22 * b.m23 + a.m23 * b.m33;

	result.m30 = a.m30 * b.m00 + a.m31 * b.m10 + a.m32 * b.m20 + a.m33 * b.m30;
	result.m31 = a.m30 * b.m01 + a.m31 * b.m11 + a.m32 * b.m21 + a.m33 * b.m31;
	result.m32 = a.m30 * b.m02 + a.m31 * b.m12 + a.m32 * b.m22 + a.m33 * b.m32;
	result.m33 = a.m30 * b.m03 + a.m31 * b.m13 + a.m32 * b.m23 + a.m33 * b.m33;
	return result;
}

inline v4 Mat4TransformV4(const mat4 &m, const v4 &v)
{
	v4 result;
	result.x = v.x * m.m00 + v.y * m.m10 + v.z * m.m20 + v.w * m.m30;
	result.y = v.x * m.m01 + v.y * m.m11 + v.z * m.m21 + v.w * m.m31;
	result.z = v.x * m.m02 + v.y * m.m12 + v.z * m.m22 + v.w * m.m32;
	result.w = v.x * m.m03 + v.y * m.m13 + v.z * m.m23 + v.w * m.m33;
	return result;
}

#endif