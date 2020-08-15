#ifndef _MATH
#define _MATH

#include "General.h"
#include <math.h> // MSVC math intrinsics

const f32 PI = 3.1415926535897932384626433832795f;
const f32 HALFPI = 1.5707963267948966192313216916398f;
const f64 PI_D = 3.1415926535897932384626433832795;
const f64 HALFPI_D = 1.5707963267948966192313216916398;

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

#endif
