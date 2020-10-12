#include "General.h"
#include "Geometry.h"
#include "Collision.h"

#if EPA_LOGGING
#define EPALOG(...) SDL_Log(__VA_ARGS__)
#else
#define EPALOG(...)
#endif

struct Face
{
	v3 a;
	v3 b;
	v3 c;
};

struct Edge
{
	v3 a;
	v3 b;
};

#if EPA_VISUAL_DEBUGGING
bool g_writePolytopeGeom = true;
Vertex *g_polytopeSteps[16];
int g_polytopeStepCounts[16];

void GenPolytopeMesh(Face *polytopeData, int faceCount, Vertex *buffer)
{
	for (int faceIdx = 0; faceIdx < faceCount; ++faceIdx)
	{
		Face *face = &polytopeData[faceIdx];
		v3 normal = V3Cross(face->c - face->a, face->b - face->a);
		normal = normal * 0.5f + v3{ 0.5f, 0.5f, 0.5f };
		buffer[faceIdx * 3 + 0] = { face->a, {}, normal };
		buffer[faceIdx * 3 + 1] = { face->b, {}, normal };
		buffer[faceIdx * 3 + 2] = { face->c, {}, normal };
	}
}
#endif

bool RayTriangleIntersection(v3 rayOrigin, v3 rayDir, const Triangle &triangle, v3 *hit)
{
	const v3 &a = triangle.a;
	const v3 &b = triangle.b;
	const v3 &c = triangle.c;
	const v3 &nor = triangle.normal;

	const v3 ab = b - a;
	const v3 bc = c - b;
	const v3 ca = a - c;

	f32 rayDistAlongNormal = V3Dot(rayDir, -nor);
	const f32 epsilon = 0.000001f;
	if (rayDistAlongNormal > -epsilon && rayDistAlongNormal < epsilon)
		// Perpendicular
		return false;

	f32 aDistAlongNormal = V3Dot(a - rayOrigin, -nor);
	f32 factor = aDistAlongNormal / rayDistAlongNormal;

	if (factor < 0 || factor > 1)
		return false;

	v3 rayPlaneInt = rayOrigin + rayDir * factor;

	// Barycentric coordinates
	{
		const v3 projABOntoBC = bc * (V3Dot(ab, bc) / V3Dot(bc, bc));
		const v3 v = ab - projABOntoBC;
		if (V3Dot(v, ab) < V3Dot(v, rayPlaneInt - a))
			return false;
	}

	{
		const v3 projBCOntoCA = ca * (V3Dot(bc, ca) / V3Dot(ca, ca));
		const v3 v = bc - projBCOntoCA;
		if (V3Dot(v, bc) < V3Dot(v, rayPlaneInt - b))
			return false;
	}

	{
		const v3 projCAOntoAB = ab * (V3Dot(ca, ab) / V3Dot(ab, ab));
		const v3 v = ca - projCAOntoAB;
		if (V3Dot(v, ca) < V3Dot(v, rayPlaneInt - c))
			return false;
	}

	*hit = rayPlaneInt;
	return true;
}

inline v3 FurthestInDirection(const v3 *points, u32 pointCount, v3 dir)
{
	f32 maxDist = -INFINITY;
	v3 result = {};

	const v3 *scan = points;
	for (u32 i = 0; i < pointCount; ++i)
	{
		v3 p = *scan++;
		f32 dot = V3Dot(p, dir);
		if (dot > maxDist)
		{
			maxDist = dot;
			result = p;
		}
	}

	return result;
}

inline v3 GJKSupport(const v3 *vA, u32 vACount, const v3 *vB, u32 vBCount, v3 dir)
{
	v3 va = FurthestInDirection(vA, vACount, dir);
	v3 vb = FurthestInDirection(vB, vBCount, -dir);
	return va - vb;
}

