#include <windows.h>
#include <stdio.h>

#include "tinyxml2.cpp"
using namespace tinyxml2;

#include "General.h"
#include "Maths.h"
#include "Containers.h"
#include "Geometry.h"

// DEFER
#ifndef _DEFER_HELPER_DEFINED
#define _DEFER_HELPER_DEFINED
class _DEFER_HELPER
{
public:
	void *ptr;
	_DEFER_HELPER(void *ptr) : ptr(ptr) {}
	~_DEFER_HELPER() { free(ptr); }
};
#endif
#define _DEFER_HELPER_CONCAT_(a, b) a##b
#define _DEFER_HELPER_CONCAT2_(a, b) _DEFER_HELPER_CONCAT_(a, b)
#define DeferredFree(ptr) _DEFER_HELPER _DEFER_HELPER_CONCAT2_(_defer_helper_, __LINE__) = { ptr }
////////

const char *dataDir = "../data/";

enum ErrorCode
{
	ERROR_OK,
	ERROR_COLLADA_NOROOT = XML_ERROR_COUNT
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

DECLARE_ARRAY(u16);
DECLARE_ARRAY(int);
DECLARE_ARRAY(f32);
DECLARE_ARRAY(v3);
DECLARE_ARRAY(v2);
DECLARE_ARRAY(mat4);
DECLARE_ARRAY(SkinnedPosition);
DECLARE_DYNAMIC_ARRAY(RawVertex);
DECLARE_DYNAMIC_ARRAY(AnimationChannel);

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

int ReadCollada(const char *filename)
{
	XMLError error;

	tinyxml2::XMLDocument doc;
	error = doc.LoadFile(filename);
	if (error != XML_SUCCESS)
	{
		printf("ERROR! Parsing XML file \"%s\" (%s)\n", filename, doc.ErrorStr());
		return error;
	}

	XMLElement *rootEl = doc.FirstChildElement("COLLADA");
	if (!rootEl)
	{
		printf("ERROR! Collada root node not found\n");
		return ERROR_COLLADA_NOROOT;
	}

	// GEOMETRY
	XMLElement *geometryEl = rootEl->FirstChildElement("library_geometries")
		->FirstChildElement("geometry");
	printf("Found geometry %s\n", geometryEl->FindAttribute("name")->Value());

	XMLElement *meshEl = geometryEl->FirstChildElement("mesh");

	XMLElement *verticesSource = 0;
	XMLElement *normalsSource = 0;
	XMLElement *positionsSource = 0;
	XMLElement *uvsSource = 0;
	XMLElement *triangleIndicesSource = 0;
	int triangleIndicesCount = -1;

	const char *verticesSourceName = 0;
	const char *normalsSourceName = 0;
	const char *positionsSourceName = 0;
	const char *uvsSourceName = 0;

	int verticesOffset = -1;
	int normalsOffset = -1;
	int uvsOffset = -1;
	int attrCount = 0;

	// Get triangle source ids and index array
	{
		XMLElement *trianglesEl = meshEl->FirstChildElement("triangles");
		triangleIndicesSource = trianglesEl->FirstChildElement("p");
		triangleIndicesCount = trianglesEl->FindAttribute("count")->IntValue();

		XMLElement *inputEl = trianglesEl->FirstChildElement("input");
		while (inputEl != nullptr)
		{
			const char *semantic = inputEl->FindAttribute("semantic")->Value();
			const char *source = inputEl->FindAttribute("source")->Value();
			int offset = inputEl->FindAttribute("offset")->IntValue();
			if (strcmp(semantic, "VERTEX") == 0)
			{
				verticesSourceName = source + 1;
				verticesOffset = offset;
				++attrCount;
			}
			else if (strcmp(semantic, "NORMAL") == 0)
			{
				normalsSourceName = source + 1;
				normalsOffset = offset;
				++attrCount;
			}
			else if (strcmp(semantic, "TEXCOORD") == 0)
			{
				uvsSourceName = source + 1;
				uvsOffset = offset;
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
	Array_v3 positions;
	{
		XMLElement *arrayEl = positionsSource->FirstChildElement("float_array");
		int positionsCount = arrayEl->FindAttribute("count")->IntValue();
		ArrayInit_v3(&positions, positionsCount / 3);
		ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), (f32 *)positions.data,
				positionsCount);
		positions.size = positionsCount / 3;
	}
	DeferredFree(positions.data);

	// Read normals
	Array_v3 normals = {};
	if (normalsSource)
	{
		XMLElement *arrayEl = normalsSource->FirstChildElement("float_array");
		int normalsCount = arrayEl->FindAttribute("count")->IntValue();
		ArrayInit_v3(&normals, normalsCount / 3);
		ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), (f32 *)normals.data, normalsCount);
		normals.size = normalsCount / 3;
	}
	DeferredFree(normals.data);

