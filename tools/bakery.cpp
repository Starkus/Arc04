#include <windows.h>
#include <stdio.h>
#include <SDL.h>

#include "tinyxml2.cpp"
using namespace tinyxml2;

#include "General.h"
#include "Maths.h"
#include "Containers.h"
#include "Geometry.h"

const char *dataDir = "../data/";

enum ErrorCode
{
	ERROR_OK,
	ERROR_META_NOROOT = XML_ERROR_COUNT,
	ERROR_COLLADA_NOROOT,
	ERROR_META_WRONG_TYPE
};

enum MetaType
{
	METATYPE_INVALID = -1,
	METATYPE_MESH,
	METATYPE_SKINNED_MESH,
	METATYPE_TRIANGLE_DATA,
	METATYPE_POINTS,
	METATYPE_COUNT
};

const char *MetaTypeNames[] =
{
	"MESH",
	"SKINNED_MESH",
	"TRIANGLE_DATA",
	"POINTS"
};

struct SkinnedPosition
{
	v3 pos;
	u8 jointIndices[4];
	f32 jointWeights[4];
};

struct RawVertex
{
	SkinnedPosition skinnedPos;
	v2 uv;
	v3 normal;
};

struct Skeleton
{
	u8 jointCount;
	char **ids;
	mat4 *bindPoses;
	u8 *jointParents;
	mat4 *restPoses;
};

struct AnimationChannel
{
	u8 jointIndex;
	mat4 *transforms;
};

struct Animation
{
	u32 frameCount;
	f32 *timestamps;
	u32 channelCount;
	AnimationChannel *channels;
};

typedef XMLElement* XMLElementPtr;

// STACK
const u64 stackSize = 128 * 1024 * 1024;
void *stackMem;
void *stackPtr;
void StackInit()
{
	stackMem = malloc(stackSize);
	stackPtr = stackMem;
}
void StackFinalize()
{
	free(stackMem);
}
void *StackAlloc(u64 size)
{
	ASSERT((u8 *)stackPtr + size < (u8 *)stackMem + stackSize); // Out of memory!
	void *result;

	result = stackPtr;
	stackPtr = (u8 *)stackPtr + size;

	return result;
}
void *StackRealloc(void *ptr, u64 newSize)
{
	//ASSERT(false);
	//SDL_Log("WARNING: STACK REALLOC\n");

	void *newBlock = StackAlloc(newSize);
	memcpy(newBlock, ptr, newSize);
	return newBlock;
}
void StackFree(void *ptr)
{
	stackPtr = ptr;
}

DECLARE_ARRAY(u16);
DECLARE_ARRAY(int);
DECLARE_ARRAY(f32);
DECLARE_ARRAY(v3);
DECLARE_ARRAY(v2);
DECLARE_ARRAY(mat4);
DECLARE_ARRAY(SkinnedPosition);
DECLARE_DYNAMIC_ARRAY(u16);
DECLARE_DYNAMIC_ARRAY(XMLElementPtr);
DECLARE_DYNAMIC_ARRAY(RawVertex);
DECLARE_DYNAMIC_ARRAY(AnimationChannel);
DECLARE_DYNAMIC_ARRAY(Animation);

struct RawGeometry
{
	Array_v3 positions;
	Array_v2 uvs;
	Array_v3 normals;
	Array_int triangleIndices;
	int verticesOffset;
	int normalsOffset;
	int uvsOffset;
	int triangleCount;
};

struct WeightData
{
	f32 *weights;
	int *weightCounts;
	int *weightIndices;
};

// FRAME
const u64 frameSize = 256 * 1024 * 1024;
void *frameMem;
void *framePtr;
void FrameInit()
{
	frameMem = malloc(stackSize);
	framePtr = frameMem;
}
void FrameFinalize()
{
	free(frameMem);
}
void *FrameAlloc(u64 size)
{
	ASSERT((u8 *)framePtr + size < (u8 *)frameMem + stackSize); // Out of memory!
	void *result;

	result = framePtr;
	framePtr = (u8 *)framePtr + size;

	return result;
}
void *FrameRealloc(void *ptr, u64 newSize)
{
	//ASSERT(false);
	//SDL_Log("WARNING: FRAME REALLOC\n");

	void *newBlock = FrameAlloc(newSize);
	memcpy(newBlock, ptr, newSize);
	return newBlock;
}
void FrameFree(void *ptr)
{
}
void FrameWipe()
{
	framePtr = frameMem;
}

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
	SDL_Log("Output name: %s\n", outputFilename);
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