GJKResult GJKTest(const v3 *vA, u32 vACount, const v3 *vB, u32 vBCount)
{
	GJKResult result;
	result.hit = true;

	int foundPointsCount = 1;
	v3 testDir = { 0, 1, 0 }; // Random initial test direction

	result.points[0] = GJKSupport(vB, vBCount, vA, vACount, testDir);
	testDir = -result.points[0];

	for (int iterations = 0; result.hit && foundPointsCount < 4; ++iterations)
	{
		if (iterations >= 50)
		{
			ASSERT(false);
			break;
		}

		v3 a = GJKSupport(vB, vBCount, vA, vACount, testDir);
		if (V3Dot(testDir, a) < 0)
		{
			result.hit = false;
			break;
		}

		switch (foundPointsCount)
		{
			case 1: // Line
			{
				const v3 b = result.points[0];

				result.points[foundPointsCount++] = a;
				testDir = V3Cross(V3Cross(b - a, -a), b - a);
			} break;
			case 2: // Plane
			{
				const v3 b = result.points[1];
				const v3 c = result.points[0];

				const v3 ab = b - a;
				const v3 ac = c - a;
				const v3 nor = V3Cross(ac, ab);
				v3 abNor = V3Cross(nor, ab);
				v3 acNor = V3Cross(ac, nor);
				if (V3Dot(acNor, -a) > 0)
				{
					result.points[0] = a;
					result.points[1] = c;
					testDir = V3Cross(V3Cross(ac, -a), ac);
				}
				else
				{
					if (V3Dot(abNor, -a) > 0)
					{
						result.points[0] = a;
						result.points[1] = b;
						testDir = V3Cross(V3Cross(ab, -a), ab);
					}
					else
					{
						if (V3Dot(nor, -a) > 0)
						{
							result.points[foundPointsCount++] = a;
							testDir = nor;
						}
						else
						{
							result.points[0] = b;
							result.points[1] = c;
							result.points[2] = a;
							++foundPointsCount;
							testDir = -nor;
						}

						// Assert triangle is wound clockwise
						const v3 A = result.points[2];
						const v3 B = result.points[1];
						const v3 C = result.points[0];
						ASSERT(V3Dot(testDir, V3Cross(C - A, B - A)) >= 0);
					}
				}
			} break;
			case 3: // Tetrahedron
			{
				const v3 b = result.points[2];
				const v3 c = result.points[1];
				const v3 d = result.points[0];

				const v3 ab = b - a;
				const v3 ac = c - a;
				const v3 ad = d - a;

				const v3 abcNor = V3Cross(ac, ab);
				const v3 adbNor = V3Cross(ab, ad);
				const v3 acdNor = V3Cross(ad, ac);

				// Assert normals point outside
				ASSERT(V3Dot(d - a, abcNor) <= 0);
				ASSERT(V3Dot(c - a, adbNor) <= 0);
				ASSERT(V3Dot(b - a, acdNor) <= 0);

				if (V3Dot(abcNor, -a) > 0)
				{
					if (V3Dot(adbNor, -a) > 0)
					{
						result.points[0] = b;
						result.points[1] = a;
						foundPointsCount = 2;
						testDir = V3Cross(V3Cross(ab, -a), ab);
					}
					else
					{
						if (V3Dot(acdNor, -a) > 0)
						{
							result.points[0] = c;
							result.points[1] = a;
							foundPointsCount = 2;
							testDir = V3Cross(V3Cross(ac, -a), ac);
						}
						else
						{
							if (V3Dot(V3Cross(abcNor, ab), -a) > 0)
							{
								result.points[0] = b;
								result.points[1] = a;
								foundPointsCount = 2;
								testDir = V3Cross(V3Cross(ab, -a), ab);
							}
							else
							{
								if (V3Dot(V3Cross(ac, abcNor), -a) > 0)
								{
									result.points[0] = c;
									result.points[1] = a;
									foundPointsCount = 2;
									testDir = V3Cross(V3Cross(ac, -a), ac);
								}
								else
								{
									result.points[0] = c;
									result.points[1] = b;
									result.points[2] = a;
									testDir = abcNor;
								}
							}
						}
					}
				}
				else
				{
					if (V3Dot(acdNor, -a) > 0)
					{
						if (V3Dot(adbNor, -a) > 0)
						{
							result.points[0] = d;
							result.points[1] = a;
							foundPointsCount = 2;
							testDir = V3Cross(V3Cross(ad, -a), ad);
						}
						else
						{
							if (V3Dot(V3Cross(acdNor, ac), -a) > 0)
							{
								result.points[0] = c;
								result.points[1] = a;
								foundPointsCount = 2;
								testDir = V3Cross(V3Cross(ac, -a), ac);
							}
							else
							{
								if (V3Dot(V3Cross(ad, acdNor), -a) > 0)
								{
									result.points[0] = d;
									result.points[1] = a;
									foundPointsCount = 2;
									testDir = V3Cross(V3Cross(ad, -a), ad);
								}
								else
								{
									result.points[0] = d;
									result.points[1] = c;
									result.points[2] = a;
									testDir = acdNor;
								}
							}
						}
					}
					else
					{
						if (V3Dot(adbNor, -a) > 0)
						{
							if (V3Dot(V3Cross(adbNor, ad), -a) > 0)
							{
								result.points[0] = d;
								result.points[1] = a;
								foundPointsCount = 2;
								testDir = V3Cross(V3Cross(ad, -a), ad);
							}
							else
							{
								if (V3Dot(V3Cross(ab, adbNor), -a) > 0)
								{
									result.points[0] = b;
									result.points[1] = a;
									foundPointsCount = 2;
									testDir = V3Cross(V3Cross(ab, -a), ab);
								}
								else
								{
									result.points[0] = b;
									result.points[1] = d;
									result.points[2] = a;
									testDir = adbNor;
								}
							}
						}
						else
						{
							// Done!
							result.points[foundPointsCount++] = a;
						}
					}
				}
			} break;
		}
	}

	return result;
}

