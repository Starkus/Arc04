#ifndef _GEOMETRY
#define _GEOMETRY

#include "General.h"
#include "Maths.h"

struct Vertex
{
	v3 pos;
	v2 uv;
	v3 nor;
};

struct SkinnedVertex
{
	v3 pos;
	v2 uv;
	v3 nor;
	u16 indices[4];
	f32 weights[4];
};

struct Triangle
{
	v3 a;
	v3 b;
	v3 c;
	v3 normal;
};

#endif