int ReadColladaGeometry(const char *filename, RawGeometry *result)
{
	XMLError error;

	tinyxml2::XMLDocument doc;
	error = doc.LoadFile(filename);
	if (error != XML_SUCCESS)
	{
		SDL_Log("ERROR! Parsing XML file \"%s\" (%s)\n", filename, doc.ErrorStr());
		return error;
	}

	XMLElement *rootEl = doc.FirstChildElement("COLLADA");
	if (!rootEl)
	{
		SDL_Log("ERROR! Collada root node not found\n");
		return ERROR_COLLADA_NOROOT;
	}

	*result = {};

	// GEOMETRY
	XMLElement *geometryEl = rootEl->FirstChildElement("library_geometries")
		->FirstChildElement("geometry");
	SDL_Log("Found geometry %s\n", geometryEl->FindAttribute("name")->Value());

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
		ArrayInit_v3(&result->positions, positionsCount / 3);
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
		ArrayInit_v3(&result->normals, normalsCount / 3);
		ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), (f32 *)result->normals.data, normalsCount);
		result->normals.size = normalsCount / 3;
	}

	// Read uvs
	result->uvs = {};
	if (uvsSource)
	{
		XMLElement *arrayEl = uvsSource->FirstChildElement("float_array");
		int uvsCount = arrayEl->FindAttribute("count")->IntValue();
		ArrayInit_v2(&result->uvs, uvsCount / 2);
		ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), (f32 *)result->uvs.data, uvsCount);
		result->uvs.size = uvsCount / 2;
	}

	// Read indices
	result->triangleIndices;
	{
		const int totalIndexCount = result->triangleCount * 3 * attrCount;

		ArrayInit_int(&result->triangleIndices, totalIndexCount);
		ReadStringToInts(triangleIndicesSource->FirstChild()->ToText()->Value(),
				result->triangleIndices.data, totalIndexCount);
		result->triangleIndices.size = totalIndexCount;
	}
	return 0;
}

int ReadColladaController(const char *filename, Skeleton *skeleton, WeightData *weightData)
{
	XMLError error;

	tinyxml2::XMLDocument doc;
	error = doc.LoadFile(filename);
	if (error != XML_SUCCESS)
	{
		SDL_Log("ERROR! Parsing XML file \"%s\" (%s)\n", filename, doc.ErrorStr());
		return error;
	}

	XMLElement *rootEl = doc.FirstChildElement("COLLADA");
	if (!rootEl)
	{
		SDL_Log("ERROR! Collada root node not found\n");
		return ERROR_COLLADA_NOROOT;
	}

	// SKIN
	XMLElement *controllerEl = rootEl->FirstChildElement("library_controllers")
		->FirstChildElement("controller");
	SDL_Log("Found controller %s\n", controllerEl->FindAttribute("name")->Value());
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
		SDL_Log("Found visual scene %s\n", sceneEl->FindAttribute("id")->Value());
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

						//SDL_Log("Joint %s has parent %s\n", childName, parentName);
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

	return 0;
}

int ConstructSkinnedMesh(const char *filename, const RawGeometry *geometry,
		const WeightData *weightData,
		DynamicArray_RawVertex &finalVertices, Array_u16 &finalIndices)
{
	const u32 vertexTotal = geometry->positions.size;

	Array_SkinnedPosition skinnedPositions;
	ArrayInit_SkinnedPosition(&skinnedPositions, vertexTotal);
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
		void *oldStackPtr = stackPtr;

		const bool hasUvs = geometry->uvsOffset >= 0;
		const bool hasNormals = geometry->normalsOffset >= 0;
		const int attrCount = 1 + hasUvs + hasNormals;

		ArrayInit_u16(&finalIndices, geometry->triangleCount * 3);

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
		SDL_Log("Going with %d buckets for %d indices\n", nOfBuckets, geometry->triangleCount * 3);

		DynamicArray_u16 *buckets = (DynamicArray_u16 *)StackAlloc(sizeof(DynamicArray_u16) * nOfBuckets);
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

		SDL_Log("Vertex indexing collided %d times out of %d (%.02f%%)\n", collisions, total,
				100.0f * collisions / (f32)total);

		SDL_Log("Vertex indexing used %.02fkb of stack\n", ((u8 *)stackPtr - (u8 *)oldStackPtr) /
				1024.0f);
		StackFree(oldStackPtr);
	}

	SDL_Log("Ended up with %d unique vertices and %d indices\n", finalVertices.size,
			finalIndices.size);

	// Rewind triangles
	for (int i = 0; i < geometry->triangleCount; ++i)
	{
		u16 tmp = finalIndices[i * 3 + 1];
		finalIndices[i * 3 + 1] = finalIndices[i * 3 + 2];
		finalIndices[i * 3 + 2] = tmp;
	}

	return 0;
}

