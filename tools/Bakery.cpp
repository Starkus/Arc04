#include <windows.h>
#include <strsafe.h>
#include <stdio.h>

#include "tinyxml/tinyxml2.cpp"
using namespace tinyxml2;

HANDLE g_hStdout;
void Log(const char *format, ...)
{
	char buffer[2048];
	va_list args;
	va_start(args, format);

	StringCbVPrintfA(buffer, sizeof(buffer), format, args);
	OutputDebugStringA(buffer);

	DWORD bytesWritten;
	WriteFile(g_hStdout, buffer, (DWORD)strlen(buffer), &bytesWritten, nullptr);

	va_end(args);
}

inline FILETIME Win32GetLastWriteTime(const char *filename)
{
	FILETIME lastWriteTime = {};

	WIN32_FILE_ATTRIBUTE_DATA data;
	if (GetFileAttributesEx(filename, GetFileExInfoStandard, &data))
	{
		lastWriteTime = data.ftLastWriteTime;
	}

	return lastWriteTime;
}

#include "General.h"
#include "Maths.h"
#include "BakeryInterop.h"
#include "Platform.h"
#include "Containers.h"
#include "Geometry.h"
#include "Bakery.h"

GameMemory *g_gameMemory;

#include "Memory.cpp"

void GetDataPath(char *dataPath)
{
	GetModuleFileNameA(0, dataPath, MAX_PATH);

	// Get parent directory
	char *lastSlash = dataPath;
	char *secondLastSlash = dataPath;
	for (char *scan = dataPath; *scan != 0; ++scan)
	{
		if (*scan == '\\')
		{
			secondLastSlash = lastSlash;
			lastSlash = scan;
		}
	}

	// Go into data dir
	strcpy(secondLastSlash + 1, dataDir);
}

bool Win32DidFileChange(const char *fullName, Array_FileCacheEntry &cache)
{
	bool changed = true;
	FileCacheEntry *cacheEntry = 0;
	FILETIME newWriteTime = Win32GetLastWriteTime(fullName);
	for (u32 i = 0; i < cache.size; ++i)
	{
		if (strcmp(fullName, cache[i].filename) == 0)
		{
			cacheEntry = &cache[i];

			if (!cacheEntry->changed && CompareFileTime(&newWriteTime, &cache[i].lastWriteTime) == 0)
			{
				changed = false;
			}
			break;
		}
	}
#if 1
	if (changed)
	{
		if (!cacheEntry)
		{
			cacheEntry = &cache[cache.size++];
			strcpy(cacheEntry->filename, fullName);
		}
		cacheEntry->lastWriteTime = newWriteTime;
		cacheEntry->changed = true;
	}
#endif
	return changed;
}

HANDLE OpenForWrite(const char *filename)
{
	HANDLE file = CreateFileA(
			filename,
			GENERIC_WRITE,
			0, // Share
			nullptr,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
			);
	ASSERT(file != INVALID_HANDLE_VALUE);

	return file;
}

u64 WriteToFile(HANDLE file, void *buffer, u64 size)
{
	DWORD writtenBytes;
	WriteFile(
			file,
			buffer,
			(DWORD)size,
			&writtenBytes,
			nullptr
			);
	ASSERT(writtenBytes == size);

	return (u64)writtenBytes;
}

u64 FileSeek(HANDLE file, i64 shift, DWORD mode)
{
	LARGE_INTEGER lInt;
	LARGE_INTEGER lIntRes;
	lInt.QuadPart = shift;

	SetFilePointerEx(file, lInt, &lIntRes, mode);

	return lIntRes.QuadPart;
}

#define FilePosition(file) FileSeek(file, 0, FILE_CURRENT)
#define FileAlign(fileHandle) FileSeek(fileHandle, FilePosition(fileHandle) & 0b11, FILE_CURRENT)

u32 HashRawVertex(const RawVertex &v)
{
	u32 result = 0;
	for (u32 counter = 0; counter < sizeof(RawVertex); counter++)
	{
		result = ((u8 *)&v)[counter] + (result << 6) + (result << 16) - result;
	}
	return result;
}

void GetOutputFilename(const char *metaFilename, char *outputFilename)
{
	strcpy(outputFilename, metaFilename);
	// Change extension
	char *lastDot = 0;
	for (char *scan = outputFilename; ; ++scan)
	{
		if (*scan == '.')
		{
			lastDot = scan;
		}
		else if (*scan == 0)
		{
			if (!lastDot)
				lastDot = scan;
			break;
		}
	}
	strcpy(lastDot, ".bin\0");
	Log("Output name: %s\n", outputFilename);
}

void ReadStringToFloats(const char *text, f32 *buffer, int count)
{
	f32 *bufferScan = buffer;
	int readCount = 0;

	char numBuffer[32];
	char *numBufferScan = numBuffer;
	for (const char *scan = text; ; ++scan)
	{
		if (*scan == ' ' || *scan == 0)
		{
			if (numBuffer != numBufferScan) // Empty string!
			{
				ASSERT(readCount < count);
				*numBufferScan = 0;
				*bufferScan++ = (f32)atof(numBuffer);
				numBufferScan = numBuffer;
				++readCount;
			}
		}
		else
		{
			*numBufferScan++ = *scan;
		}
		if (*scan == 0)
		{
			break;
		}
	}
	ASSERT(readCount == count);
}

void ReadStringToInts(const char *text, int *buffer, int count)
{
	int *bufferScan = buffer;
	int readCount = 0;

	char numBuffer[32];
	char *numBufferScan = numBuffer;
	for (const char *scan = text; ; ++scan)
	{
		if (*scan == ' ' || *scan == 0)
		{
			if (numBuffer != numBufferScan) // Empty string!
			{
				ASSERT(readCount < count);
				*numBufferScan = 0;
				*bufferScan++ = atoi(numBuffer);
				numBufferScan = numBuffer;
				++readCount;
			}
		}
		else
		{
			*numBufferScan++ = *scan;
		}
		if (*scan == 0)
		{
			break;
		}
	}
}

