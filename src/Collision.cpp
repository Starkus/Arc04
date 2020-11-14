#if DEBUG_BUILD && defined(USING_IMGUI)
#define VERBOSE_LOG(...) if (g_debugContext->verboseCollisionLogging) Log(__VA_ARGS__)
#else
#define VERBOSE_LOG(...)
#endif

struct GJKResult
{
	bool hit;
	v3 points[4];
};

struct EPAFace
{
	v3 a;
	v3 b;
	v3 c;
};

struct EPAEdge
{
	v3 a;
	v3 b;
};

#if DEBUG_BUILD
void GenPolytopeMesh(EPAFace *polytopeData, int faceCount, DebugVertex *buffer)
{
	for (int faceIdx = 0; faceIdx < faceCount; ++faceIdx)
	{
		EPAFace *face = &polytopeData[faceIdx];
		v3 normal = V3Normalize(V3Cross(face->c - face->a, face->b - face->a));
		normal = normal * 0.5f + v3{ 0.5f, 0.5f, 0.5f };
		buffer[faceIdx * 3 + 0] = { face->a, normal };
		buffer[faceIdx * 3 + 1] = { face->b, normal };
		buffer[faceIdx * 3 + 2] = { face->c, normal };
	}
}
#endif

bool RayTriangleIntersection(v3 rayOrigin, v3 rayDir, const Triangle *triangle, v3 *hit)
{
	const v3 &a = triangle->a;
	const v3 &b = triangle->b;
	const v3 &c = triangle->c;
	const v3 &nor = triangle->normal;

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

bool HitTest_CheckCell(GameState *gameState, int cellX, int cellY, bool swapXY, v3 rayOrigin, v3
		rayDir, v3 *hit, Triangle *triangle)
{
	if (swapXY)
	{
		int tmp = cellX;
		cellX = cellY;
		cellY = tmp;
	}

	const ResourceGeometryGrid *geometryGrid = &gameState->levelGeometry.geometryGrid->geometryGrid;
	v2 cellSize = (geometryGrid->highCorner - geometryGrid->lowCorner) / (f32)(geometryGrid->cellsSide);

	if (cellX < 0 || cellX >= geometryGrid->cellsSide ||
		cellY < 0 || cellY >= geometryGrid->cellsSide)
		return false;

	v3 closestHit = {};
	Triangle *closestTriangle = 0;
	f32 closestSqrLen = INFINITY;

	int offsetIdx = cellX + cellY * geometryGrid->cellsSide;
	u32 triBegin = geometryGrid->offsets[offsetIdx];
	u32 triEnd = geometryGrid->offsets[offsetIdx + 1];
	for (u32 triIdx = triBegin; triIdx < triEnd; ++triIdx)
	{
		Triangle *curTriangle = &geometryGrid->triangles[triIdx];

#if HITTEST_VISUAL_DEBUG
		Vertex tri[] =
		{
			{curTriangle->a, {}, {}},
			{curTriangle->b, {}, {}},
			{curTriangle->c, {}, {}}
		};
		DrawDebugTriangles(gameState, tri, 3);
#endif

		v3 thisHit;
		if (RayTriangleIntersection(rayOrigin, rayDir, curTriangle, &thisHit))
		{
			f32 sqrLen = V3SqrLen(thisHit - rayOrigin);
			if (sqrLen < closestSqrLen)
			{
				closestHit = thisHit;
				closestTriangle = curTriangle;
				closestSqrLen = sqrLen;
			}
		}
	}

#if HITTEST_VISUAL_DEBUG
	f32 cellMinX = geometryGrid->lowCorner.x + (cellX) * cellSize.x;
	f32 cellMaxX = cellMinX + cellSize.x;
	f32 cellMinY = geometryGrid->lowCorner.y + (cellY) * cellSize.y;
	f32 cellMaxY = cellMinY + cellSize.y;
	f32 top = 1;
	Vertex cellDebugTris[] =
	{
		{ { cellMinX, cellMinY, -1 }, {}, {} },
		{ { cellMinX, cellMinY, top }, {}, {} },
		{ { cellMaxX, cellMinY, -1 }, {}, {} },

		{ { cellMaxX, cellMinY, -1 }, {}, {} },
		{ { cellMinX, cellMinY, top }, {}, {} },
		{ { cellMaxX, cellMinY, top }, {}, {} },

		{ { cellMinX, cellMinY, -1 }, {}, {} },
		{ { cellMinX, cellMinY, top }, {}, {} },
		{ { cellMinX, cellMaxY, -1 }, {}, {} },

		{ { cellMinX, cellMaxY, -1 }, {}, {} },
		{ { cellMinX, cellMinY, top }, {}, {} },
		{ { cellMinX, cellMaxY, top }, {}, {} },

		{ { cellMinX, cellMaxY, -1 }, {}, {} },
		{ { cellMinX, cellMaxY, top }, {}, {} },
		{ { cellMaxX, cellMaxY, -1 }, {}, {} },

		{ { cellMaxX, cellMaxY, -1 }, {}, {} },
		{ { cellMinX, cellMaxY, top }, {}, {} },
		{ { cellMaxX, cellMaxY, top }, {}, {} },

		{ { cellMaxX, cellMinY, -1 }, {}, {} },
		{ { cellMaxX, cellMinY, top }, {}, {} },
		{ { cellMaxX, cellMaxY, -1 }, {}, {} },

		{ { cellMaxX, cellMaxY, -1 }, {}, {} },
		{ { cellMaxX, cellMinY, top }, {}, {} },
		{ { cellMaxX, cellMaxY, top }, {}, {} },
	};
	DrawDebugTriangles(gameState, cellDebugTris, 24);
#endif

	if (closestTriangle)
	{
		*hit = closestHit;
		*triangle = *closestTriangle;
		return true;
	}
	return false;
}

bool HitTest(GameState *gameState, v3 rayOrigin, v3 rayDir, v3 *hit, Triangle *triangle)
{
	const ResourceGeometryGrid *geometryGrid = &gameState->levelGeometry.geometryGrid->geometryGrid;
	v2 cellSize = (geometryGrid->highCorner - geometryGrid->lowCorner) / (f32)(geometryGrid->cellsSide);

#if HITTEST_VISUAL_DEBUG
	auto ddraw = [&gameState, &cellSize, &geometryGrid](f32 relX, f32 relY)
	{
		DrawDebugCubeAA(gameState, v3{
				relX * cellSize.x + geometryGrid->lowCorner.x,
				relY * cellSize.y + geometryGrid->lowCorner.y,
				1}, 0.1f);
	};
#else
#define ddraw(...)
#endif

	v2 p1 = { rayOrigin.x, rayOrigin.y };
	v2 p2 = { rayOrigin.x + rayDir.x, rayOrigin.y + rayDir.y };
	p1 = p1 - geometryGrid->lowCorner;
	p2 = p2 - geometryGrid->lowCorner;
	p1.x /= cellSize.x;
	p1.y /= cellSize.y;
	p2.x /= cellSize.x;
	p2.y /= cellSize.y;

	f32 slope = (p2.y - p1.y) / (p2.x - p1.x);
	bool swapXY = slope > 1 || slope < -1;
	if (swapXY)
	{
		f32 tmp = p1.x;
		p1.x = p1.y;
		p1.y = tmp;
		tmp = p2.x;
		p2.x = p2.y;
		p2.y = tmp;
		slope = 1.0f / slope;
	}
	bool flipX = p1.x > p2.x;
	if (flipX)
	{
		slope = -slope;
	}
	f32 xSign = 1.0f - 2.0f * flipX;

	f32 curX = p1.x;
	f32 curY = p1.y;

	if (HitTest_CheckCell(gameState, (int)Floor(curX), (int)Floor(curY), swapXY, rayOrigin, rayDir,
				hit, triangle))
		return true;

	int oldY = (int)Floor(curY);
	for (bool done = false; !done; )
	{
		curX += xSign;
		curY += slope;
		int newY = (int)Floor(curY);

		if ((!flipX && curX > p2.x) || (flipX && curX < p2.x))
		{
			curX = p2.x;
			curY = p2.y;
			done = true;
		}

		if (!swapXY)
			ddraw(curX, curY);
		else
			ddraw(curY, curX);

		if (newY != oldY)
		{
			// When we go to a new row, check whether we come from the side or the top by projecting
			// the current point to the left border of the current cell. If the row of the projected
			// point is different than the current one, then we come from below.
			f32 cellRelX = Fmod(curX, 1.0f) - flipX;
			f32 projY = curY - cellRelX * slope * xSign;

			if (!swapXY)
				ddraw(curX - cellRelX, projY);
			else
				ddraw(projY, curX - cellRelX);

			if ((int)Floor(projY) != newY)
			{
				// Paint cell below
				if (HitTest_CheckCell(gameState, (int)Floor(curX), (int)Floor(projY), swapXY,
							rayOrigin, rayDir, hit, triangle))
					return true;
			}
			else
			{
				// Paint cell behind
				if (HitTest_CheckCell(gameState, (int)Floor(curX - xSign), (int)Floor(curY), swapXY,
							rayOrigin, rayDir, hit, triangle))
					return true;
			}
		}

		if (HitTest_CheckCell(gameState, (int)Floor(curX), (int)Floor(curY), swapXY, rayOrigin,
					rayDir, hit, triangle))
			return true;

		oldY = newY;
	}

	return false;
}

void GetAABB(Entity *entity, v3 *min, v3 *max)
{
	Collider *c = &entity->collider;
	ColliderType type = c->type;
	switch(type)
	{
	case COLLIDER_CONVEX_HULL:
	{
		*min = { INFINITY, INFINITY, INFINITY };
		*max = { -INFINITY, -INFINITY, -INFINITY };

		// @Speed: inverse-transform direction, pick a point, and then transform only that point!
		mat4 modelMatrix = Mat4ChangeOfBases(entity->fw, {0,0,1}, entity->pos);

		const ResourcePointCloud *pointsRes = &c->convexHull.pointCloud->points;
		u32 pointCount = pointsRes->pointCount;

		for (u32 i = 0; i < pointCount; ++i)
		{
			v3 p = pointsRes->pointData[i];
			v4 v = { p.x, p.y, p.z, 1.0f };
			v = Mat4TransformV4(modelMatrix, v);

			if (v.x < min->x) min->x = v.x;
			if (v.y < min->y) min->y = v.y;
			if (v.z < min->z) min->z = v.z;
			if (v.x > max->x) max->x = v.x;
			if (v.y > max->y) max->y = v.y;
			if (v.z > max->z) max->z = v.z;
		}
	} break;
	case COLLIDER_SPHERE:
	{
		f32 r = c->sphere.radius;
		*min = entity->pos + c->sphere.offset - v3{ r, r, r };
		*max = entity->pos + c->sphere.offset + v3{ r, r, r };
	} break;
	case COLLIDER_CYLINDER:
	{
		f32 halfH = c->cylinder.height * 0.5f;
		f32 r = c->cylinder.radius;
		*min = entity->pos + c->cylinder.offset - v3{ r, r, halfH };
		*max = entity->pos + c->cylinder.offset + v3{ r, r, halfH };
	} break;
	case COLLIDER_CAPSULE:
	{
		f32 halfH = c->cylinder.height * 0.5f;
		f32 r = c->cylinder.radius;
		*min = entity->pos + c->cylinder.offset - v3{ r, r, halfH + r };
		*max = entity->pos + c->cylinder.offset + v3{ r, r, halfH + r };
	} break;
	default:
	{
		ASSERT(false);
	}
	}

#if DEBUG_BUILD
	if (g_debugContext->drawAABBs)
	{
		DrawDebugWiredBox(*min, *max);
	}
#endif
}

v3 FurthestInDirection(Entity *entity, v3 dir)
{
	v3 result = {};

	Collider *c = &entity->collider;
	ColliderType type = c->type;
	switch(type)
	{
	case COLLIDER_CONVEX_HULL:
	{
		f32 maxDist = -INFINITY;

		mat4 modelMatrix = Mat4ChangeOfBases(entity->fw, {0,0,1}, entity->pos);

		// Un-rotate direction
		// @Speed: maybe do this with quaternions once decomposed transformations are a thing.
		mat4 rotMatrixInv =
		{
			modelMatrix.m00,	modelMatrix.m10,	modelMatrix.m20,	0,
			modelMatrix.m01,	modelMatrix.m11,	modelMatrix.m21,	0,
			modelMatrix.m02,	modelMatrix.m12,	modelMatrix.m22,	0,
			0,			0,		0,		1
		};
		v4 locDir4 = Mat4TransformV4(rotMatrixInv, v4{ dir.x, dir.y, dir.z, 0 });
		v3 locDir = { locDir4.x, locDir4.y, locDir4.z };

		const ResourcePointCloud *pointsRes = &c->convexHull.pointCloud->points;
		const u32 pointCount = pointsRes->pointCount;
		const v3 *scan = pointsRes->pointData;
		for (u32 i = 0; i < pointCount; ++i)
		{
			v3 p = *scan++;
			f32 dot = V3Dot(p, locDir);
			if (dot > maxDist)
			{
				maxDist = dot;
				result = p;
			}
		}

		// Transform point to world coordinates and return as result
		v4 r4 = { result.x, result.y, result.z, 1 };
		r4 = Mat4TransformV4(modelMatrix, r4);
		result = v3{ r4.x, r4.y, r4.z };
	} break;
	case COLLIDER_SPHERE:
	{
		result = entity->pos + c->sphere.offset + V3Normalize(dir) * c->sphere.radius;
	} break;
	case COLLIDER_CYLINDER:
	{
		result = entity->pos + c->cylinder.offset;

		f32 halfH = c->cylinder.height * 0.5f;
		if (dir.z != 0)
		{
			result.z += Sign(dir.z) * halfH;
		}
		if (dir.x != 0 || dir.y != 0)
		{
			f32 lat = Sqrt(dir.x * dir.x + dir.y * dir.y);
			result.x += dir.x / lat * c->cylinder.radius;
			result.y += dir.y / lat * c->cylinder.radius;
		}
	} break;
	case COLLIDER_CAPSULE:
	{
		result = entity->pos + c->cylinder.offset;

		f32 halfH = c->capsule.height * 0.5f;
		f32 lat = Sqrt(dir.x * dir.x + dir.y * dir.y);
		if (lat == 0)
		{
			// If direction is parallel to cylinder the answer is trivial
			result.z += Sign(dir.z) * (halfH + c->capsule.radius);
		}
		else
		{
			// Project dir into cylinder wall
			v3 d = dir / lat;
			d = d * c->capsule.radius;

			// If it goes out the top, return furthest point from top sphere
			if (d.z > 0)
				result += v3{ 0, 0, halfH } + V3Normalize(dir) * c->capsule.radius;
			// Analogue for bottom
			else
				result += v3{ 0, 0, -halfH } + V3Normalize(dir) * c->capsule.radius;
		}
	} break;
	default:
	{
		ASSERT(false);
	}
	}

#if DEBUG_BUILD
	if (g_debugContext->drawSupports)
		DrawDebugCubeAA(result, 0.04f, {0,1,1});
#endif

	return result;
}

inline v3 GJKSupport(Entity *vA, Entity *vB, v3 dir)
{
	v3 va = FurthestInDirection(vA, dir);
	v3 vb = FurthestInDirection(vB, -dir);
	return va - vb;
}

GJKResult GJKTest(Entity *vA, Entity *vB)
{
#if DEBUG_BUILD
	if (g_debugContext->GJKSteps[0] == nullptr)
	{
		for (u32 i = 0; i < ArrayCount(g_debugContext->GJKSteps); ++i)
			g_debugContext->GJKSteps[i] = (DebugVertex *)TransientAlloc(sizeof(DebugVertex) * 12);
	}
#endif

	GJKResult result;
	result.hit = true;

	v3 minA, maxA;
	GetAABB(vA, &minA, &maxA);
	v3 minB, maxB;
	GetAABB(vB, &minB, &maxB);
	if ((minA.x > maxB.x || minB.x > maxA.x) ||
		(minA.y > maxB.y || minB.y > maxA.y) ||
		(minA.z > maxB.z || minB.z > maxA.z))
	{
		result.hit = false;
		return result;
	}

	int foundPointsCount = 1;
	v3 testDir = { 0, 1, 0 }; // Random initial test direction

	result.points[0] = GJKSupport(vB, vA, testDir);
	testDir = -result.points[0];

	for (int iterations = 0; result.hit && foundPointsCount < 4; ++iterations)
	{
#if DEBUG_BUILD
		int i_ = iterations;
		if (!g_debugContext->freezeGJKGeom)
		{
			g_debugContext->GJKStepCounts[i_] = 0;
			g_debugContext->gjkStepCount = i_ + 1;
		}
#endif

		if (iterations >= 30)
		{
			Log("ERROR! GJK: Reached iteration limit!\n");
			//ASSERT(false);
#if DEBUG_BUILD
			g_debugContext->freezeGJKGeom = true;
#endif
			result.hit = false;
			break;
		}

		v3 a = GJKSupport(vB, vA, testDir);
		if (V3Dot(testDir, a) < 0)
		{
			result.hit = false;
			break;
		}

#if DEBUG_BUILD
		if ((!g_debugContext->freezeGJKGeom) && iterations)
		{
			g_debugContext->GJKNewPoint[iterations - 1] = a;
		}
#endif

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

#if DEBUG_BUILD
				if (!g_debugContext->freezeGJKGeom)
				{
					g_debugContext->GJKSteps[i_][g_debugContext->GJKStepCounts[i_]++] =
						{ a, v3{1,0,0} };
					g_debugContext->GJKSteps[i_][g_debugContext->GJKStepCounts[i_]++] =
						{ b, v3{1,0,0} };
					g_debugContext->GJKSteps[i_][g_debugContext->GJKStepCounts[i_]++] =
						{ c, v3{1,0,0} };
					ASSERT(g_debugContext->GJKStepCounts[i_] == 3);
				}
#endif

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

#if DEBUG_BUILD
				if (!g_debugContext->freezeGJKGeom)
				{
					g_debugContext->GJKSteps[i_][g_debugContext->GJKStepCounts[i_]++] =
						{ b, v3{1,0,0} };
					g_debugContext->GJKSteps[i_][g_debugContext->GJKStepCounts[i_]++] =
						{ d, v3{1,0,0} };
					g_debugContext->GJKSteps[i_][g_debugContext->GJKStepCounts[i_]++] =
						{ c, v3{1,0,0} };

					g_debugContext->GJKSteps[i_][g_debugContext->GJKStepCounts[i_]++] =
						{ a, v3{0,1,0} };
					g_debugContext->GJKSteps[i_][g_debugContext->GJKStepCounts[i_]++] =
						{ b, v3{0,1,0} };
					g_debugContext->GJKSteps[i_][g_debugContext->GJKStepCounts[i_]++] =
						{ c, v3{0,1,0} };

					g_debugContext->GJKSteps[i_][g_debugContext->GJKStepCounts[i_]++] =
						{ a, v3{0,0,1} };
					g_debugContext->GJKSteps[i_][g_debugContext->GJKStepCounts[i_]++] =
						{ d, v3{0,0,1} };
					g_debugContext->GJKSteps[i_][g_debugContext->GJKStepCounts[i_]++] =
						{ b, v3{0,0,1} };

					g_debugContext->GJKSteps[i_][g_debugContext->GJKStepCounts[i_]++] =
						{ a, v3{1,0,1} };
					g_debugContext->GJKSteps[i_][g_debugContext->GJKStepCounts[i_]++] =
						{ c, v3{1,0,1} };
					g_debugContext->GJKSteps[i_][g_debugContext->GJKStepCounts[i_]++] =
						{ d, v3{1,0,1} };

					ASSERT(g_debugContext->GJKStepCounts[i_] == 3 * 4);
				}
#endif

				// Assert normals point outside
				if (V3Dot(d - a, abcNor) > 0)
				{
					Log("ERROR: ABC normal facing inward! (dot=%f)\n",
							V3Dot(d - a, abcNor));
				}
				if (V3Dot(c - a, adbNor) > 0)
				{
					Log("ERROR: ADB normal facing inward! (dot=%f)\n",
							V3Dot(d - a, abcNor));
				}
				if (V3Dot(b - a, acdNor) > 0)
				{
					Log("ERROR: ACD normal facing inward! (dot=%f)\n",
							V3Dot(d - a, abcNor));
				}

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

v3 ComputeDepenetration(GJKResult gjkResult, Entity *vA, Entity *vB)
{
#if DEBUG_BUILD
	if (g_debugContext->polytopeSteps[0] == nullptr)
	{
		for (u32 i = 0; i < ArrayCount(g_debugContext->polytopeSteps); ++i)
			g_debugContext->polytopeSteps[i] = (DebugVertex *)TransientAlloc(sizeof(DebugVertex) * 256);
	}
#endif

	VERBOSE_LOG("\n\nNew EPA calculation\n");

	// By now we should have the tetrahedron from GJK
	// TODO dynamic array?
	EPAFace polytope[256];
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

	EPAFace closestFeature;
	f32 lastLeastDistance = -1.0f;
	const int maxIterations = 64;
	for (int epaStep = 0; epaStep < maxIterations; ++epaStep)
	{
		ASSERT(epaStep < maxIterations - 1);

#if DEBUG_BUILD
		// Save polytope for debug visualization
		if (!g_debugContext->freezePolytopeGeom && epaStep < DebugContext::epaMaxSteps)
		{
			GenPolytopeMesh(polytope, polytopeCount, g_debugContext->polytopeSteps[epaStep]);
			g_debugContext->polytopeStepCounts[epaStep] = polytopeCount;
			g_debugContext->epaStepCount = epaStep + 1;
		}
#endif

		// Find closest feature to origin in polytope
		EPAFace newClosestFeature = {};
		int validFacesFound = 0;
		f32 leastDistance = INFINITY;
		VERBOSE_LOG("Looking for closest feature\n");
		for (int faceIdx = 0; faceIdx < polytopeCount; ++faceIdx)
		{
			EPAFace *face = &polytope[faceIdx];
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
			Log("ERROR: EPA: Couldn't find closest feature!");
			// Collision is probably on the very edge, we don't need depenetration
			return {};
		}
		else if (leastDistance <= lastLeastDistance + 0.0001f)
		{
			// We tried to expand in this direction but failed! So we are at the edge of
			// the Minkowski difference
			// TODO do we really need two exit conditions? Something must be wrong with
			// the other one!
			VERBOSE_LOG("Ending because couldn't get any further away from the outer edge");
			break;
		}
		else
		{
			closestFeature = newClosestFeature;
			lastLeastDistance = leastDistance;
			VERBOSE_LOG("Picked face with distance %.02f out of %d faces\n", leastDistance,
					validFacesFound);
		}

		// Expand polytope!
		v3 testDir = V3Cross(closestFeature.c - closestFeature.a, closestFeature.b - closestFeature.a);
		v3 newPoint = GJKSupport(vB, vA, testDir);
		VERBOSE_LOG("Found new point { %.02f, %.02f. %.02f } while looking in direction { %.02f, %.02f. %.02f }\n",
				newPoint.x, newPoint.y, newPoint.z, testDir.x, testDir.y, testDir.z);
#if DEBUG_BUILD
		if (!g_debugContext->freezePolytopeGeom && epaStep < DebugContext::epaMaxSteps)
			g_debugContext->epaNewPoint[epaStep] = newPoint;
#endif
		// Without a little epsilon here we can sometimes pick a point that's already part of the
		// polytope, resulting in weird artifacts later on. I guess we could manually check for that
		// but this should be good enough.
		const f32 epsilon = 0.000001f;
		if (V3Dot(testDir, newPoint - closestFeature.a) <= epsilon)
		{
			VERBOSE_LOG("Done! Couldn't find a closer point\n");
			break;
		}
#if DEBUG_BUILD
		else if (V3Dot(testDir, newPoint - closestFeature.b) <= epsilon)
		{
			Log("ERROR! EPA: Redundant check triggered (B)\n");
			//ASSERT(false);
			break;
		}
		else if (V3Dot(testDir, newPoint - closestFeature.c) <= epsilon)
		{
			Log("ERROR! EPA: Redundant check triggered (C)\n");
			//ASSERT(false);
			break;
		}
#endif
		EPAEdge holeEdges[256];
		int holeEdgesCount = 0;
		int oldPolytopeCount = polytopeCount;
		for (int faceIdx = 0; faceIdx < polytopeCount; )
		{
			EPAFace *face = &polytope[faceIdx];
			v3 normal = V3Cross(face->c - face->a, face->b - face->a);
			// Ignore if not facing new point
			if (V3Dot(normal, newPoint - face->a) <= 0)
			{
				++faceIdx;
				continue;
			}

			// Add/remove edges to the hole (XOR)
			EPAEdge faceEdges[3] =
			{
				{ face->a, face->b },
				{ face->b, face->c },
				{ face->c, face->a }
			};
			for (int edgeIdx = 0; edgeIdx < 3; ++edgeIdx)
			{
				const EPAEdge &edge = faceEdges[edgeIdx];
				// If it's already on the list, remove it
				bool found = false;
				for (int holeEdgeIdx = 0; holeEdgeIdx < holeEdgesCount; ++holeEdgeIdx)
				{
					const EPAEdge &holeEdge = holeEdges[holeEdgeIdx];
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
		VERBOSE_LOG("Deleted %d faces which were facing new point\n", deletedFaces);
		VERBOSE_LOG("Presumably left a hole with %d edges\n", holeEdgesCount);
		if (deletedFaces > 1 && holeEdgesCount >= deletedFaces * 3)
		{
			Log("ERROR! EPA: Multiple holes were made on the polytope!\n");
#if DEBUG_BUILD
			if (!g_debugContext->freezePolytopeGeom && epaStep < DebugContext::epaMaxSteps - 1)
			{
				GenPolytopeMesh(polytope, polytopeCount, g_debugContext->polytopeSteps[epaStep + 1]);
				g_debugContext->polytopeStepCounts[epaStep + 1] = polytopeCount;
				g_debugContext->epaNewPoint[epaStep + 1] = newPoint;
				g_debugContext->freezePolytopeGeom = true;
			}
#endif
		}
		oldPolytopeCount = polytopeCount;
		// Now we should have a hole in the polytope, of which all edges are in holeEdges

		for (int holeEdgeIdx = 0; holeEdgeIdx < holeEdgesCount; ++holeEdgeIdx)
		{
			const EPAEdge &holeEdge = holeEdges[holeEdgeIdx];
			EPAFace newFace = { holeEdge.a, holeEdge.b, newPoint };
			polytope[polytopeCount++] = newFace;
		}
		VERBOSE_LOG("Added %d faces to fill the hole. Polytope now has %d faces\n",
				polytopeCount - oldPolytopeCount, polytopeCount);
	}

	v3 closestFeatureNor = V3Normalize(V3Cross(closestFeature.c - closestFeature.a,
			closestFeature.b - closestFeature.a));
	return closestFeatureNor * V3Dot(closestFeatureNor, closestFeature.a);
}

#if DEBUG_BUILD
void GetGJKStepGeometry(int step, DebugVertex **buffer, u32 *count)
{
	*count = g_debugContext->GJKStepCounts[step];
	*buffer = g_debugContext->GJKSteps[step];
}

void GetEPAStepGeometry(int step, DebugVertex **buffer, u32 *count)
{
	*count = g_debugContext->polytopeStepCounts[step];
	*buffer = g_debugContext->polytopeSteps[step];
}
#endif

#undef VERBOSE_LOG
