u32 HashRawVertex(const RawVertex &v)
{
	u32 result = 0;
	for (u32 counter = 0; counter < sizeof(RawVertex); counter++)
	{
		result = ((u8 *)&v)[counter] + (result << 6) + (result << 16) - result;
	}
	return result;
}

ErrorCode ConstructSkinnedMesh(const RawGeometry *geometry, const WeightData *weightData,
		DynamicArray_RawVertex &finalVertices, DynamicArray_u16 &finalIndices)
{
	void *oldStackPtr = g_memory->stackPtr;

	const u32 vertexTotal = geometry->positions.size;

	Array_SkinnedPosition skinnedPositions;
	ArrayInit_SkinnedPosition(&skinnedPositions, vertexTotal, StackAlloc);
	for (u32 i = 0; i < vertexTotal; ++i)
	{
		SkinnedPosition *skinnedPos = ArrayAdd_SkinnedPosition(&skinnedPositions);
		*skinnedPos = {};
		skinnedPos->pos = geometry->positions[i];
	}

	// Inject weights into skinned positions
	if (weightData)
	{
		const int *weightCountScan = weightData->weightCounts;
		const int *weightIndexScan = weightData->weightIndices;
		for (u32 i = 0; i < skinnedPositions.size; ++i)
		{
			SkinnedPosition *skinnedPos = &skinnedPositions[i];

			int jointsNum = *weightCountScan++;
			ASSERT(jointsNum <= 4);
			for (int j = 0; j < jointsNum; ++j)
			{
				skinnedPos->jointIndices[j] = (u8)*weightIndexScan++;
				skinnedPos->jointWeights[j] = weightData->weights[*weightIndexScan++];
			}
			for (int j = jointsNum; j < 4; ++j)
			{
				skinnedPos->jointIndices[j] = 0;
				skinnedPos->jointWeights[j] = 0;
			}

			f32 weightTotal = 
					skinnedPos->jointWeights[0] + skinnedPos->jointWeights[1] +
					skinnedPos->jointWeights[2] + skinnedPos->jointWeights[3];
			ASSERT(weightTotal > 0.99999f && weightTotal < 1.000001f);
		}
	}

	// Join positions and normals and eliminate duplicates
	{
		void *oldStackPtrJoin = g_memory->stackPtr;

		const bool hasUvs = geometry->uvsOffset >= 0;
		const bool hasNormals = geometry->normalsOffset >= 0;
		const int attrCount = 1 + hasUvs + hasNormals;

		DynamicArrayInit_u16(&finalIndices, geometry->triangleCount * 3, FrameAlloc);

		// We make the number of buckets the next power of 2 after the count
		// to minimize collisions with a reasonable amount of memory.
		int nOfBuckets = 128;
		// Assume roughly one unique vertex per triangle, which is reasonable
		// for smooth triangles that share all of their verices most of the
		// time.
		while (nOfBuckets < geometry->triangleCount)
		{
			nOfBuckets *= 2;
		}
		//Log("Going with %d buckets for %d indices\n", nOfBuckets, geometry->triangleCount * 3);

		DynamicArray_u16 *buckets = (DynamicArray_u16 *)StackAlloc(sizeof(DynamicArray_u16) *
				nOfBuckets);
		for (int bucketIdx = 0; bucketIdx < nOfBuckets; ++bucketIdx)
		{
			DynamicArrayInit_u16(&buckets[bucketIdx], 4, StackAlloc);
		}

		// nOfBuckets should be a good enough estimation for the size of this array too
		const u32 initialCapacity = nOfBuckets;
		DynamicArrayInit_RawVertex(&finalVertices, initialCapacity, FrameAlloc);

		for (int triangleIdx = 0; triangleIdx < geometry->triangleCount * 3; ++triangleIdx)
		{
			const int posIdx = geometry->triangleIndices[triangleIdx * attrCount + geometry->verticesOffset];
			SkinnedPosition pos = skinnedPositions[posIdx];

			int uvIdx = 0;
			v2 uv = {};
			if (hasUvs)
			{
				uvIdx = geometry->triangleIndices[triangleIdx * attrCount + geometry->uvsOffset];
				uv = geometry->uvs[uvIdx];
			}

			int norIdx = 0;
			v3 nor = {};
			if (hasNormals)
			{
				norIdx = geometry->triangleIndices[triangleIdx * attrCount + geometry->normalsOffset];
				nor = geometry->normals[norIdx];
			}

			RawVertex newVertex = { pos, uv, nor };

			u16 index = 0xFFFF;
			const u32 hash = HashRawVertex(newVertex);
			DynamicArray_u16 &bucket = (buckets[hash & (nOfBuckets - 1)]);
			for (u32 i = 0; i < bucket.size; ++i)
			{
				u16 vertexIndex = bucket[i];
				RawVertex *other = &finalVertices[vertexIndex];
				if (memcmp(other, &newVertex, sizeof(RawVertex)) == 0)
				{
					index = vertexIndex;
					break;
				}
			}

			if (index == 0xFFFF)
			{
				// Push back vertex
				*DynamicArrayAdd_RawVertex(&finalVertices, FrameRealloc) = newVertex;
				u64 newVertexIdx = finalVertices.size - 1;
				ASSERT(newVertexIdx < U16_MAX);
				*DynamicArrayAdd_u16(&finalIndices, FrameRealloc) = (u16)newVertexIdx;

				// Add to map
				*DynamicArrayAdd_u16(&bucket, StackRealloc) = (u16)newVertexIdx;
			}
			else
			{
				*DynamicArrayAdd_u16(&finalIndices, FrameRealloc) = index;
			}
		}

		int total = 0;
		int collisions = 0;
		for (int bucketIdx = 0; bucketIdx < nOfBuckets; ++bucketIdx)
		{
			DynamicArray_u16 &bucket = (buckets[bucketIdx]);
			collisions += bucket.size ? bucket.size - 1 : 0;
			total += bucket.size;
		}

#if 0
		Log("Vertex indexing collided %d times out of %d (%.02f%%)\n", collisions, total,
				100.0f * collisions / (f32)total);

		Log("Vertex indexing used %.02fkb of stack\n", ((u8 *)stackPtr - (u8 *)oldStackPtr) /
				1024.0f);
#endif
		StackFree(oldStackPtrJoin);
	}

	Log("Ended up with %d unique vertices and %d indices\n", finalVertices.size,
			finalIndices.size);

	// Rewind triangles
	for (int i = 0; i < geometry->triangleCount; ++i)
	{
		u16 tmp = finalIndices[i * 3 + 1];
		finalIndices[i * 3 + 1] = finalIndices[i * 3 + 2];
		finalIndices[i * 3 + 2] = tmp;
	}

	StackFree(oldStackPtr);

	return ERROR_OK;
}