v3 ComputeDepenetration(GJKResult gjkResult, const v3 *vA, u32 vACount, const v3 *vB, u32 vBCount)
{
#if EPA_VISUAL_DEBUGGING
	if (g_polytopeSteps[0] == nullptr)
	{
		for (int i = 0; i < ArrayCount(g_polytopeSteps); ++i)
			g_polytopeSteps[i] = (Vertex *)malloc(sizeof(Vertex) * 256);
	}
#endif

	EPALOG("\n\nNew EPA calculation\n");

	// By now we should have the tetrahedron from GJK
	// TODO dynamic array?
	Face polytope[256];
	int polytopeCount = 0;

	// Make all faces from tetrahedron
	{
		const v3 &a = gjkResult.points[3];
		const v3 &b = gjkResult.points[2];
		const v3 &c = gjkResult.points[1];
		const v3 &d = gjkResult.points[0];
		// We took care during GJK to ensure the BCD triangle faces away from the origin
		polytope[0] = { b, d, c };
		// And we know A is on the opposite side of the origin
		polytope[1] = { a, b, c };
		polytope[2] = { a, c, d };
		polytope[3] = { a, d, b };
		polytopeCount = 4;
	}

	Face closestFeature;
	f32 lastLeastDistance = -1.0f;
	const int maxIterations = 64;
	for (int epaStep = 0; epaStep < maxIterations; ++epaStep)
	{
		ASSERT(epaStep < maxIterations - 1);

#if EPA_VISUAL_DEBUGGING
		// Save polytope for debug visualization
		if (g_writePolytopeGeom)
		{
			GenPolytopeMesh(polytope, polytopeCount, g_polytopeSteps[epaStep]);
			g_polytopeStepCounts[epaStep] = polytopeCount;
		}
#endif

		// Find closest feature to origin in polytope
		Face newClosestFeature = {};
		int validFacesFound = 0;
		f32 leastDistance = INFINITY;
		EPALOG("Looking for closest feature\n");
		for (int faceIdx = 0; faceIdx < polytopeCount; ++faceIdx)
		{
			Face *face = &polytope[faceIdx];
			v3 normal = V3Normalize(V3Cross(face->c - face->a, face->b - face->a));
			f32 distToOrigin = V3Dot(normal, face->a);
			if (distToOrigin >= leastDistance)
				continue;

			// Make sure projected origin is within triangle
			const v3 abNor = V3Cross(face->a - face->b, normal);
			const v3 bcNor = V3Cross(face->b - face->c, normal);
			const v3 acNor = V3Cross(face->c - face->a, normal);
			if (V3Dot(abNor, -face->a) > 0 ||
				V3Dot(bcNor, -face->b) > 0 ||
				V3Dot(acNor, -face->a) > 0)
				continue;

			leastDistance = distToOrigin;
			newClosestFeature = *face;
			++validFacesFound;
		}
		if (leastDistance == INFINITY)
		{
			//ASSERT(false);
			EPALOG("ERROR: Couldn't find closest feature!");
			// Collision is probably on the very edge, we don't need depenetration
			break;
		}
		else if (leastDistance <= lastLeastDistance)
		{
			// We tried to expand in this direction but failed! So we are at the edge of
			// the Minkowski difference
			// TODO do we really need two exit conditions? Something must be wrong with
			// the other one!
			EPALOG("Ending because couldn't get any further away from the outer edge");
			break;
		}
		else
		{
			closestFeature = newClosestFeature;
			lastLeastDistance = leastDistance;
			EPALOG("Picked face with distance %.02f out of %d faces\n", leastDistance,
					validFacesFound);
		}

		// Expand polytope!
		v3 testDir = V3Cross(closestFeature.c - closestFeature.a, closestFeature.b - closestFeature.a);
		v3 newPoint = GJKSupport(vB, vBCount, vA, vACount, testDir);
		EPALOG("Found new point { %.02f, %.02f. %.02f } while looking in direction { %.02f, %.02f. %.02f }\n",
				newPoint.x, newPoint.y, newPoint.z, testDir.x, testDir.y, testDir.z);
		if (V3Dot(testDir, newPoint - closestFeature.a) <= 0)
		{
			EPALOG("Done! Couldn't find a closer point\n");
			break;
		}
		Edge holeEdges[256];
		int holeEdgesCount = 0;
		int oldPolytopeCount = polytopeCount;
		for (int faceIdx = 0; faceIdx < polytopeCount; )
		{
			Face *face = &polytope[faceIdx];
			v3 normal = V3Cross(face->c - face->a, face->b - face->a);
			// Ignore if not facing new point
			if (V3Dot(normal, newPoint - face->a) <= 0)
			{
				++faceIdx;
				continue;
			}

			// Add/remove edges to the hole (XOR)
			Edge faceEdges[3] =
			{
				{ face->a, face->b },
				{ face->b, face->c },
				{ face->c, face->a }
			};
			for (int edgeIdx = 0; edgeIdx < 3; ++edgeIdx)
			{
				const Edge &edge = faceEdges[edgeIdx];
				// If it's already on the list, remove it
				bool found = false;
				for (int holeEdgeIdx = 0; holeEdgeIdx < holeEdgesCount; ++holeEdgeIdx)
				{
					const Edge &holeEdge = holeEdges[holeEdgeIdx];
					if ((edge.a == holeEdge.a && edge.b == holeEdge.b) ||
						(edge.a == holeEdge.b && edge.b == holeEdge.a))
					{
						holeEdges[holeEdgeIdx] = holeEdges[--holeEdgesCount];
						found = true;
						break;
					}
				}
				// Otherwise add it
				if (!found)
					holeEdges[holeEdgesCount++] = edge;
			}
			// Remove face from polytope
			polytope[faceIdx] = polytope[--polytopeCount];
		}
		int deletedFaces = oldPolytopeCount - polytopeCount;
		EPALOG("Deleted %d faces which were facing new point\n", deletedFaces);
		EPALOG("Presumably left a hole with %d edges\n", holeEdgesCount);
		if (deletedFaces > 1 && holeEdgesCount >= deletedFaces * 3)
		{
			EPALOG("ERROR! Multiple holes were made on the polytope!\n");
#if EPA_VISUAL_DEBUGGING
			GenPolytopeMesh(polytope, polytopeCount, g_polytopeSteps[epaStep + 1]);
			g_polytopeStepCounts[epaStep + 1] = polytopeCount;
			g_writePolytopeGeom = false;
#endif
		}
		oldPolytopeCount = polytopeCount;
		// Now we should have a hole in the polytope, of which all edges are in holeEdges

		for (int holeEdgeIdx = 0; holeEdgeIdx < holeEdgesCount; ++holeEdgeIdx)
		{
			const Edge &holeEdge = holeEdges[holeEdgeIdx];
			Face newFace = { holeEdge.a, holeEdge.b, newPoint };
			// Assert it's facing the right direction
			// If this assert starts failing we can just rewind the triangles
			const v3 normal = V3Cross(newFace.c - newFace.a, newFace.b - newFace.a);
			ASSERT(V3Dot(normal, newFace.a) >= 0);
			polytope[polytopeCount++] = newFace;
		}
		EPALOG("Added %d faces to fill the hole. Polytope now has %d faces\n",
				polytopeCount - oldPolytopeCount, polytopeCount);
	}

	v3 closestFeatureNor = V3Normalize(V3Cross(closestFeature.c - closestFeature.a,
			closestFeature.b - closestFeature.a));
	return closestFeatureNor * V3Dot(closestFeatureNor, closestFeature.a);
}

#if EPA_VISUAL_DEBUGGING
void GetEPAStepGeometry(int step, Vertex **buffer, u32 *count)
{
	*count = g_polytopeStepCounts[step];
	*buffer = g_polytopeSteps[step];
}
#endif

#undef EPALOG