int ReadColladaAnimation(const char *filename, Animation &animation, Skeleton &skeleton)
{
	XMLError error;

	tinyxml2::XMLDocument doc;
	error = doc.LoadFile(filename);
	if (error != XML_SUCCESS)
	{
		SDL_Log("ERROR! Parsing XML file \"%s\" (%s)\n", filename, doc.ErrorStr());
		return error;
	}

	XMLElement *rootEl = doc.FirstChildElement("COLLADA");
	if (!rootEl)
	{
		SDL_Log("ERROR! Collada root node not found\n");
		return ERROR_COLLADA_NOROOT;
	}

	// ANIMATIONS
	DynamicArray_AnimationChannel channels;
	DynamicArrayInit_AnimationChannel(&channels, skeleton.jointCount, FrameAlloc);

	XMLElement *animationEl = rootEl->FirstChildElement("library_animations")
		->FirstChildElement("animation");
	SDL_Log("Found animation container %s\n", animationEl->FindAttribute("id")->Value());
	XMLElement *subAnimEl = animationEl->FirstChildElement("animation");
	while (subAnimEl != nullptr)
	{
		//SDL_Log("Found animation %s\n", subAnimEl->FindAttribute("id")->Value());

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
			//SDL_Log("Channel thought to target joint \"%s\", ID %d\n", jointIdBuffer,
					//channel.jointIndex);
		}

		int newChannelIdx = DynamicArrayAdd_AnimationChannel(&channels, FrameRealloc);
		channels[newChannelIdx] = channel;

		subAnimEl = subAnimEl->NextSiblingElement("animation");
	}
	animation.channels = channels.data;
	animation.channelCount = channels.size;

	return 0;
}

int OutputMesh(const char *filename, DynamicArray_RawVertex &finalVertices, Array_u16 &finalIndices)
{
	// Output!
	{
		SDL_RWops *newFile = SDL_RWFromFile(filename, "wb");
		{
			SDL_RWwrite(newFile, &finalVertices.size, sizeof(finalVertices.size), 1);
			SDL_RWwrite(newFile, &finalIndices.size, sizeof(finalIndices.size), 1);

			// Write vertex data
			for (u32 i = 0; i < finalVertices.size; ++i)
			{
				RawVertex *rawVertex = &finalVertices[i];
				Vertex gameVertex;
				gameVertex.pos = rawVertex->skinnedPos.pos;
				gameVertex.uv = rawVertex->uv;
				gameVertex.nor = rawVertex->normal;

				SDL_RWwrite(newFile, &gameVertex, sizeof(gameVertex), 1);
			}

			// Write indices
			for (u32 i = 0; i < finalIndices.size; ++i)
			{
				u16 outputIdx = finalIndices[i];
				SDL_RWwrite(newFile, &outputIdx, sizeof(outputIdx), 1);
			}
		}
		SDL_RWclose(newFile);
	}

	return 0;
}

