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

inline f32 V3SqrLen(const v3 &a)
{
	return V3Dot(a, a);
}

inline f32 V3Lenght(const v3 &a)
{
	return Sqrt(V3SqrLen(a));
}

inline v3 operator+(const v3 &a, const v3 &b)
{
	const v3 result = { a.x + b.x, a.y + b.y, a.z + b.z };
	return result;
}

inline v3 operator*(const v3 &a, f32 b)
{
	const v3 result = { a.x * b, a.y * b, a.z * b };
	return result;
}

inline v3 operator+=(v3 &a, const v3 &b)
{
	a = { a.x + b.x, a.y + b.y, a.z + b.z };
	return a;
}

inline v3 operator*=(v3 &a, f32 b)
{
	a = { a.x * b, a.y * b, a.z * b };
	return a;
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

#endif