	// Read uvs
	Array_v2 uvs = {};
	if (uvsSource)
	{
		XMLElement *arrayEl = uvsSource->FirstChildElement("float_array");
		int uvsCount = arrayEl->FindAttribute("count")->IntValue();
		ArrayInit_v2(&uvs, uvsCount / 2);
		ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), (f32 *)uvs.data, uvsCount);
		uvs.size = uvsCount / 2;
	}
	DeferredFree(uvs.data);

	// Read indices
	Array_int triangleIndices;
	{
		const int totalIndexCount = triangleIndicesCount * 3 * attrCount;

		ArrayInit_int(&triangleIndices, totalIndexCount);
		ReadStringToInts(triangleIndicesSource->FirstChild()->ToText()->Value(),
				triangleIndices.data, totalIndexCount);
		triangleIndices.size = totalIndexCount;
	}
	DeferredFree(triangleIndices.data);

	// SKIN
	XMLElement *controllerEl = rootEl->FirstChildElement("library_controllers")
		->FirstChildElement("controller");
	printf("Found controller %s\n", controllerEl->FindAttribute("name")->Value());
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
	f32 *weights = 0;
	{
		XMLElement *arrayEl = weightsSource->FirstChildElement("float_array");
		int count = arrayEl->FindAttribute("count")->IntValue();
		weights = (f32 *)malloc(sizeof(f32) * count);
		ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), weights, count);
	}
	DeferredFree(weights);

	Skeleton skeleton = {};
	char *jointNamesBuffer = 0;
	// Read joint names
	{
		XMLElement *arrayEl = jointsSource->FirstChildElement("Name_array");
		skeleton.jointCount = (u8)arrayEl->FindAttribute("count")->IntValue();
		skeleton.ids = (char **)malloc(sizeof(char *) * skeleton.jointCount);

		const char *in = arrayEl->FirstChild()->ToText()->Value();
		jointNamesBuffer = (char *)malloc(sizeof(char *) * strlen(in));
		strcpy(jointNamesBuffer, in);
		char *wordBegin = jointNamesBuffer;
		int jointIdx = 0;
		for (char *scan = jointNamesBuffer; ; ++scan)
		{
			if (*scan == ' ')
			{
				if (wordBegin != scan)
				{
					skeleton.ids[jointIdx++] = wordBegin;
					*scan = 0;
				}
				wordBegin = scan + 1;
			}
			else if (*scan == 0)
			{
				if (wordBegin != scan)
				{
					skeleton.ids[jointIdx++] = wordBegin;
				}
				break;
			}
		}
	}
	DeferredFree(jointNamesBuffer);
	DeferredFree(skeleton.ids);

	// Read bind poses
	{
		XMLElement *arrayEl = bindPosesSource->FirstChildElement("float_array");
		int count = arrayEl->FindAttribute("count")->IntValue();
		ASSERT(count / 16 == skeleton.jointCount);
		skeleton.bindPoses = (mat4 *)malloc(sizeof(f32) * count);
		ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), (f32 *)skeleton.bindPoses,
				count);
	}
	DeferredFree(skeleton.bindPoses);

	// Read counts
	int *weightCounts = 0;
	{
		weightCounts = (int *)malloc(sizeof(int) * weightIndexCount);
		ReadStringToInts(weightCountSource->FirstChild()->ToText()->Value(), weightCounts, weightIndexCount);
	}
	DeferredFree(weightCounts);

	// Read weight indices
	int *weightIndices = 0;
	{
		int count = 0;
		for (int i = 0; i < weightIndexCount; ++i)
			count += weightCounts[i] * 2; // FIXME hardcoded 2!
		weightIndices = (int *)malloc(sizeof(int) * count);
		ReadStringToInts(weightIndicesSource->FirstChild()->ToText()->Value(), weightIndices, count);
	}
	DeferredFree(weightIndices);

	Array_SkinnedPosition skinnedPositions;
	ArrayInit_SkinnedPosition(&skinnedPositions, positions.size);
	DeferredFree(skinnedPositions.data);
	{
		int *weightCountScan = weightCounts;
		int *weightIndexScan = weightIndices;
		for (u32 i = 0; i < positions.size; ++i)
		{
			SkinnedPosition *skinnedPos = &skinnedPositions[skinnedPositions.size++];
			skinnedPos->pos = positions[i];

			int jointsNum = *weightCountScan++;
			ASSERT(jointsNum <= 4);
			for (int j = 0; j < jointsNum; ++j)
			{
				skinnedPos->jointIndices[j] = (u8)*weightIndexScan++;
				skinnedPos->jointWeights[j] = weights[*weightIndexScan++];
			}
			for (int j = jointsNum; j < 4; ++j)
			{
				skinnedPos->jointIndices[j] = 0;
				skinnedPos->jointWeights[j] = 0;
			}
		}
	}

	// Join positions and normals and eliminate duplicates
	DynamicArray_RawVertex finalVertices;
	Array_u16 finalIndices;
	{
		const bool hasUvs = uvsOffset >= 0;
		const bool hasNormals = normalsOffset >= 0;

		// TODO something better than n*m map for when things get big?
		const int normalsCount = hasNormals ? normals.size : 1;
		const int uvsCount = hasUvs ? uvs.size : 1;
		const int mapSize = normalsCount * uvsCount * positions.size;
		Array_u16 vertexIndexMap;
		ArrayInit_u16(&vertexIndexMap, mapSize);
		memset(vertexIndexMap.data, 0xFFFF, sizeof(u16) * mapSize);
		vertexIndexMap.size = mapSize;
		DeferredFree(vertexIndexMap.data);

		finalVertices.capacity = 32;
		DynamicArrayInit_RawVertex(&finalVertices);

		ArrayInit_u16(&finalIndices, triangleIndicesCount * 3);

		for (int i = 0; i < triangleIndicesCount * 3; ++i)
		{
			const int posIdx = triangleIndices[i * attrCount + verticesOffset];
			const int uvIdx = hasUvs * triangleIndices[i * attrCount + uvsOffset];
			const int norIdx = hasNormals * triangleIndices[i * attrCount + normalsOffset];
			SkinnedPosition pos = skinnedPositions[posIdx];

			v2 uv = {};
			if (hasUvs)
				uv = uvs[uvIdx];

			v3 nor = {};
			if (hasNormals)
				nor = normals[norIdx];

			const int mapIdx = (posIdx * normalsCount + norIdx) * uvsCount + uvIdx;
			if (vertexIndexMap[mapIdx] == 0xFFFF)
			{
				RawVertex newVertex = { pos, uv, nor };

				// Check it's different than every other vertex!
				for (u32 vertIdx = 0; vertIdx < finalVertices.size; ++vertIdx)
				{
					RawVertex v = finalVertices[vertIdx];
					ASSERT(memcmp(&v, &newVertex, sizeof(RawVertex)) != 0);
				}

				// Push back vertex
				u16 newVertexIdx = (u16)DynamicArrayAdd_RawVertex(&finalVertices);
				finalVertices[newVertexIdx] = newVertex;

				vertexIndexMap[mapIdx] = newVertexIdx;
				finalIndices[finalIndices.size++] = newVertexIdx;

#if 0
				v3 pp = newVertex.skinnedPos.pos;
				v2 uu = newVertex.uv;
				v3 nn = newVertex.normal;
				printf("Idx: %d {%.02f, %.02f, %.02f} {%.02f, %.02f} {%.02f, %.02f, %.02f}\n",
						newVertexIdx, pp.x, pp.y, pp.z, uu.x, uu.y, nn.x, nn.y, nn.z);
#endif
			}
			else
			{
				const u16 newVertexIdx = vertexIndexMap[mapIdx];
				finalIndices[finalIndices.size++] = newVertexIdx;

#if 0
				v3 pp = finalVertices[newVertexIdx].skinnedPos.pos;
				v2 uu = finalVertices[newVertexIdx].uv;
				v3 nn = finalVertices[newVertexIdx].normal;
				printf("Idx: %d {%.02f, %.02f, %.02f} {%.02f, %.02f} {%.02f, %.02f, %.02f}\n",
						newVertexIdx, pp.x, pp.y, pp.z, uu.x, uu.y, nn.x, nn.y, nn.z);
#endif
			}
		}
	}

	printf("Ended up with %d unique vertices and %d indices\n", finalVertices.size,
			finalIndices.size);

	// ANIMATIONS
	Animation animation = {};

	DynamicArray_AnimationChannel channels;
	channels.capacity = 16;
	DynamicArrayInit_AnimationChannel(&channels);

	XMLElement *animationEl = rootEl->FirstChildElement("library_animations")
		->FirstChildElement("animation");
	printf("Found animation container %s\n", animationEl->FindAttribute("id")->Value());
	XMLElement *subAnimEl = animationEl->FirstChildElement("animation");
	while (subAnimEl != nullptr)
	{
		printf("Found animation %s\n", subAnimEl->FindAttribute("id")->Value());

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
			animation.timestamps = (f32 *)malloc(sizeof(f32) * animation.frameCount);
			ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), animation.timestamps,
					animation.frameCount);
		}

		AnimationChannel channel = {};

		// Read matrices
		{
			XMLElement *arrayEl = matricesSource->FirstChildElement("float_array");
			int count = arrayEl->FindAttribute("count")->IntValue();
			ASSERT(animation.frameCount == (u32)(count / 16));
			channel.transforms = (mat4 *)malloc(sizeof(f32) * count);
			ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), (f32 *)channel.transforms,
					count);
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
			channel.jointIndex;
			for (int i = 0; i < skeleton.jointCount; ++i)
			{
				if (strcmp(skeleton.ids[i], jointIdBuffer) == 0)
				{
					ASSERT(i < 0xFF);
					channel.jointIndex = (u8)i;
					break;
				}
			}
			printf("Channel thought to target joint \"%s\", ID %d\n", jointIdBuffer,
					channel.jointIndex);
		}

		int newChannelIdx = DynamicArrayAdd_AnimationChannel(&channels);
		channels[newChannelIdx] = channel;

		subAnimEl = subAnimEl->NextSiblingElement("animation");
	}
	animation.channels = channels.data;
	animation.channelCount = channels.size;

	// Output!
	{
		char outputName[MAX_PATH];
		strcpy(outputName, filename);
		// Change extension
		char *lastDot = 0;
		for (char *scan = outputName; ; ++scan)
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
		printf("Output name: %s\n", outputName);

		HANDLE newFile = CreateFileA(outputName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL, NULL);
		DWORD bytesWritten;
		{
			WriteFile(newFile, &finalVertices.size, sizeof(finalVertices.size), &bytesWritten, NULL);
			WriteFile(newFile, &finalIndices.size, sizeof(finalIndices.size), &bytesWritten, NULL);

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

				WriteFile(newFile, &gameVertex, sizeof(gameVertex), &bytesWritten, NULL);
				ASSERT(bytesWritten == sizeof(SkinnedVertex));
			}

			// Write indices
			for (u32 i = 0; i < finalIndices.size; ++i)
			{
				u16 outputIdx = finalIndices[i];
				WriteFile(newFile, &outputIdx, sizeof(outputIdx), &bytesWritten, NULL);
				ASSERT(bytesWritten == sizeof(outputIdx));
			}

			// Write skeleton
			WriteFile(newFile, &skeleton.jointCount, sizeof(u32), &bytesWritten, NULL);
			WriteFile(newFile, skeleton.bindPoses, sizeof(mat4) * skeleton.jointCount, &bytesWritten, NULL);

			// Write animations
			// Write animation count
			const u32 animationCount = 1;
			WriteFile(newFile, &animationCount, sizeof(u32), &bytesWritten, NULL);

			WriteFile(newFile, &animation.frameCount, sizeof(u32), &bytesWritten, NULL);
			WriteFile(newFile, animation.timestamps, sizeof(f32) * animation.frameCount, &bytesWritten, NULL);

			WriteFile(newFile, &animation.channelCount, sizeof(u32), &bytesWritten, NULL);
			for (u32 channelIdx = 0; channelIdx < animation.channelCount; ++channelIdx)
			{
				AnimationChannel *channel = &animation.channels[channelIdx];
				WriteFile(newFile, &channel->jointIndex, sizeof(u32), &bytesWritten, NULL);
				WriteFile(newFile, channel->transforms, sizeof(mat4) * animation.frameCount, &bytesWritten, NULL);
			}
		}
		CloseHandle(newFile);
	}

	free(finalVertices.data);
	free(finalIndices.data);

	return 0;
}

int main(int argc, char **argv)
{
	char colladaWildcard[MAX_PATH];
	sprintf(colladaWildcard, "%s*.dae", dataDir);
	WIN32_FIND_DATA findData;
	HANDLE searchHandle = FindFirstFileA(colladaWildcard, &findData);
	char fullName[MAX_PATH];
	while (1)
	{
		sprintf(fullName, "%s%s", dataDir, findData.cFileName);
		printf("Detected file %s\n", fullName);
		int error = ReadCollada(fullName);
		if (error != 0)
			return error;

		if (!FindNextFileA(searchHandle, &findData))
			break;
	}

	return 0;
}