int OutputSkinnedMesh(const char *filename, DynamicArray_RawVertex &finalVertices,
		Array_u16 &finalIndices, Skeleton &skeleton, DynamicArray_Animation &animations)
{
	// Output!
	{
		SDL_RWops *newFile = SDL_RWFromFile(filename, "wb");
		{
			SDL_RWwrite(newFile, &finalVertices.size, sizeof(finalVertices.size), 1);
			SDL_RWwrite(newFile, &finalIndices.size, sizeof(finalIndices.size), 1);

			// Write vertex data
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

				SDL_RWwrite(newFile, &gameVertex, sizeof(gameVertex), 1);
			}

			// Write indices
			for (u32 i = 0; i < finalIndices.size; ++i)
			{
				u16 outputIdx = finalIndices[i];
				SDL_RWwrite(newFile, &outputIdx, sizeof(outputIdx), 1);
			}

			// Write skeleton
			SDL_RWwrite(newFile, &skeleton.jointCount, sizeof(u32), 1);
			SDL_RWwrite(newFile, skeleton.bindPoses, sizeof(mat4) * skeleton.jointCount, 1);
			SDL_RWwrite(newFile, skeleton.jointParents, skeleton.jointCount, 1);
			SDL_RWwrite(newFile, skeleton.restPoses, sizeof(mat4) * skeleton.jointCount, 1);

			// Write animations
			SDL_RWwrite(newFile, &animations.size, sizeof(u32), 1);

			for (u32 animIdx = 0; animIdx < animations.size; ++animIdx)
			{
				Animation *animation = &animations[animIdx];
				SDL_RWwrite(newFile, &animation->frameCount, sizeof(u32), 1);
				SDL_RWwrite(newFile, animation->timestamps, sizeof(u32) * animation->frameCount, 1);

				SDL_RWwrite(newFile, &animation->channelCount, sizeof(u32), 1);
				for (u32 channelIdx = 0; channelIdx < animation->channelCount; ++channelIdx)
				{
					AnimationChannel *channel = &animation->channels[channelIdx];
					SDL_RWwrite(newFile, &channel->jointIndex, sizeof(u32), 1);
					SDL_RWwrite(newFile, channel->transforms, sizeof(mat4) * animation->frameCount, 1);
				}
			}
		}
		SDL_RWclose(newFile);
	}

	return 0;
}

