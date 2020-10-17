#include "BakeryInterop.h"

void *ReadMesh(const char *filename, Vertex **vertexData, u16 **indexData, u32 *vertexCount, u32 *indexCount)
{
	SDL_RWops *file = SDL_RWFromFile(filename, "rb");
	const u64 fileSize = SDL_RWsize(file);
	u8 *fileBuffer = (u8 *)malloc(fileSize);
	SDL_RWread(file, fileBuffer, sizeof(u8), fileSize);
	SDL_RWclose(file);

	u8 *fileScan = fileBuffer;

	BakeryMeshHeader *header = (BakeryMeshHeader *)fileScan;

	*vertexCount = header->vertexCount;
	*indexCount = header->indexCount;

	*vertexData = (Vertex *)(fileScan + header->vertexBlobOffset);
	*indexData = (u16 *)(fileScan + header->indexBlobOffset);

	// The caller must free the buffer after copying data to the render device
	return fileBuffer;
}

void *ReadSkinnedMesh(const char *filename, SkeletalMesh *skinnedMesh, SkinnedVertex **vertexData,
		u16 **indexData, u32 *vertexCount)
{
	SDL_RWops *file = SDL_RWFromFile(filename, "rb");
	const u64 fileSize = SDL_RWsize(file);
	u8 *fileBuffer = (u8 *)malloc(fileSize);
	SDL_RWread(file, fileBuffer, sizeof(u8), fileSize);
	SDL_RWclose(file);

	BakerySkinnedMeshHeader *header = (BakerySkinnedMeshHeader *)fileBuffer;

	*vertexCount = header->vertexCount;
	u32 indexCount = header->indexCount;

	skinnedMesh->deviceMesh.indexCount = indexCount;

	*vertexData = (SkinnedVertex *)(fileBuffer + header->vertexBlobOffset);
	*indexData = (u16 *)(fileBuffer + header->indexBlobOffset);

	u32 jointCount = header->jointCount;

	const u64 bindPosesBlobSize = sizeof(mat4) * jointCount;
	mat4 *bindPoses = (mat4 *)malloc(bindPosesBlobSize);
	memcpy(bindPoses, fileBuffer + header->bindPosesBlobOffset, bindPosesBlobSize);

	const u64 jointParentsBlobSize = jointCount;
	u8 *jointParents = (u8 *)malloc(jointParentsBlobSize);
	memcpy(jointParents, fileBuffer + header->jointParentsBlobOffset, jointParentsBlobSize);

	const u64 restPosesBlobSize = sizeof(mat4) * jointCount;
	mat4 *restPoses = (mat4 *)malloc(restPosesBlobSize);
	memcpy(restPoses, fileBuffer + header->restPosesBlobOffset, restPosesBlobSize);

	ASSERT(jointCount < U8_MAX);
	skinnedMesh->jointCount = (u8)jointCount;
	skinnedMesh->bindPoses = bindPoses;
	skinnedMesh->jointParents = jointParents;
	skinnedMesh->restPoses = restPoses;

	const u32 animationCount = header->animationCount;
	skinnedMesh->animationCount = animationCount;

	const u64 animationsBlobSize = sizeof(Animation) * animationCount;
	skinnedMesh->animations = (Animation *)malloc(animationsBlobSize);

	BakerySkinnedMeshAnimationHeader *animationHeaders = (BakerySkinnedMeshAnimationHeader *)
		(fileBuffer + header->animationBlobOffset);
	for (u32 animIdx = 0; animIdx < animationCount; ++animIdx)
	{
		Animation *animation = &skinnedMesh->animations[animIdx];

		BakerySkinnedMeshAnimationHeader *animationHeader = &animationHeaders[animIdx];

		u32 frameCount = animationHeader->frameCount;
		u32 channelCount = animationHeader->channelCount;

		const u64 timestampsBlobSize = sizeof(f32) * frameCount;
		f32 *timestamps = (f32 *)malloc(timestampsBlobSize);
		memcpy(timestamps, fileBuffer + animationHeader->timestampsBlobOffset, timestampsBlobSize);

		animation->frameCount = frameCount;
		animation->timestamps = timestamps;
		animation->channelCount = channelCount;
		animation->channels = (AnimationChannel *)malloc(sizeof(AnimationChannel) *channelCount);

		BakerySkinnedMeshAnimationChannelHeader *channelHeaders =
			(BakerySkinnedMeshAnimationChannelHeader *)(fileBuffer +
					animationHeader->channelsBlobOffset);
		for (u32 channelIdx = 0; channelIdx < channelCount; ++channelIdx)
		{
			AnimationChannel *channel = &animation->channels[channelIdx];

			BakerySkinnedMeshAnimationChannelHeader *channelHeader = &channelHeaders[channelIdx];

			u32 jointIndex = channelHeader->jointIndex;

			u64 transformsBlobSize = sizeof(mat4) * frameCount;
			mat4 *transforms = (mat4 *)malloc(transformsBlobSize);
			memcpy(transforms, fileBuffer + channelHeader->transformsBlobOffset,
					transformsBlobSize);

			channel->jointIndex = jointIndex;
			channel->transforms = transforms;
		}
	}

	// The caller must free the buffer after copying data to the render device
	return fileBuffer;
}

void ReadTriangleGeometry(const char *filename, Triangle **triangleData, u32 *triangleCount)
{
	SDL_RWops *file = SDL_RWFromFile(filename, "rb");
	const u64 fileSize = SDL_RWsize(file);
	u8 *fileBuffer = (u8 *)malloc(fileSize);
	SDL_RWread(file, fileBuffer, sizeof(u8), fileSize);
	SDL_RWclose(file);

	BakeryTriangleDataHeader *header = (BakeryTriangleDataHeader *)fileBuffer;

	*triangleCount = header->triangleCount;

	u64 trianglesBlobSize = sizeof(Triangle) * *triangleCount;
	*triangleData = (Triangle *)malloc(trianglesBlobSize);
	memcpy(*triangleData, fileBuffer + header->trianglesBlobOffset, trianglesBlobSize);

	free(fileBuffer);
}

void ReadPoints(const char *filename, v3 **pointData, u32 *pointCount)
{
	SDL_RWops *file = SDL_RWFromFile(filename, "rb");
	const u64 fileSize = SDL_RWsize(file);
	u8 *fileBuffer = (u8 *)malloc(fileSize);
	SDL_RWread(file, fileBuffer, sizeof(u8), fileSize);
	SDL_RWclose(file);

	BakeryPointsHeader *header = (BakeryPointsHeader *)fileBuffer;

	*pointCount = header->pointCount;

	*pointData = (v3 *)malloc(sizeof(v3) * *pointCount);
	memcpy(*pointData, fileBuffer + header->pointsBlobOffset, sizeof(v3) * *pointCount);

	free(fileBuffer);
}