ErrorCode ReadColladaGeometry(XMLElement *rootEl, const char *geometryId, RawGeometry *result)
{
	*result = {};

	XMLElement *geometryEl = rootEl->FirstChildElement("library_geometries")
		->FirstChildElement("geometry");

	if (geometryId)
	{
		bool found = false;
		while (geometryEl)
		{
			if (strcmp(geometryEl->FindAttribute("name")->Value(), geometryId) == 0)
			{
				found = true;
				break;
			}
			geometryEl = geometryEl->NextSiblingElement("geometry");
		}
		if (!found)
		{
			Log("Error! couldn't find geometry with name \"%s\"\n", geometryId);
			return ERROR_NO_ELEMENT_WITH_ID;
		}
	}
	Log("Found geometry %s\n", geometryEl->FindAttribute("name")->Value());

	XMLElement *meshEl = geometryEl->FirstChildElement("mesh");

	XMLElement *verticesSource = 0;
	XMLElement *normalsSource = 0;
	XMLElement *positionsSource = 0;
	XMLElement *uvsSource = 0;
	XMLElement *triangleIndicesSource = 0;

	const char *verticesSourceName = 0;
	const char *normalsSourceName = 0;
	const char *positionsSourceName = 0;
	const char *uvsSourceName = 0;

	result->verticesOffset = -1;
	result->normalsOffset = -1;
	result->uvsOffset = -1;
	int attrCount = 0;

	// Get triangle source ids and index array
	{
		XMLElement *trianglesEl = meshEl->FirstChildElement("triangles");
		triangleIndicesSource = trianglesEl->FirstChildElement("p");
		result->triangleCount = trianglesEl->FindAttribute("count")->IntValue();

		XMLElement *inputEl = trianglesEl->FirstChildElement("input");
		while (inputEl != nullptr)
		{
			const char *semantic = inputEl->FindAttribute("semantic")->Value();
			const char *source = inputEl->FindAttribute("source")->Value();
			int offset = inputEl->FindAttribute("offset")->IntValue();
			if (strcmp(semantic, "VERTEX") == 0)
			{
				verticesSourceName = source + 1;
				result->verticesOffset = offset;
				++attrCount;
			}
			else if (strcmp(semantic, "NORMAL") == 0)
			{
				normalsSourceName = source + 1;
				result->normalsOffset = offset;
				++attrCount;
			}
			else if (strcmp(semantic, "TEXCOORD") == 0)
			{
				uvsSourceName = source + 1;
				result->uvsOffset = offset;
				++attrCount;
			}
			inputEl = inputEl->NextSiblingElement("input");
		}
	}

	// Find vertex node
	{
		XMLElement *verticesEl = meshEl->FirstChildElement("vertices");
		while (verticesEl != nullptr)
		{
			const char *id = verticesEl->FindAttribute("id")->Value();
			if (strcmp(id, verticesSourceName) == 0)
			{
				verticesSource = verticesEl;
			}
			verticesEl = verticesEl->NextSiblingElement("vertices");
		}
	}

	// Get vertex source ids
	{
		XMLElement *inputEl = verticesSource->FirstChildElement("input");
		while (inputEl != nullptr)
		{
			const char *semantic = inputEl->FindAttribute("semantic")->Value();
			const char *source = inputEl->FindAttribute("source")->Value();
			if (strcmp(semantic, "POSITION") == 0)
			{
				positionsSourceName = source + 1;
			}
			inputEl = inputEl->NextSiblingElement("input");
		}
	}

	// Find all sources
	{
		XMLElement *sourceEl = meshEl->FirstChildElement("source");
		while (sourceEl != nullptr)
		{
			const char *id = sourceEl->FindAttribute("id")->Value();
			if (strcmp(id, positionsSourceName) == 0)
			{
				positionsSource = sourceEl;
			}
			else if (strcmp(id, normalsSourceName) == 0)
			{
				normalsSource = sourceEl;
			}
			else if (strcmp(id, uvsSourceName) == 0)
			{
				uvsSource = sourceEl;
			}
			sourceEl = sourceEl->NextSiblingElement("source");
		}
	}

	// Read positions
	{
		XMLElement *arrayEl = positionsSource->FirstChildElement("float_array");
		int positionsCount = arrayEl->FindAttribute("count")->IntValue();
		ArrayInit_v3(&result->positions, positionsCount / 3, FrameAlloc);
		ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), (f32 *)result->positions.data,
				positionsCount);
		result->positions.size = positionsCount / 3;
	}

	// Read normals
	result->normals = {};
	if (normalsSource)
	{
		XMLElement *arrayEl = normalsSource->FirstChildElement("float_array");
		int normalsCount = arrayEl->FindAttribute("count")->IntValue();
		ArrayInit_v3(&result->normals, normalsCount / 3, FrameAlloc);
		ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), (f32 *)result->normals.data, normalsCount);
		result->normals.size = normalsCount / 3;
	}

	// Read uvs
	result->uvs = {};
	if (uvsSource)
	{
		XMLElement *arrayEl = uvsSource->FirstChildElement("float_array");
		int uvsCount = arrayEl->FindAttribute("count")->IntValue();
		ArrayInit_v2(&result->uvs, uvsCount / 2, FrameAlloc);
		ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), (f32 *)result->uvs.data, uvsCount);
		result->uvs.size = uvsCount / 2;
	}

	// Read indices
	result->triangleIndices;
	{
		const int totalIndexCount = result->triangleCount * 3 * attrCount;

		ArrayInit_int(&result->triangleIndices, totalIndexCount, FrameAlloc);
		ReadStringToInts(triangleIndicesSource->FirstChild()->ToText()->Value(),
				result->triangleIndices.data, totalIndexCount);
		result->triangleIndices.size = totalIndexCount;
	}
	return ERROR_OK;
}