ErrorCode OutputMesh(const char *filename, DynamicArray_RawVertex &finalVertices,
		DynamicArray_u16 &finalIndices, const char *materialFilename)
{
	// Output!
	{
		FileHandle file = PlatformOpenForWrite(filename);
		{
			BakeryMeshHeader header;
			header.vertexCount = finalVertices.size;
			header.indexCount = finalIndices.size;

			header.vertexBlobOffset = PlatformFileSeek(file, sizeof(header), SEEK_SET);
			// Write vertex data
			for (u32 i = 0; i < finalVertices.size; ++i)
			{
				RawVertex *rawVertex = &finalVertices[i];
				Vertex gameVertex;
				gameVertex.pos = rawVertex->skinnedPos.pos;
				gameVertex.uv = rawVertex->uv;
				gameVertex.nor = rawVertex->normal;

				PlatformWriteToFile(file, &gameVertex, sizeof(gameVertex));
			}

			header.indexBlobOffset = FilePosition(file);
			// Write indices
			for (u32 i = 0; i < finalIndices.size; ++i)
			{
				u16 outputIdx = finalIndices[i];
				PlatformWriteToFile(file, &outputIdx, sizeof(outputIdx));
			}

			header.materialNameOffset = FilePosition(file);
			if (materialFilename)
				PlatformWriteToFile(file, materialFilename, strlen(materialFilename) + 1);
			else
			{
				const u8 zero = 0;
				PlatformWriteToFile(file, &zero, 1);
			}

			PlatformFileSeek(file, 0, SEEK_SET);
			PlatformWriteToFile(file, &header, sizeof(header));
		}
		PlatformCloseFile(file);
	}

	return ERROR_OK;
}

