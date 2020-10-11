#ifndef _GEOMETRY
#define _GEOMETRY

#include "General.h"
#include "Maths.h"

struct Vertex
{
	v3 pos;
	v2 uv;
	v3 color;
};

struct SkinnedVertex
{
	v3 pos;
	v2 uv;
	v3 nor;
	u16 indices[4];
	f32 weights[4];
};

#endif
