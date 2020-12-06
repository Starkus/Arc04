#include "BakeryInterop.h"

void ReadMesh(const u8 *fileBuffer, Vertex **vertexData, u16 **indexData, u32 *vertexCount,
		u32 *indexCount, const char **materialFilename)
{
	BakeryMeshHeader *header = (BakeryMeshHeader *)fileBuffer;

	*vertexCount = header->vertexCount;
	*indexCount = header->indexCount;

	*vertexData = (Vertex *)(fileBuffer + header->vertexBlobOffset);
	*indexData = (u16 *)(fileBuffer + header->indexBlobOffset);

	*materialFilename = (const char *)(fileBuffer + header->materialNameOffset);
}

void ReadBakeryShader(const u8 *fileBuffer, const char **vertexShader, const char **fragmentShader)
{
	BakeryShaderHeader *header = (BakeryShaderHeader *)fileBuffer;
	*vertexShader = (const char *)(fileBuffer + header->vertexShaderBlobOffset);
	*fragmentShader = (const char *)(fileBuffer + header->fragmentShaderBlobOffset);
}

void ReadSkinnedMesh(const u8 *fileBuffer, ResourceSkinnedMesh *skinnedMesh, SkinnedVertex **vertexData,
		u16 **indexData, u32 *vertexCount, u32 *indexCount, const char **materialFilename)
{
	BakerySkinnedMeshHeader *header = (BakerySkinnedMeshHeader *)fileBuffer;

	*vertexCount = header->vertexCount;
	*indexCount = header->indexCount;

	*vertexData = (SkinnedVertex *)(fileBuffer + header->vertexBlobOffset);
	*indexData = (u16 *)(fileBuffer + header->indexBlobOffset);

	u32 jointCount = header->jointCount;

	const u64 bindPosesBlobSize = sizeof(Transform) * jointCount;
	Transform *bindPoses = (Transform *)TransientAlloc(bindPosesBlobSize); // @Broken: Memory leak with resource reload!!!
	memcpy(bindPoses, fileBuffer + header->bindPosesBlobOffset, bindPosesBlobSize);

	const u64 jointParentsBlobSize = jointCount;
	u8 *jointParents = (u8 *)TransientAlloc(jointParentsBlobSize);
	memcpy(jointParents, fileBuffer + header->jointParentsBlobOffset, jointParentsBlobSize);

	const u64 restPosesBlobSize = sizeof(Transform) * jointCount;
	Transform *restPoses = (Transform *)TransientAlloc(restPosesBlobSize);
	memcpy(restPoses, fileBuffer + header->restPosesBlobOffset, restPosesBlobSize);

	ASSERT(jointCount < U8_MAX);
	skinnedMesh->jointCount = (u8)jointCount;
	skinnedMesh->bindPoses = bindPoses;
	skinnedMesh->jointParents = jointParents;
	skinnedMesh->restPoses = restPoses;

	const u32 animationCount = header->animationCount;
	skinnedMesh->animationCount = animationCount;

	const u64 animationsBlobSize = sizeof(Animation) * animationCount;
	skinnedMesh->animations = (Animation *)TransientAlloc(animationsBlobSize);

	BakerySkinnedMeshAnimationHeader *animationHeaders = (BakerySkinnedMeshAnimationHeader *)
		(fileBuffer + header->animationBlobOffset);
	for (u32 animIdx = 0; animIdx < animationCount; ++animIdx)
	{
		Animation *animation = &skinnedMesh->animations[animIdx];

		BakerySkinnedMeshAnimationHeader *animationHeader = &animationHeaders[animIdx];

		u32 frameCount = animationHeader->frameCount;
		u32 channelCount = animationHeader->channelCount;

		const u64 timestampsBlobSize = sizeof(f32) * frameCount;
		f32 *timestamps = (f32 *)TransientAlloc(timestampsBlobSize);
		memcpy(timestamps, fileBuffer + animationHeader->timestampsBlobOffset, timestampsBlobSize);

		animation->frameCount = frameCount;
		animation->timestamps = timestamps;
		animation->channelCount = channelCount;
		animation->loop = animationHeader->loop;
		animation->channels = (AnimationChannel *)TransientAlloc(sizeof(AnimationChannel) *channelCount);

		BakerySkinnedMeshAnimationChannelHeader *channelHeaders =
			(BakerySkinnedMeshAnimationChannelHeader *)(fileBuffer +
					animationHeader->channelsBlobOffset);
		for (u32 channelIdx = 0; channelIdx < channelCount; ++channelIdx)
		{
			AnimationChannel *channel = &animation->channels[channelIdx];

			BakerySkinnedMeshAnimationChannelHeader *channelHeader = &channelHeaders[channelIdx];

			u32 jointIndex = channelHeader->jointIndex;

			u64 transformsBlobSize = sizeof(Transform) * frameCount;
			Transform *transforms = (Transform *)TransientAlloc(transformsBlobSize);
			memcpy(transforms, fileBuffer + channelHeader->transformsBlobOffset,
					transformsBlobSize);

			channel->jointIndex = jointIndex;
			channel->transforms = transforms;
		}
	}

	*materialFilename = (const char *)(fileBuffer + header->materialNameOffset);
}