// WeightData is what gets injected into the vertex data, Skeleton is the controller, contains
// intrinsic joint information like bind poses
ErrorCode ReadColladaController(XMLElement *rootEl, Skeleton *skeleton, WeightData *weightData)
{
	XMLElement *controllerEl = rootEl->FirstChildElement("library_controllers")
		->FirstChildElement("controller");
	Log("Found controller %s\n", controllerEl->FindAttribute("name")->Value());
	XMLElement *skinEl = controllerEl->FirstChildElement("skin");

	const char *jointsSourceName = 0;
	const char *weightsSourceName = 0;
	XMLElement *weightCountSource = 0;
	XMLElement *weightIndicesSource = 0;
	int weightIndexCount = -1;
	{
		XMLElement *vertexWeightsEl = skinEl->FirstChildElement("vertex_weights");
		weightCountSource = vertexWeightsEl->FirstChildElement("vcount");
		weightIndicesSource = vertexWeightsEl->FirstChildElement("v");
		weightIndexCount = vertexWeightsEl->FindAttribute("count")->IntValue();

		XMLElement *inputEl = vertexWeightsEl->FirstChildElement("input");
		while (inputEl != nullptr)
		{
			const char *semantic = inputEl->FindAttribute("semantic")->Value();
			const char *source = inputEl->FindAttribute("source")->Value();
			if (strcmp(semantic, "JOINT") == 0)
			{
				jointsSourceName = source + 1;
			}
			else if (strcmp(semantic, "WEIGHT") == 0)
			{
				weightsSourceName = source + 1;
			}
			inputEl = inputEl->NextSiblingElement("input");
		}
	}

	const char *bindPosesSourceName = 0;
	{
		XMLElement *jointsEl = skinEl->FirstChildElement("joints");
		XMLElement *inputEl = jointsEl->FirstChildElement("input");
		while (inputEl != nullptr)
		{
			const char *semantic = inputEl->FindAttribute("semantic")->Value();
			const char *source = inputEl->FindAttribute("source")->Value();
			if (strcmp(semantic, "INV_BIND_MATRIX") == 0)
			{
				bindPosesSourceName = source + 1;
			}
			inputEl = inputEl->NextSiblingElement("input");
		}
	}

	XMLElement *jointsSource = 0;
	XMLElement *weightsSource = 0;
	XMLElement *bindPosesSource = 0;
	// Find all sources
	{
		XMLElement *sourceEl = skinEl->FirstChildElement("source");
		while (sourceEl != nullptr)
		{
			const char *id = sourceEl->FindAttribute("id")->Value();
			if (strcmp(id, jointsSourceName) == 0)
			{
				jointsSource = sourceEl;
			}
			else if (strcmp(id, weightsSourceName) == 0)
			{
				weightsSource = sourceEl;
			}
			else if (strcmp(id, bindPosesSourceName) == 0)
			{
				bindPosesSource = sourceEl;
			}
			sourceEl = sourceEl->NextSiblingElement("source");
		}
	}

	// Read weights
	weightData->weights = 0;
	{
		XMLElement *arrayEl = weightsSource->FirstChildElement("float_array");
		int count = arrayEl->FindAttribute("count")->IntValue();
		weightData->weights = (f32 *)FrameAlloc(sizeof(f32) * count);
		ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), weightData->weights, count);
	}

	char *jointNamesBuffer = 0;
	// Read joint names
	{
		XMLElement *arrayEl = jointsSource->FirstChildElement("Name_array");
		skeleton->jointCount = (u8)arrayEl->FindAttribute("count")->IntValue();
		skeleton->ids = (char **)FrameAlloc(sizeof(char *) * skeleton->jointCount);

		const char *in = arrayEl->FirstChild()->ToText()->Value();
		jointNamesBuffer = (char *)FrameAlloc(sizeof(char *) * strlen(in));
		strcpy(jointNamesBuffer, in);
		char *wordBegin = jointNamesBuffer;
		int jointIdx = 0;
		for (char *scan = jointNamesBuffer; ; ++scan)
		{
			if (*scan == ' ')
			{
				if (wordBegin != scan)
				{
					skeleton->ids[jointIdx++] = wordBegin;
					*scan = 0;
				}
				wordBegin = scan + 1;
			}
			else if (*scan == 0)
			{
				if (wordBegin != scan)
				{
					skeleton->ids[jointIdx++] = wordBegin;
				}
				break;
			}
		}
	}

	// Read bind poses
	{
		XMLElement *arrayEl = bindPosesSource->FirstChildElement("float_array");
		int count = arrayEl->FindAttribute("count")->IntValue();
		ASSERT(count / 16 == skeleton->jointCount);
		skeleton->bindPoses = (mat4 *)FrameAlloc(sizeof(f32) * count);
		ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), (f32 *)skeleton->bindPoses,
				count);

		for (int i = 0; i < count / 16; ++i)
		{
			skeleton->bindPoses[i] = Mat4Transpose(skeleton->bindPoses[i]);
		}
	}

	// Read counts
	weightData->weightCounts = 0;
	{
		weightData->weightCounts = (int *)FrameAlloc(sizeof(int) * weightIndexCount);
		ReadStringToInts(weightCountSource->FirstChild()->ToText()->Value(), weightData->weightCounts, weightIndexCount);
	}

	// Read weight indices
	weightData->weightIndices = 0;
	{
		int count = 0;
		for (int i = 0; i < weightIndexCount; ++i)
			count += weightData->weightCounts[i] * 2; // FIXME hardcoded 2!
		weightData->weightIndices = (int *)FrameAlloc(sizeof(int) * count);
		ReadStringToInts(weightIndicesSource->FirstChild()->ToText()->Value(), weightData->weightIndices, count);
	}

	// JOINT HIERARCHY
	{
		skeleton->jointParents = (u8 *)FrameAlloc(skeleton->jointCount);
		memset(skeleton->jointParents, 0xFFFF, skeleton->jointCount);

		skeleton->restPoses = (mat4 *)FrameAlloc(sizeof(mat4) * skeleton->jointCount);

		XMLElement *sceneEl = rootEl->FirstChildElement("library_visual_scenes")
			->FirstChildElement("visual_scene");
		//Log("Found visual scene %s\n", sceneEl->FindAttribute("id")->Value());
		XMLElement *nodeEl = sceneEl->FirstChildElement("node");

		DynamicArray_XMLElementPtr stack;
		DynamicArrayInit_XMLElementPtr(&stack, 16, FrameAlloc);

		bool checkChildren = true;
		while (nodeEl != nullptr)
		{
			const char *nodeType = nodeEl->FindAttribute("type")->Value();
			if (checkChildren && strcmp(nodeType, "JOINT") == 0)
			{
				XMLElement *child = nodeEl;
				const char *childName = child->FindAttribute("sid")->Value();

				u8 jointIdx = 0xFF;
				for (int i = 0; i < skeleton->jointCount; ++i)
				{
					if (strcmp(skeleton->ids[i], childName) == 0)
						jointIdx = (u8)i;
				}
				ASSERT(jointIdx != 0xFF);

				if (stack.size)
				{
					XMLElement *parent = stack[stack.size - 1];
					const char *parentType = parent->FindAttribute("type")->Value();
					if (strcmp(parentType, "JOINT") == 0)
					{
						// Save parent ID
						const char *parentName = parent->FindAttribute("sid")->Value();

						u8 parentJointIdx = 0xFF;
						for (int i = 0; i < skeleton->jointCount; ++i)
						{
							if (strcmp(skeleton->ids[i], parentName) == 0)
								parentJointIdx = (u8)i;
							else if (strcmp(skeleton->ids[i], childName) == 0)
								jointIdx = (u8)i;
						}
						ASSERT(parentJointIdx != 0xFF);

						skeleton->jointParents[jointIdx] = parentJointIdx;

						//Log("Joint %s has parent %s\n", childName, parentName);
					}
				}

				XMLElement *matrixEl = nodeEl->FirstChildElement("matrix");
				const char *matrixStr = matrixEl->FirstChild()->ToText()->Value();
				mat4 restPose;
				ReadStringToFloats(matrixStr, (f32 *)&restPose, 16);
				skeleton->restPoses[jointIdx] = Mat4Transpose(restPose);
			}

			if (checkChildren)
			{
				XMLElement *childEl = nodeEl->FirstChildElement("node");
				if (childEl)
				{
					const char *type = nodeEl->FindAttribute("type")->Value();
					if (strcmp(type, "JOINT") == 0)
					{
						// Push joint to stack!
						stack[DynamicArrayAdd_XMLElementPtr(&stack, FrameRealloc)] = nodeEl;
					}

					nodeEl = childEl;
					continue;
				}
			}

			XMLElement *siblingEl = nodeEl->NextSiblingElement("node");
			if (siblingEl)
			{
				nodeEl = siblingEl;
				checkChildren = true;
				continue;
			}

			// Pop from stack!
			if (stack.size == 0)
				break;

			nodeEl = stack[--stack.size];
			checkChildren = false;
		}
	}

	return ERROR_OK;
}