ErrorCode OutputSkinnedMesh(const char *filename,
		DynamicArray_RawVertex &finalVertices, DynamicArray_u16 &finalIndices, Skeleton &skeleton,
		DynamicArray_Animation &animations, const char *materialFilename)
{
	void *oldStackPtr = g_memory->stackPtr;

	FileHandle file = PlatformOpenForWrite(filename);
	
	BakerySkinnedMeshHeader header;
	header.vertexCount = finalVertices.size;
	header.indexCount = finalIndices.size;
	header.jointCount = skeleton.jointCount;
	header.animationCount = animations.size;
	PlatformFileSeek(file, sizeof(header), SEEK_SET);

	// Write vertex data
	header.vertexBlobOffset = FilePosition(file);
	for (u32 i = 0; i < finalVertices.size; ++i)
	{
		RawVertex *rawVertex = &finalVertices[i];
		SkinnedVertex gameVertex;
		gameVertex.pos = rawVertex->skinnedPos.pos;
		gameVertex.uv = rawVertex->uv;
		gameVertex.nor = rawVertex->normal;
		for (int j = 0; j < 4; ++j)
		{
			gameVertex.indices[j] = rawVertex->skinnedPos.jointIndices[j];
			gameVertex.weights[j] = rawVertex->skinnedPos.jointWeights[j];
		}

		PlatformWriteToFile(file, &gameVertex, sizeof(gameVertex));
	}

	FileAlign(file);

	// Write indices
	header.indexBlobOffset = FilePosition(file);
	for (u32 i = 0; i < finalIndices.size; ++i)
	{
		u16 outputIdx = finalIndices[i];
		PlatformWriteToFile(file, &outputIdx, sizeof(outputIdx));
	}

	FileAlign(file);

	// Write skeleton
	header.bindPosesBlobOffset = FilePosition(file);
	PlatformWriteToFile(file, skeleton.bindPoses, sizeof(Transform) * skeleton.jointCount);
	FileAlign(file);

	header.jointParentsBlobOffset = FilePosition(file);
	PlatformWriteToFile(file, skeleton.jointParents, skeleton.jointCount);
	FileAlign(file);

	header.restPosesBlobOffset = FilePosition(file);
	PlatformWriteToFile(file, skeleton.restPoses, sizeof(Transform) * skeleton.jointCount);
	FileAlign(file);

	// Write animations
	Array_BakerySkinnedMeshAnimationHeader animationHeaders;
	ArrayInit_BakerySkinnedMeshAnimationHeader(&animationHeaders, animations.size, StackAlloc);
	for (u32 animIdx = 0; animIdx < animations.size; ++animIdx)
	{
		Animation *animation = &animations[animIdx];

		BakerySkinnedMeshAnimationHeader *animationHeader = &animationHeaders[animIdx];
		animationHeader->frameCount = animation->frameCount;
		animationHeader->channelCount = animation->channelCount;
		animationHeader->loop = animation->loop;

		animationHeader->timestampsBlobOffset = FilePosition(file);
		PlatformWriteToFile(file, animation->timestamps, sizeof(u32) * animation->frameCount);

		Array_BakerySkinnedMeshAnimationChannelHeader channelHeaders;
		ArrayInit_BakerySkinnedMeshAnimationChannelHeader(&channelHeaders, animation->channelCount,
				StackAlloc);
		for (u32 channelIdx = 0; channelIdx < animation->channelCount; ++channelIdx)
		{
			AnimationChannel *channel = &animation->channels[channelIdx];

			BakerySkinnedMeshAnimationChannelHeader *channelHeader = &channelHeaders[channelIdx];
			channelHeader->jointIndex = channel->jointIndex;
			channelHeader->transformsBlobOffset = FilePosition(file);
			PlatformWriteToFile(file, channel->transforms, sizeof(Transform) * animation->frameCount);
		}

		animationHeader->channelsBlobOffset = FilePosition(file);
		for (u32 channelIdx = 0; channelIdx < animation->channelCount; ++channelIdx)
		{
			BakerySkinnedMeshAnimationChannelHeader *channelHeader = &channelHeaders[channelIdx];
			PlatformWriteToFile(file, channelHeader, sizeof(*channelHeader));
		}
	}

	header.animationBlobOffset = FilePosition(file);
	for (u32 animIdx = 0; animIdx < animations.size; ++animIdx)
	{
		BakerySkinnedMeshAnimationHeader *animationHeader = &animationHeaders[animIdx];
		PlatformWriteToFile(file, animationHeader, sizeof(*animationHeader));
	}

	header.materialNameOffset = FilePosition(file);
	if (materialFilename)
		PlatformWriteToFile(file, materialFilename, strlen(materialFilename) + 1);
	else
	{
		const u8 zero = 0;
		PlatformWriteToFile(file, &zero, 1);
	}

	PlatformFileSeek(file, 0, SEEK_SET);
	PlatformWriteToFile(file, &header, sizeof(header));

	PlatformCloseFile(file);

	StackFree(oldStackPtr);

	return ERROR_OK;
}

