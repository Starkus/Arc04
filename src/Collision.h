#ifndef _COLLISION
#define _COLLISION

#include "Math.h"

struct GJKResult
{
	bool hit;
	v3 points[4];
};

GJKResult GJKTest(const v3 *vA, u32 vACount, const v3 *vB, u32 vBCount);
v3 ComputeDepenetration(GJKResult gjkResult, const v3 *vA, u32 vACount, const v3 *vB, u32 vBCount);

#if EPA_VISUAL_DEBUGGING
struct Vertex;
void GetEPAStepGeometry(int step, Vertex *buffer, u32 *count);
#endif

#endif