int ReadMeta(const char *filename)
{
	XMLError xmlError;

	tinyxml2::XMLDocument doc;
	xmlError = doc.LoadFile(filename);
	if (xmlError != XML_SUCCESS)
	{
		SDL_Log("ERROR! Parsing XML file \"%s\" (%s)\n", filename, doc.ErrorStr());
		return xmlError;
	}

	XMLElement *rootEl = doc.FirstChildElement("meta");
	if (!rootEl)
	{
		SDL_Log("ERROR! Meta root node not found\n");
		return ERROR_META_NOROOT;
	}

	DynamicArray_RawVertex finalVertices = {};
	Array_u16 finalIndices = {};
	Skeleton skeleton = {};
	DynamicArray_Animation animations = {};

	DynamicArrayInit_Animation(&animations, 16, FrameAlloc);

	const char *typeStr = rootEl->FindAttribute("type")->Value();
	SDL_Log("Meta type: %s\n", typeStr);

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
		SDL_Log("ERROR! Invalid meta type\n");
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
			SDL_Log("Found geometry file: %s\n", geomFile);

			char fullname[MAX_PATH];
			sprintf(fullname, "%s%s", dataDir, geomFile);

			RawGeometry rawGeometry;
			int error = ReadColladaGeometry(fullname, &rawGeometry);
			if (error)
				return error;

			ConstructSkinnedMesh(fullname, &rawGeometry, nullptr, finalVertices,
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
			SDL_Log("Found geometry file: %s\n", geomFile);

			char fullname[MAX_PATH];
			sprintf(fullname, "%s%s", dataDir, geomFile);

			RawGeometry rawGeometry;
			int error = ReadColladaGeometry(fullname, &rawGeometry);
			if (error)
				return error;

			WeightData weightData;
			error = ReadColladaController(fullname, &skeleton, &weightData);
			if (error)
				return error;

			ConstructSkinnedMesh(fullname, &rawGeometry, &weightData, finalVertices,
					finalIndices);

			geometryEl = geometryEl->NextSiblingElement("geometry");
		}

		XMLElement *animationEl = rootEl->FirstChildElement("animation");
		while (animationEl != nullptr)
		{
			const char *animFile = animationEl->FirstChild()->ToText()->Value();
			SDL_Log("Found animation file: %s\n", animFile);

			char fullname[MAX_PATH];
			sprintf(fullname, "%s%s", dataDir, animFile);

			Animation &animation = animations[DynamicArrayAdd_Animation(&animations, FrameRealloc)];
			animation = {};
			int error = ReadColladaAnimation(fullname, animation, skeleton);
			if (error)
				return error;

			animationEl = animationEl->NextSiblingElement("animation");
		}

		// Output
		{
			char outputName[MAX_PATH];
			GetOutputFilename(filename, outputName);

			int error = OutputSkinnedMesh(outputName, finalVertices, finalIndices, skeleton, animations);
			if (error)
				return error;
		}
	} break;
	case METATYPE_TRIANGLE_DATA:
	{
		XMLElement *geometryEl = rootEl->FirstChildElement("geometry");

		const char *geomFile = geometryEl->FirstChild()->ToText()->Value();
		SDL_Log("Found geometry file: %s\n", geomFile);

		char fullname[MAX_PATH];
		sprintf(fullname, "%s%s", dataDir, geomFile);

		RawGeometry rawGeometry;
		int error = ReadColladaGeometry(fullname, &rawGeometry);
		if (error)
			return error;

		// Output
		char outputName[MAX_PATH];
		GetOutputFilename(filename, outputName);

		SDL_RWops *file = SDL_RWFromFile(outputName, "wb");

		const u32 triangleCount = rawGeometry.triangleCount;
		SDL_RWwrite(file, &triangleCount, sizeof(u32), 1);

		int attrCount = 1;
		if (rawGeometry.normalsOffset >= 0)
			++attrCount;
		if (rawGeometry.uvsOffset >= 0)
			++attrCount;

		for (u32 i = 0; i < triangleCount; ++i)
		{
			struct
			{
				v3 a;
				v3 b;
				v3 c;
				v3 normal;
			} triangle;

			int ai = rawGeometry.triangleIndices[(i * 3 + 0) * attrCount + rawGeometry.verticesOffset];
			int bi = rawGeometry.triangleIndices[(i * 3 + 2) * attrCount + rawGeometry.verticesOffset];
			int ci = rawGeometry.triangleIndices[(i * 3 + 1) * attrCount + rawGeometry.verticesOffset];
			//SDL_Log("Triangle: %d %d %d\n", ai, bi, ci);

			triangle.a = rawGeometry.positions[ai];
			triangle.b = rawGeometry.positions[bi];
			triangle.c = rawGeometry.positions[ci];
			//SDL_Log("Triangle: [%.02f,%.02f,%.02f] [%.02f,%.02f,%.02f] [%.02f,%.02f,%.02f]\n",
					//a.x, a.y, a.z,
					//b.x, b.y, b.z,
					//c.x, c.y, c.z);

			triangle.normal = V3Normalize(V3Cross(triangle.c - triangle.a, triangle.b - triangle.a));

			SDL_RWwrite(file, &triangle, sizeof(triangle), 1);
		}

		SDL_RWclose(file);
	} break;
	case METATYPE_POINTS:
	{
		XMLElement *geometryEl = rootEl->FirstChildElement("geometry");

		const char *geomFile = geometryEl->FirstChild()->ToText()->Value();
		SDL_Log("Found geometry file: %s\n", geomFile);

		char fullname[MAX_PATH];
		sprintf(fullname, "%s%s", dataDir, geomFile);

		RawGeometry rawGeometry;
		int error = ReadColladaGeometry(fullname, &rawGeometry);
		if (error)
			return error;

		// Output
		char outputName[MAX_PATH];
		GetOutputFilename(filename, outputName);

		SDL_RWops *file = SDL_RWFromFile(outputName, "wb");

		const u32 pointCount = rawGeometry.positions.size;
		SDL_RWwrite(file, &pointCount, sizeof(u32), 1);
		SDL_RWwrite(file, rawGeometry.positions.data, sizeof(v3), pointCount);

		SDL_RWclose(file);
	} break;
	};

	return 0;
}

int main(int argc, char **argv)
{
	SDL_Init(0);

	StackInit();
	FrameInit();

	int error = 0;

	char colladaWildcard[MAX_PATH];
	sprintf(colladaWildcard, "%s*.meta", dataDir);
	WIN32_FIND_DATA findData;
	HANDLE searchHandle = FindFirstFileA(colladaWildcard, &findData);
	char fullName[MAX_PATH];
	while (1)
	{
		sprintf(fullName, "%s%s", dataDir, findData.cFileName);
		SDL_Log("\nDetected file %s\n------------------------------\n", fullName);
		error = ReadMeta(fullName);

		SDL_Log("Used %.02fkb of frame allocator\n", ((u8 *)framePtr - (u8 *)frameMem) / 1024.0f);
		FrameWipe();

		if (error != 0)
			return error;

		if (!FindNextFileA(searchHandle, &findData))
			break;
	}

	StackFinalize();
	FrameFinalize();

	return error;
}