int ConstructSkinnedMesh(const RawGeometry *geometry, const WeightData *weightData,
		DynamicArray_RawVertex &finalVertices, Array_u16 &finalIndices)
{
	void *oldStackPtr = g_gameMemory->stackPtr;

	const u32 vertexTotal = geometry->positions.size;

	Array_SkinnedPosition skinnedPositions;
	ArrayInit_SkinnedPosition(&skinnedPositions, vertexTotal, StackAlloc);
	for (u32 i = 0; i < vertexTotal; ++i)
	{
		SkinnedPosition *skinnedPos = &skinnedPositions[skinnedPositions.size++];
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
		void *oldStackPtrJoin = g_gameMemory->stackPtr;

		const bool hasUvs = geometry->uvsOffset >= 0;
		const bool hasNormals = geometry->normalsOffset >= 0;
		const int attrCount = 1 + hasUvs + hasNormals;

		ArrayInit_u16(&finalIndices, geometry->triangleCount * 3, FrameAlloc);

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
				u16 newVertexIdx = (u16)DynamicArrayAdd_RawVertex(&finalVertices, FrameRealloc);
				finalVertices[newVertexIdx] = newVertex;
				finalIndices[finalIndices.size++] = newVertexIdx;

				// Add to map
				bucket[DynamicArrayAdd_u16(&bucket, StackRealloc)] = newVertexIdx;
			}
			else
			{
				finalIndices[finalIndices.size++] = index;
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

	return 0;
}

ErrorCode ReadColladaAnimation(XMLElement *rootEl, Animation &animation,
		Skeleton &skeleton)
{
	DynamicArray_AnimationChannel channels;
	DynamicArrayInit_AnimationChannel(&channels, skeleton.jointCount, FrameAlloc);

	XMLElement *animationEl = rootEl->FirstChildElement("library_animations")
		->FirstChildElement("animation");
	XMLElement *subAnimEl = animationEl->FirstChildElement("animation");
	while (subAnimEl != nullptr)
	{
		//Log("Found animation %s\n", subAnimEl->FindAttribute("id")->Value());

		XMLElement *channelEl = subAnimEl->FirstChildElement("channel");
		const char *samplerSourceName = channelEl->FindAttribute("source")->Value() + 1;

		XMLElement *samplerSource = 0;
		// Find all samplers
		{
			XMLElement *samplerEl = subAnimEl->FirstChildElement("sampler");
			while (samplerEl != nullptr)
			{
				const char *id = samplerEl->FindAttribute("id")->Value();
				if (strcmp(id, samplerSourceName) == 0)
				{
					samplerSource = samplerEl;
				}
				samplerEl = samplerEl->NextSiblingElement("sampler");
			}
		}

		const char *timestampsSourceName = 0;
		const char *matricesSourceName = 0;
		{
			XMLElement *inputEl = samplerSource->FirstChildElement("input");
			while (inputEl != nullptr)
			{
				const char *semantic = inputEl->FindAttribute("semantic")->Value();
				const char *source = inputEl->FindAttribute("source")->Value();
				if (strcmp(semantic, "INPUT") == 0)
				{
					timestampsSourceName = source + 1;
				}
				else if (strcmp(semantic, "OUTPUT") == 0)
				{
					matricesSourceName = source + 1;
				}
				inputEl = inputEl->NextSiblingElement("input");
			}
		}

		XMLElement *timestampsSource = 0;
		XMLElement *matricesSource = 0;
		// Find all sources
		{
			XMLElement *sourceEl = subAnimEl->FirstChildElement("source");
			while (sourceEl != nullptr)
			{
				const char *id = sourceEl->FindAttribute("id")->Value();
				if (strcmp(id, timestampsSourceName) == 0)
				{
					timestampsSource = sourceEl;
				}
				else if (strcmp(id, matricesSourceName) == 0)
				{
					matricesSource = sourceEl;
				}
				sourceEl = sourceEl->NextSiblingElement("source");
			}
		}

		// Read timestamps
		if (animation.timestamps == nullptr)
		{
			// We assume an equal amount of samples per channel, so we only save timestamps
			// once per animation
			XMLElement *arrayEl = timestampsSource->FirstChildElement("float_array");
			animation.frameCount = arrayEl->FindAttribute("count")->IntValue();
			animation.timestamps = (f32 *)FrameAlloc(sizeof(f32) * animation.frameCount);
			ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), animation.timestamps,
					animation.frameCount);
		}

		AnimationChannel channel = {};

		// Read matrices
		{
			XMLElement *arrayEl = matricesSource->FirstChildElement("float_array");
			int count = arrayEl->FindAttribute("count")->IntValue();
			ASSERT(animation.frameCount == (u32)(count / 16));
			channel.transforms = (mat4 *)FrameAlloc(sizeof(f32) * count);
			ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), (f32 *)channel.transforms,
					count);

			for (int i = 0; i < count / 16; ++i)
			{
				channel.transforms[i] = Mat4Transpose(channel.transforms[i]);
			}
		}

		// Get joint id
		{
			// TODO use scene to properly get the target joint
			const char *targetName = channelEl->FindAttribute("target")->Value();
			char jointIdBuffer[64];
			for (const char *scan = targetName; ; ++scan)
			{
				if (*scan == '_')
				{
					strcpy(jointIdBuffer, scan + 1);
					break;
				}
			}
			for (char *scan = jointIdBuffer; ; ++scan)
			{
				if (*scan == '/')
				{
					*scan = 0;
					break;
				}
			}
			channel.jointIndex = 0xFF;
			for (int i = 0; i < skeleton.jointCount; ++i)
			{
				if (strcmp(skeleton.ids[i], jointIdBuffer) == 0)
				{
					ASSERT(i < 0xFF);
					channel.jointIndex = (u8)i;
					break;
				}
			}
			ASSERT(channel.jointIndex != 0xFF);
			//Log("Channel thought to target joint \"%s\", ID %d\n", jointIdBuffer,
					//channel.jointIndex);
		}

		// Move start to 0
		f32 oldStart = animation.timestamps[0];
		for (u32 i = 0; i < animation.frameCount; ++i)
		{
			animation.timestamps[i] -= oldStart;
		}

		int newChannelIdx = DynamicArrayAdd_AnimationChannel(&channels, FrameRealloc);
		channels[newChannelIdx] = channel;

		subAnimEl = subAnimEl->NextSiblingElement("animation");
	}
	animation.channels = channels.data;
	animation.channelCount = channels.size;

	return ERROR_OK;
}

