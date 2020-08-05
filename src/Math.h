#ifndef _MATH
#define _MATH

#include "General.h"
#include <math.h> // MSVC math intrinsics

const f32 PI = 3.1415926535897932384626433832795f;
const f32 HALFPI = 1.5707963267948966192313216916398f;
const f64 PI_D = 3.1415926535897932384626433832795;
const f64 HALFPI_D = 1.5707963267948966192313216916398;

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