void GenerateGeometryGrid(const Array_v3 &positions, const Array_IndexTriangle &triangles, GeometryGrid *geometryGrid)
{
	void *oldStackPtr = g_memory->stackPtr;

	v2 lowCorner = { INFINITY, INFINITY };
	v2 highCorner = { -INFINITY, -INFINITY };

	// Get limits
	for (u32 i = 0; i < positions.size; ++i)
	{
		v3 p = positions[i];
		if (p.x < lowCorner.x) lowCorner.x = p.x;
		if (p.x > highCorner.x) highCorner.x = p.x;
		if (p.y < lowCorner.y) lowCorner.y = p.y;
		if (p.y > highCorner.y) highCorner.y = p.y;
	}

	const int cellsSide = 4;
	const int cellCount = cellsSide * cellsSide;
	const v2 span = (highCorner - lowCorner);
	const v2 cellSize = span / (f32)(cellsSide);

	Log("Geometry grid limits: { %.02f, %.02f } - { %.02f, %.02f }\n", lowCorner.x, lowCorner.y,
			highCorner.x, highCorner.y);

	// Init buckets
	DynamicArray_IndexTriangle *cellBuckets = (DynamicArray_IndexTriangle *)
		StackAlloc(sizeof(DynamicArray_IndexTriangle) * cellCount);
	for (int i = 0; i < cellCount; ++i)
	{
		DynamicArrayInit_IndexTriangle(&cellBuckets[i], 256, StackAlloc);
	}
	u32 totalTriangleCount = 0;

	for (u32 triangleIdx = 0; triangleIdx < triangles.size; ++triangleIdx)
	{
		const IndexTriangle *curTriangle = &triangles[triangleIdx];

		v2 corners[3];
		for (int i = 0; i < 3; ++i)
		{
			v3 worldP = positions[curTriangle->corners[i]];

			// Subtract epsilon from vertices on the high edge, since Floor(p) would give out of
			// bounds indices.
			const f32 epsilon = 0.00001f;
			if (worldP.x >= highCorner.x)
				worldP.x -= epsilon;
			if (worldP.y >= highCorner.y)
				worldP.y -= epsilon;

			v2 p = {
				(worldP.x - lowCorner.x) / cellSize.x,
				(worldP.y - lowCorner.y) / cellSize.y
			};

			corners[i] = { p.x, p.y };
		}
		// Sort by Y ascending
		for (int max = 2; max > 0; --max)
		{
			for (int i = 0; i < max; ++i)
			{
				if (corners[i].y > corners[i + 1].y)
				{
					// Swap
					v2 tmp = corners[i];
					corners[i] = corners[i + 1];
					corners[i + 1] = tmp;
				}
			}
		}

		// If the whole triangle fits in a single row, just fill from left to right
		if (Floor(corners[0].y) == Floor(corners[2].y))
		{
			int scanlineY = (int)Floor(corners[0].y);
			f32 left = Min(corners[0].x, Min(corners[1].x, corners[2].x));
			f32 right = Max(corners[0].x, Max(corners[1].x, corners[2].x));
			for (int scanX = (int)Floor(left);
				scanX <= (int)Floor(right);
				++scanX)
			{
				ASSERT(scanX >= 0 && scanX < cellsSide);
				DynamicArray_IndexTriangle &bucket = cellBuckets[scanX + scanlineY * cellsSide];
				*DynamicArrayAdd_IndexTriangle(&bucket, StackRealloc) = *curTriangle;
				++totalTriangleCount;
			}
			continue;
		}

		// We split the triangle into a flat-bottomed triangle and a flat-topped one, and rasterize
		// them individually
		f32 lerpT = (corners[1].y - corners[0].y) / (corners[2].y - corners[0].y);
		v2 midPoint = corners[0] * (1.0f - lerpT) + corners[2] * lerpT;

		// If top half fits in one row, just scan left to right
		if (Floor(corners[1].y) == Floor(corners[2].y))
		{
			int scanlineY = (int)Floor(corners[1].y);
			f32 left = Min(midPoint.x, Min(corners[1].x, corners[2].x));
			f32 right = Max(midPoint.x, Max(corners[1].x, corners[2].x));
			for (int scanX = (int)Floor(left);
				scanX <= (int)Floor(right);
				++scanX)
			{
				ASSERT(scanX >= 0 && scanX < cellsSide);
				DynamicArray_IndexTriangle &bucket = cellBuckets[scanX + scanlineY * cellsSide];
				*DynamicArrayAdd_IndexTriangle(&bucket, StackRealloc) = *curTriangle;
				++totalTriangleCount;
			}
		}
		// Otherwise rasterize top triangle
		else
		{
			f32 invSlope1 = (corners[1].x - corners[2].x) / (corners[1].y - corners[2].y);
			f32 invSlope2 = (corners[0].x - corners[2].x) / (corners[0].y - corners[2].y);
			if (invSlope1 < invSlope2)
			{
				f32 tmp = invSlope1;
				invSlope1 = invSlope2;
				invSlope2 = tmp;
			}

			int bottom = (int)Floor(corners[1].y);
			int top = (int)Floor(corners[2].y);

			f32 curX1 = corners[2].x;
			f32 curX2 = corners[2].x;
			// The top point can be anywhere between the top of the row and the bottom, so advance
			// until we meet the bottom border of the row
			f32 advTillNextGridLine = Fmod(corners[2].y, 1.0f);
			f32 nextX1 = curX1 - invSlope1 * advTillNextGridLine;
			f32 nextX2 = curX2 - invSlope2 * advTillNextGridLine;
			ASSERT(bottom >= 0 && top < cellsSide);
			for (int scanlineY = top; scanlineY >= bottom; --scanlineY)
			{
				for (int scanX = (int)Floor(Min(curX1, nextX1));
					scanX <= (int)Floor(Max(curX2, nextX2));
					++scanX)
				{
					ASSERT(scanX >= 0 && scanX < cellsSide);

					DynamicArray_IndexTriangle &bucket = cellBuckets[scanX + scanlineY * cellsSide];
					*DynamicArrayAdd_IndexTriangle(&bucket, StackRealloc) = *curTriangle;
					++totalTriangleCount;
				}
				curX1 = nextX1;
				curX2 = nextX2;
				if (scanlineY - 1 > bottom)
				{
					nextX1 -= invSlope1;
					nextX2 -= invSlope2;
				}
				else
				{
					// For the last one, only advance as much as there's left.
					const f32 remain = 1.0f - Fmod(corners[1].y, 1.0f);
					nextX1 -= invSlope1 * remain;
					nextX2 -= invSlope2 * remain;
				}
			}
		}
		// Rasterize bottom triangle
		// Pretty much the same as above but upside down
		if (Floor(corners[0].y) == Floor(corners[1].y))
		{
			int scanlineY = (int)Floor(corners[1].y);
			f32 left = Min(corners[0].x, Min(corners[1].x, midPoint.x));
			f32 right = Max(corners[0].x, Max(corners[1].x, midPoint.x));
			for (int scanX = (int)Floor(left);
				scanX <= (int)Floor(right);
				++scanX)
			{
				ASSERT(scanX >= 0 && scanX < cellsSide);
				DynamicArray_IndexTriangle &bucket = cellBuckets[scanX + scanlineY * cellsSide];
				*DynamicArrayAdd_IndexTriangle(&bucket, StackRealloc) = *curTriangle;
				++totalTriangleCount;
			}
		}
		else
		{
			f32 invSlope1 = (f32)(corners[1].x - corners[0].x) / (f32)(corners[1].y - corners[0].y);
			f32 invSlope2 = (f32)(corners[2].x - corners[0].x) / (f32)(corners[2].y - corners[0].y);
			if (invSlope1 > invSlope2)
			{
				f32 tmp = invSlope1;
				invSlope1 = invSlope2;
				invSlope2 = tmp;
			}

			int bottom = (int)Floor(corners[0].y);
			int top = (int)Floor(corners[1].y);

			f32 curX1 = corners[0].x;
			f32 curX2 = corners[0].x;
			f32 advTillNextGridLine = 1.0f - Fmod(corners[0].y, 1.0f);
			f32 nextX1 = curX1 + invSlope1 * advTillNextGridLine;
			f32 nextX2 = curX2 + invSlope2 * advTillNextGridLine;
			ASSERT(bottom >= 0 && top < cellsSide);
			for (int scanlineY = bottom; scanlineY < top; ++scanlineY)
			{
				for (int scanX = (int)Floor(Min(curX1, nextX1));
					scanX <= (int)Floor(Max(curX2, nextX2));
					++scanX)
				{
					ASSERT(scanX >= 0 && scanX < cellsSide);

					DynamicArray_IndexTriangle &bucket = cellBuckets[scanX + scanlineY * cellsSide];
					*DynamicArrayAdd_IndexTriangle(&bucket, StackRealloc) = *curTriangle;
					++totalTriangleCount;
				}
				curX1 = nextX1;
				curX2 = nextX2;
				nextX1 += invSlope1;
				nextX2 += invSlope2;
			}
		}
	}

	geometryGrid->lowCorner = lowCorner;
	geometryGrid->highCorner = highCorner;
	geometryGrid->cellsSide = cellsSide;
	geometryGrid->offsets = (u32 *)FrameAlloc(sizeof(u32) * (cellCount + 1));
	geometryGrid->triangles = (IndexTriangle *)FrameAlloc(sizeof(IndexTriangle) * totalTriangleCount);

	int triangleCount = 0;
	for (int y = 0; y < cellsSide; ++y)
	{
		for (int x = 0; x < cellsSide; ++x)
		{
			int i = x + y * cellsSide;
			geometryGrid->offsets[i] = (u32)triangleCount;

			DynamicArray_IndexTriangle &bucket = cellBuckets[i];
			for (u32 bucketIdx = 0; bucketIdx < bucket.size; ++bucketIdx)
			{
				geometryGrid->triangles[triangleCount++] = bucket[bucketIdx];
			}
		}
	}
	ASSERT(totalTriangleCount == (u32)triangleCount);
	geometryGrid->offsets[cellCount] = (u32)triangleCount;

	StackFree(oldStackPtr);
}