int OutputMesh(const char *filename, DynamicArray_RawVertex &finalVertices,
		Array_u16 &finalIndices)
{
	// Output!
	{
		HANDLE newFile = OpenForWrite(filename);
		u64 filePos = 0;
		{
			u64 vertexBlobSize = sizeof(Vertex) * finalVertices.size;

			BakeryMeshHeader header;
			header.vertexCount = finalVertices.size;
			header.indexCount = finalIndices.size;
			header.vertexBlobOffset = sizeof(header);
			header.indexBlobOffset = header.vertexBlobOffset + vertexBlobSize;

			filePos += WriteToFile(newFile, &header, sizeof(header));

			ASSERT(filePos == header.vertexBlobOffset);

			// Write vertex data
			for (u32 i = 0; i < finalVertices.size; ++i)
			{
				RawVertex *rawVertex = &finalVertices[i];
				Vertex gameVertex;
				gameVertex.pos = rawVertex->skinnedPos.pos;
				gameVertex.uv = rawVertex->uv;
				gameVertex.nor = rawVertex->normal;

				filePos += WriteToFile(newFile, &gameVertex, sizeof(gameVertex));
			}

			ASSERT(filePos == header.indexBlobOffset);

			// Write indices
			for (u32 i = 0; i < finalIndices.size; ++i)
			{
				u16 outputIdx = finalIndices[i];
				filePos += WriteToFile(newFile, &outputIdx, sizeof(outputIdx));
			}
		}
		CloseHandle(newFile);
	}

	return 0;
}

int OutputSkinnedMesh(const char *filename,
		DynamicArray_RawVertex &finalVertices, Array_u16 &finalIndices, Skeleton &skeleton,
		DynamicArray_Animation &animations)
{
	void *oldStackPtr = g_gameMemory->stackPtr;

	HANDLE newFile = OpenForWrite(filename);
	
	BakerySkinnedMeshHeader header;
	header.vertexCount = finalVertices.size;
	header.indexCount = finalIndices.size;
	header.jointCount = skeleton.jointCount;
	header.animationCount = animations.size;
	FileSeek(newFile, sizeof(header), FILE_BEGIN);

	// Write vertex data
	header.vertexBlobOffset = FilePosition(newFile);
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

		WriteToFile(newFile, &gameVertex, sizeof(gameVertex));
	}

	FileAlign(newFile);

	// Write indices
	header.indexBlobOffset = FilePosition(newFile);
	for (u32 i = 0; i < finalIndices.size; ++i)
	{
		u16 outputIdx = finalIndices[i];
		WriteToFile(newFile, &outputIdx, sizeof(outputIdx));
	}

	FileAlign(newFile);

	// Write skeleton
	header.bindPosesBlobOffset = FilePosition(newFile);
	WriteToFile(newFile, skeleton.bindPoses, sizeof(mat4) * skeleton.jointCount);
	FileAlign(newFile);

	header.jointParentsBlobOffset = FilePosition(newFile);
	WriteToFile(newFile, skeleton.jointParents, skeleton.jointCount);
	FileAlign(newFile);

	header.restPosesBlobOffset = FilePosition(newFile);
	WriteToFile(newFile, skeleton.restPoses, sizeof(mat4) * skeleton.jointCount);
	FileAlign(newFile);

	// Write animations
	Array_BakerySkinnedMeshAnimationHeader animationHeaders;
	ArrayInit_BakerySkinnedMeshAnimationHeader(&animationHeaders, animations.size, StackAlloc);
	for (u32 animIdx = 0; animIdx < animations.size; ++animIdx)
	{
		Animation *animation = &animations[animIdx];

		BakerySkinnedMeshAnimationHeader *animationHeader = &animationHeaders[animIdx];
		animationHeader->frameCount = animation->frameCount;
		animationHeader->channelCount = animation->channelCount;

		animationHeader->timestampsBlobOffset = FilePosition(newFile);
		WriteToFile(newFile, animation->timestamps, sizeof(u32) * animation->frameCount);

		Array_BakerySkinnedMeshAnimationChannelHeader channelHeaders;
		ArrayInit_BakerySkinnedMeshAnimationChannelHeader(&channelHeaders, animation->channelCount,
				StackAlloc);
		for (u32 channelIdx = 0; channelIdx < animation->channelCount; ++channelIdx)
		{
			AnimationChannel *channel = &animation->channels[channelIdx];

			BakerySkinnedMeshAnimationChannelHeader *channelHeader = &channelHeaders[channelIdx];
			channelHeader->jointIndex = channel->jointIndex;
			channelHeader->transformsBlobOffset = FilePosition(newFile);
			WriteToFile(newFile, channel->transforms, sizeof(mat4) * animation->frameCount);
		}

		animationHeader->channelsBlobOffset = FilePosition(newFile);
		for (u32 channelIdx = 0; channelIdx < animation->channelCount; ++channelIdx)
		{
			BakerySkinnedMeshAnimationChannelHeader *channelHeader = &channelHeaders[channelIdx];
			WriteToFile(newFile, channelHeader, sizeof(*channelHeader));
		}
	}

	header.animationBlobOffset = FilePosition(newFile);
	for (u32 animIdx = 0; animIdx < animations.size; ++animIdx)
	{
		BakerySkinnedMeshAnimationHeader *animationHeader = &animationHeaders[animIdx];
		WriteToFile(newFile, animationHeader, sizeof(*animationHeader));
	}

	FileSeek(newFile, 0, FILE_BEGIN);
	WriteToFile(newFile, &header, sizeof(header));

	CloseHandle(newFile);

	StackFree(oldStackPtr);

	return 0;
}

bool DidMetaDependenciesChange(const char *metaFilename, const char *fullDataDir, Array_FileCacheEntry &cache)
{
	XMLError xmlError;

	tinyxml2::XMLDocument doc;
	xmlError = doc.LoadFile(metaFilename);
	if (xmlError != XML_SUCCESS)
	{
		Log("ERROR! Parsing XML file \"%s\" (%s)\n", metaFilename, doc.ErrorStr());
		return true;
	}

	XMLElement *rootEl = doc.FirstChildElement("meta");
	if (!rootEl)
	{
		Log("ERROR! Meta root node not found\n");
		return true;
	}

	bool somethingChanged = false;
	XMLElement *fileEl = rootEl->FirstChildElement();
	while (fileEl)
	{
		const char *filename = fileEl->FirstChild()->ToText()->Value();
		char fullName[MAX_PATH];
		sprintf(fullName, "%s%s", fullDataDir, filename);

		if (Win32DidFileChange(fullName, cache))
		{
			somethingChanged = true;
		}

		fileEl = fileEl->NextSiblingElement();
	}
	return somethingChanged;
}