void ReadTriangleGeometry(const u8 *fileBuffer, ResourceGeometryGrid *geometryGrid)
{
	BakeryTriangleDataHeader *header = (BakeryTriangleDataHeader *)fileBuffer;
	geometryGrid->lowCorner = header->lowCorner;
	geometryGrid->highCorner = header->highCorner;
	geometryGrid->cellsSide = header->cellsSide;
	geometryGrid->positionCount = header->positionCount;

	u32 offsetCount = header->cellsSide * header->cellsSide + 1;
	u64 offsetBlobSize = sizeof(u32) * offsetCount;
	geometryGrid->offsets = (u32 *)TransientAlloc(offsetBlobSize);
	memcpy(geometryGrid->offsets, fileBuffer + header->offsetsBlobOffset, offsetBlobSize);

	u32 positionCount = header->positionCount;
	u64 positionsBlobSize = sizeof(v3) * positionCount;
	geometryGrid->positions = (v3 *)TransientAlloc(positionsBlobSize);
	memcpy(geometryGrid->positions, fileBuffer + header->positionsBlobOffset, positionsBlobSize);

	u32 triangleCount = geometryGrid->offsets[offsetCount - 1];
	u64 trianglesBlobSize = sizeof(Triangle) * triangleCount;
	geometryGrid->triangles = (IndexTriangle *)TransientAlloc(trianglesBlobSize);
	memcpy(geometryGrid->triangles, fileBuffer + header->trianglesBlobOffset, trianglesBlobSize);
}

void ReadCollisionMesh(const u8 *fileBuffer, ResourceCollisionMesh *collisionMesh)
{
	BakeryCollisionMeshHeader *header = (BakeryCollisionMeshHeader *)fileBuffer;

	collisionMesh->positionCount = header->positionCount;
	collisionMesh->positionData = (v3 *)TransientAlloc(sizeof(v3) * collisionMesh->positionCount);
	memcpy(collisionMesh->positionData, fileBuffer + header->positionsBlobOffset, sizeof(v3) *
			collisionMesh->positionCount);

	collisionMesh->triangleCount = header->triangleCount;
	u64 trianglesBlobSize = sizeof(IndexTriangle) * collisionMesh->triangleCount;
	collisionMesh->triangleData = (IndexTriangle *)TransientAlloc(trianglesBlobSize);
	memcpy(collisionMesh->triangleData, fileBuffer + header->trianglesBlobOffset, trianglesBlobSize);
}

void ReadImage(const u8 *fileBuffer, const u8 **imageData, u32 *width, u32 *height, u32 *components)
{
	BakeryImageHeader *header = (BakeryImageHeader *)fileBuffer;

	*width = header->width;
	*height = header->height;
	*components = header->components;
	*imageData = fileBuffer + header->dataBlobOffset;
}

void ReadMaterial(const u8 *fileBuffer, RawBakeryMaterial *rawMaterial)
{
	BakeryMaterialHeader *header = (BakeryMaterialHeader *)fileBuffer;
	rawMaterial->shaderFilename = (const char *)fileBuffer + header->shaderNameOffset;
	rawMaterial->textureCount = (u8)header->textureCount;

	const char *currentTexName = (const char *)fileBuffer + header->textureNamesOffset;
	for (u8 texIdx = 0; texIdx < header->textureCount; ++texIdx)
	{
		rawMaterial->textureFilenames[texIdx] = currentTexName;
		currentTexName += strlen(currentTexName) + 1;
	}
}