void GenerateQuadTree(Array_Triangle &triangles, QuadTree *quadTree)
{
	void *oldStackPtr = g_gameMemory->stackPtr;

	v2 lowCorner = { INFINITY, INFINITY };
	v2 highCorner = { -INFINITY, -INFINITY };

	// Get limits
	for (u32 i = 0; i < triangles.size; ++i)
	{
		for (u32 j = 0; j < 3; ++j)
		{
			v3 p = triangles[i].corners[j];
			if (p.x < lowCorner.x) lowCorner.x = p.x;
			if (p.x > highCorner.x) highCorner.x = p.x;
			if (p.y < lowCorner.y) lowCorner.y = p.y;
			if (p.y > highCorner.y) highCorner.y = p.y;
		}
	}

	const int cellsSide = 32;
	const int cellCount = cellsSide * cellsSide;
	const v2 span = (highCorner - lowCorner);
	const v2 cellSize = span / (f32)(cellsSide - 1); // -1 because we are rounding
	// We are using 0, 0 as the CENTER of a cell, otherwise the high border ends up ouside the grid

	Log("Quad tree limits: { %.02f, %.02f } - { %.02f, %.02f }\n", lowCorner.x, lowCorner.y,
			highCorner.x, highCorner.y);

	// Init buckets
	DynamicArray_Triangle *cellBuckets = (DynamicArray_Triangle *)StackAlloc(sizeof(DynamicArray_Triangle) * cellCount);
	for (int i = 0; i < cellCount; ++i)
	{
		DynamicArrayInit_Triangle(&cellBuckets[i], 256, StackAlloc);
	}
	u32 totalTriangleCount = 0;

	for (u32 triangleIdx = 0; triangleIdx < triangles.size; ++triangleIdx)
	{
		Triangle *curTriangle = &triangles[triangleIdx];

		v2 corners[3];
		for (int i = 0; i < 3; ++i)
		{
			v2 p = {
				(curTriangle->corners[i].x - lowCorner.x) / cellSize.x,
				(curTriangle->corners[i].y - lowCorner.y) / cellSize.y
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

		// We split the triangle into a flat-bottomed triangle and a flat-topped one, and rasterize
		// them individually

		// If the whole triangle fits in a single row, just fill from left to right
		if (Round(corners[1].y) == Round(corners[2].y))
		{
			int scanlineY = (int)Round(corners[1].y);
			f32 left = Min(corners[0].x, Min(corners[1].x, corners[2].x));
			f32 right = Max(corners[0].x, Max(corners[1].x, corners[2].x));
			for (int scanX = (int)Round(left);
				scanX <= (int)Round(right);
				++scanX)
			{
				ASSERT(scanX >= 0 && scanX < cellsSide);
				DynamicArray_Triangle &bucket = cellBuckets[scanX + scanlineY * cellsSide];
				bucket[DynamicArrayAdd_Triangle(&bucket, StackRealloc)] = *curTriangle;
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

			int bottom = (int)Round(corners[1].y);
			int top = (int)Round(corners[2].y);

			f32 curX1 = corners[2].x;
			f32 curX2 = corners[2].x;
			// The top point can be anywhere between the top of the row and the bottom, so advance
			// until we meet the bottom border of the row
			f32 advTillNextGridLine = Fmod(corners[2].y + 0.5f, 1.0f);
			f32 nextX1 = curX1 - invSlope1 * advTillNextGridLine;
			f32 nextX2 = curX2 - invSlope2 * advTillNextGridLine;
			ASSERT(bottom >= 0 && top < cellsSide);
			for (int scanlineY = top; scanlineY >= bottom; --scanlineY)
			{
				for (int scanX = (int)Round(Min(curX1, nextX1));
					scanX <= (int)Round(Max(curX2, nextX2));
					++scanX)
				{
					if (scanX >= 0 && scanX < cellsSide)
					{
						DynamicArray_Triangle &bucket = cellBuckets[scanX + scanlineY * cellsSide];
						bucket[DynamicArrayAdd_Triangle(&bucket, StackRealloc)] = *curTriangle;
						++totalTriangleCount;
					}
					else __debugbreak();
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
					const f32 remain = 1.0f - Fmod(corners[1].y + 0.5f, 1.0f);
					nextX1 -= invSlope1 * remain;
					nextX2 -= invSlope2 * remain;
				}
			}
		}
		// Rasterize bottom triangle
		// Pretty much the same as above but upside down
		if (Round(corners[0].y) != Round(corners[1].y))
		{
			f32 invSlope1 = (f32)(corners[1].x - corners[0].x) / (f32)(corners[1].y - corners[0].y);
			f32 invSlope2 = (f32)(corners[2].x - corners[0].x) / (f32)(corners[2].y - corners[0].y);
			if (invSlope1 > invSlope2)
			{
				f32 tmp = invSlope1;
				invSlope1 = invSlope2;
				invSlope2 = tmp;
			}

			int bottom = (int)Round(corners[0].y);
			int top = (int)Round(corners[1].y);

			f32 curX1 = corners[0].x;
			f32 curX2 = corners[0].x;
			f32 advTillNextGridLine = 1.0f - Fmod(corners[0].y + 0.5f, 1.0f);
			f32 nextX1 = curX1 + invSlope1 * advTillNextGridLine;
			f32 nextX2 = curX2 + invSlope2 * advTillNextGridLine;
			ASSERT(bottom >= 0 && top < cellsSide);
			for (int scanlineY = bottom; scanlineY < top; ++scanlineY)
			{
				for (int scanX = (int)Round(Min(curX1, nextX1));
					scanX <= (int)Round(Max(curX2, nextX2));
					++scanX)
				{
					if (scanX >= 0 && scanX < cellsSide)
					{
						DynamicArray_Triangle &bucket = cellBuckets[scanX + scanlineY * cellsSide];
						bucket[DynamicArrayAdd_Triangle(&bucket, StackRealloc)] = *curTriangle;
						++totalTriangleCount;
					}
				}
				curX1 = nextX1;
				curX2 = nextX2;
				nextX1 += invSlope1;
				nextX2 += invSlope2;
			}
		}
	}

	quadTree->lowCorner = lowCorner;
	quadTree->highCorner = highCorner;
	quadTree->cellsSide = cellsSide;
	quadTree->offsets = (u32 *)FrameAlloc(sizeof(u32) * (cellCount - 1));
	quadTree->triangles = (Triangle *)FrameAlloc(sizeof(Triangle) * totalTriangleCount);

	int triangleCount = 0;
	for (int y = 0; y < cellsSide; ++y)
	{
		for (int x = 0; x < cellsSide; ++x)
		{
			int i = x + y * cellsSide;
			ASSERT(triangleCount < U32_MAX);
			quadTree->offsets[i] = (u32)triangleCount;

			DynamicArray_Triangle &bucket = cellBuckets[i];
			for (u32 bucketIdx = 0; bucketIdx < bucket.size; ++bucketIdx)
			{
				quadTree->triangles[triangleCount++] = bucket[bucketIdx];
			}
		}
	}
	ASSERT(totalTriangleCount == (u32)triangleCount);
	quadTree->offsets[cellCount] = (u32)triangleCount;

	StackFree(oldStackPtr);
}

int ReadMeta(const char *filename, const char *fullDataDir)
{
	XMLError xmlError;

	tinyxml2::XMLDocument doc;
	xmlError = doc.LoadFile(filename);
	if (xmlError != XML_SUCCESS)
	{
		Log("ERROR! Parsing XML file \"%s\" (%s)\n", filename, doc.ErrorStr());
		return xmlError;
	}

	XMLElement *rootEl = doc.FirstChildElement("meta");
	if (!rootEl)
	{
		Log("ERROR! Meta root node not found\n");
		return ERROR_META_NOROOT;
	}

	DynamicArray_RawVertex finalVertices = {};
	Array_u16 finalIndices = {};
	Skeleton skeleton = {};
	DynamicArray_Animation animations = {};

	DynamicArrayInit_Animation(&animations, 16, FrameAlloc);

	const char *typeStr = rootEl->FindAttribute("type")->Value();
	Log("Meta type: %s\n", typeStr);

	MetaType type = METATYPE_INVALID;
	for (int i = 0; i < METATYPE_COUNT; ++i)
	{
		if (strcmp(typeStr, MetaTypeNames[i]) == 0)
		{
			type = (MetaType)i;
		}
	}
	if (type == METATYPE_INVALID)
	{
		Log("ERROR! Invalid meta type\n");
		return ERROR_META_WRONG_TYPE;
	}

	switch(type)
	{
	case METATYPE_MESH:
	{
		XMLElement *geometryEl = rootEl->FirstChildElement("geometry");
		while (geometryEl != nullptr)
		{
			const char *geomFile = geometryEl->FirstChild()->ToText()->Value();

			const char *geomName = 0;
			const XMLAttribute *nameAttr = geometryEl->FindAttribute("id");
			if (nameAttr)
			{
				geomName = nameAttr->Value();
			}

			char fullname[MAX_PATH];
			sprintf(fullname, "%s%s", fullDataDir, geomFile);

			tinyxml2::XMLDocument dataDoc;
			xmlError = dataDoc.LoadFile(fullname);
			if (xmlError != XML_SUCCESS)
			{
				Log("ERROR! Parsing XML file \"%s\" (%s)\n", fullname, dataDoc.ErrorStr());
				return xmlError;
			}

			XMLElement *dataRootEl = dataDoc.FirstChildElement("COLLADA");
			if (!dataRootEl)
			{
				Log("ERROR! Collada root node not found\n");
				return ERROR_COLLADA_NOROOT;
			}

			RawGeometry rawGeometry;
			ErrorCode error = ReadColladaGeometry(dataRootEl, geomName, &rawGeometry);
			if (error != ERROR_OK)
				return error;

			ConstructSkinnedMesh(&rawGeometry, nullptr, finalVertices,
					finalIndices);

			geometryEl = geometryEl->NextSiblingElement("geometry");
		}

		// Output
		{
			char outputName[MAX_PATH];
			GetOutputFilename(filename, outputName);

			int error = OutputMesh(outputName, finalVertices, finalIndices);
			if (error)
				return error;
		}
	} break;
	case METATYPE_SKINNED_MESH:
	{
		XMLElement *geometryEl = rootEl->FirstChildElement("geometry");
		while (geometryEl != nullptr)
		{
			const char *geomFile = geometryEl->FirstChild()->ToText()->Value();
			Log("Found geometry file: %s\n", geomFile);

			const char *geomName = 0;
			const XMLAttribute *nameAttr = geometryEl->FindAttribute("id");
			if (nameAttr)
			{
				geomName = nameAttr->Value();
			}

			char fullname[MAX_PATH];
			sprintf(fullname, "%s%s", fullDataDir, geomFile);

			tinyxml2::XMLDocument dataDoc;
			xmlError = dataDoc.LoadFile(fullname);
			if (xmlError != XML_SUCCESS)
			{
				Log("ERROR! Parsing XML file \"%s\" (%s)\n", fullname, dataDoc.ErrorStr());
				return xmlError;
			}

			XMLElement *dataRootEl = dataDoc.FirstChildElement("COLLADA");
			if (!dataRootEl)
			{
				Log("ERROR! Collada root node not found\n");
				return ERROR_COLLADA_NOROOT;
			}

			RawGeometry rawGeometry;
			int error = ReadColladaGeometry(dataRootEl, geomName, &rawGeometry);
			if (error)
				return error;

			WeightData weightData;
			error = ReadColladaController(dataRootEl, &skeleton, &weightData);
			if (error)
				return error;

			ConstructSkinnedMesh(&rawGeometry, &weightData, finalVertices,
					finalIndices);

			geometryEl = geometryEl->NextSiblingElement("geometry");
		}

		XMLElement *animationEl = rootEl->FirstChildElement("animation");
		while (animationEl != nullptr)
		{
			const char *animFile = animationEl->FirstChild()->ToText()->Value();
			Log("Found animation file: %s\n", animFile);

			char fullname[MAX_PATH];
			sprintf(fullname, "%s%s", fullDataDir, animFile);

			tinyxml2::XMLDocument dataDoc;
			xmlError = dataDoc.LoadFile(fullname);
			if (xmlError != XML_SUCCESS)
			{
				Log("ERROR! Parsing XML file \"%s\" (%s)\n", fullname, dataDoc.ErrorStr());
				return xmlError;
			}

			XMLElement *dataRootEl = dataDoc.FirstChildElement("COLLADA");
			if (!dataRootEl)
			{
				Log("ERROR! Collada root node not found\n");
				return ERROR_COLLADA_NOROOT;
			}

			Animation &animation = animations[DynamicArrayAdd_Animation(&animations, FrameRealloc)];
			animation = {};
			ErrorCode error = ReadColladaAnimation(dataRootEl, animation, skeleton);
			if (error)
				return error;

			animationEl = animationEl->NextSiblingElement("animation");
		}

		// Output
		{
			char outputName[MAX_PATH];
			GetOutputFilename(filename, outputName);

			int error = OutputSkinnedMesh(outputName, finalVertices, finalIndices,
					skeleton, animations);
			if (error)
				return error;
		}
	} break;
	case METATYPE_TRIANGLE_DATA:
	{
		XMLElement *geometryEl = rootEl->FirstChildElement("geometry");

		const char *geomFile = geometryEl->FirstChild()->ToText()->Value();
		Log("Found geometry file: %s\n", geomFile);

		const char *geomName = 0;
		const XMLAttribute *nameAttr = geometryEl->FindAttribute("id");
		if (nameAttr)
		{
			geomName = nameAttr->Value();
		}

		char fullname[MAX_PATH];
		sprintf(fullname, "%s%s", fullDataDir, geomFile);

		tinyxml2::XMLDocument dataDoc;
		xmlError = dataDoc.LoadFile(fullname);
		if (xmlError != XML_SUCCESS)
		{
			Log("ERROR! Parsing XML file \"%s\" (%s)\n", fullname, dataDoc.ErrorStr());
			return xmlError;
		}

		XMLElement *dataRootEl = dataDoc.FirstChildElement("COLLADA");
		if (!dataRootEl)
		{
			Log("ERROR! Collada root node not found\n");
			return ERROR_COLLADA_NOROOT;
		}

		RawGeometry rawGeometry;
		int error = ReadColladaGeometry(dataRootEl, geomName, &rawGeometry);
		if (error)
			return error;

		// Make triangles
		Array_Triangle triangles;
		ArrayInit_Triangle(&triangles, rawGeometry.triangleCount, FrameAlloc);

		int attrCount = 1;
		if (rawGeometry.normalsOffset >= 0)
			++attrCount;
		if (rawGeometry.uvsOffset >= 0)
			++attrCount;

		for (int i = 0; i < rawGeometry.triangleCount; ++i)
		{
			Triangle *triangle = &triangles[triangles.size++];

			int ai = rawGeometry.triangleIndices[(i * 3 + 0) * attrCount + rawGeometry.verticesOffset];
			int bi = rawGeometry.triangleIndices[(i * 3 + 2) * attrCount + rawGeometry.verticesOffset];
			int ci = rawGeometry.triangleIndices[(i * 3 + 1) * attrCount + rawGeometry.verticesOffset];

			triangle->a = rawGeometry.positions[ai];
			triangle->b = rawGeometry.positions[bi];
			triangle->c = rawGeometry.positions[ci];

			triangle->normal = V3Normalize(V3Cross(triangle->c - triangle->a, triangle->b - triangle->a));
		}

		QuadTree quadTree;
		GenerateQuadTree(triangles, &quadTree);

		// Output
		char outputName[MAX_PATH];
		GetOutputFilename(filename, outputName);

		HANDLE file = OpenForWrite(outputName);

		BakeryTriangleDataHeader header;
		u64 offsetsBlobOffset = FileSeek(file, sizeof(header), FILE_BEGIN);

		u32 offsetCount = quadTree.cellsSide * quadTree.cellsSide + 1;
		WriteToFile(file, quadTree.offsets, sizeof(*quadTree.offsets) * offsetCount);

		u64 trianglesBlobOffset = FilePosition(file);
		u32 triangleCount = quadTree.offsets[offsetCount - 1];
		WriteToFile(file, quadTree.triangles, sizeof(Triangle) * triangleCount);

		FileSeek(file, 0, FILE_BEGIN);
		header.lowCorner = quadTree.lowCorner;
		header.highCorner = quadTree.highCorner;
		header.cellsSide = quadTree.cellsSide;
		header.offsetsBlobOffset = offsetsBlobOffset;
		header.trianglesBlobOffset = trianglesBlobOffset;
		WriteToFile(file, &header, sizeof(header));

		CloseHandle(file);
	} break;
	case METATYPE_POINTS:
	{
		XMLElement *geometryEl = rootEl->FirstChildElement("geometry");

		const char *geomFile = geometryEl->FirstChild()->ToText()->Value();
		Log("Found geometry file: %s\n", geomFile);

		const char *geomName = 0;
		const XMLAttribute *nameAttr = geometryEl->FindAttribute("id");
		if (nameAttr)
		{
			geomName = nameAttr->Value();
		}

		char fullname[MAX_PATH];
		sprintf(fullname, "%s%s", fullDataDir, geomFile);

		tinyxml2::XMLDocument dataDoc;
		xmlError = dataDoc.LoadFile(fullname);
		if (xmlError != XML_SUCCESS)
		{
			Log("ERROR! Parsing XML file \"%s\" (%s)\n", fullname, dataDoc.ErrorStr());
			return xmlError;
		}

		XMLElement *dataRootEl = dataDoc.FirstChildElement("COLLADA");
		if (!dataRootEl)
		{
			Log("ERROR! Collada root node not found\n");
			return ERROR_COLLADA_NOROOT;
		}

		RawGeometry rawGeometry;
		ErrorCode error = ReadColladaGeometry(dataRootEl, geomName, &rawGeometry);
		if (error)
			return error;

		// Output
		char outputName[MAX_PATH];
		GetOutputFilename(filename, outputName);

		HANDLE file = OpenForWrite(outputName);

		const u32 pointCount = rawGeometry.positions.size;

		BakeryPointsHeader header;
		header.pointCount = pointCount;
		header.pointsBlobOffset = sizeof(header);

		WriteToFile(file, &header, sizeof(header));
		WriteToFile(file, rawGeometry.positions.data, sizeof(v3) * pointCount);

		CloseHandle(file);
	} break;
	};

	return 0;
}

int main(int argc, char **argv)
{
	(void) argc, argv;

	AttachConsole(ATTACH_PARENT_PROCESS);
	g_hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	GameMemory gameMemory;
	g_gameMemory = &gameMemory;

	gameMemory.stackMem = malloc(stackSize);
	gameMemory.stackPtr = gameMemory.stackMem;
	gameMemory.frameMem = malloc(frameSize);
	gameMemory.framePtr = gameMemory.frameMem;
	gameMemory.transientMem = malloc(transientSize);
	gameMemory.transientPtr = gameMemory.transientMem;

	Array_FileCacheEntry cache;
	ArrayInit_FileCacheEntry(&cache, 1024, TransientAlloc);

	int error = 0;

	char fullDataDir[MAX_PATH];
	GetDataPath(fullDataDir);

	char metaWildcard[MAX_PATH];
	sprintf(metaWildcard, "%s*.meta", fullDataDir);
	WIN32_FIND_DATA findData;
	char fullName[MAX_PATH];
	while (1)
	{
		HANDLE searchHandle = FindFirstFileA(metaWildcard, &findData);
		while (1)
		{
			sprintf(fullName, "%s%s", fullDataDir, findData.cFileName);

			bool changed = Win32DidFileChange(fullName, cache);

			if (!changed)
			{
				changed = DidMetaDependenciesChange(fullName, fullDataDir, cache);
			}

			if (changed)
			{
				Log("-- Detected file %s ---------------------\n", findData.cFileName);
				error = ReadMeta(fullName, fullDataDir);
				Log("\n");

				//Log("Used %.02fkb of frame allocator\n", ((u8 *)framePtr - (u8 *)frameMem) / 1024.0f);
				FrameWipe();

				if (error != 0)
					goto done;
			}

			if (!FindNextFileA(searchHandle, &findData))
				break;
		}

		// Process dirty cache entries
		for (u32 i = 0; i < cache.size; ++i)
		{
			FileCacheEntry *cacheEntry = &cache[i];
			cacheEntry->changed = false;
		}

#if 1
		Sleep(500);
#else
		break;
#endif
	}

done:
	free(gameMemory.stackMem);
	free(gameMemory.frameMem);

	return error;
}

#undef FilePosition
#undef FileAlign
