#include "BakeryInterop.h"

void ReadMesh(const u8 *fileBuffer, Vertex **vertexData, u16 **indexData, u32 *vertexCount, u32 *indexCount)
{
	BakeryMeshHeader *header = (BakeryMeshHeader *)fileBuffer;

	*vertexCount = header->vertexCount;
	*indexCount = header->indexCount;

	*vertexData = (Vertex *)(fileBuffer + header->vertexBlobOffset);
	*indexData = (u16 *)(fileBuffer + header->indexBlobOffset);
}

void ReadSkinnedMesh(const u8 *fileBuffer, SkeletalMesh *skinnedMesh, SkinnedVertex **vertexData,
		u16 **indexData, u32 *vertexCount)
{
	BakerySkinnedMeshHeader *header = (BakerySkinnedMeshHeader *)fileBuffer;

	*vertexCount = header->vertexCount;
	u32 indexCount = header->indexCount;

	skinnedMesh->deviceMesh.indexCount = indexCount;

	*vertexData = (SkinnedVertex *)(fileBuffer + header->vertexBlobOffset);
	*indexData = (u16 *)(fileBuffer + header->indexBlobOffset);

	u32 jointCount = header->jointCount;

	const u64 bindPosesBlobSize = sizeof(mat4) * jointCount;
	mat4 *bindPoses = (mat4 *)TransientAlloc(bindPosesBlobSize);
	Memcpy(bindPoses, fileBuffer + header->bindPosesBlobOffset, bindPosesBlobSize);

	const u64 jointParentsBlobSize = jointCount;
	u8 *jointParents = (u8 *)TransientAlloc(jointParentsBlobSize);
	Memcpy(jointParents, fileBuffer + header->jointParentsBlobOffset, jointParentsBlobSize);

	const u64 restPosesBlobSize = sizeof(mat4) * jointCount;
	mat4 *restPoses = (mat4 *)TransientAlloc(restPosesBlobSize);
	Memcpy(restPoses, fileBuffer + header->restPosesBlobOffset, restPosesBlobSize);

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
		Memcpy(timestamps, fileBuffer + animationHeader->timestampsBlobOffset, timestampsBlobSize);

		animation->frameCount = frameCount;
		animation->timestamps = timestamps;
		animation->channelCount = channelCount;
		animation->channels = (AnimationChannel *)TransientAlloc(sizeof(AnimationChannel) *channelCount);

		BakerySkinnedMeshAnimationChannelHeader *channelHeaders =
			(BakerySkinnedMeshAnimationChannelHeader *)(fileBuffer +
					animationHeader->channelsBlobOffset);
		for (u32 channelIdx = 0; channelIdx < channelCount; ++channelIdx)
		{
			AnimationChannel *channel = &animation->channels[channelIdx];

			BakerySkinnedMeshAnimationChannelHeader *channelHeader = &channelHeaders[channelIdx];

			u32 jointIndex = channelHeader->jointIndex;

			u64 transformsBlobSize = sizeof(mat4) * frameCount;
			mat4 *transforms = (mat4 *)TransientAlloc(transformsBlobSize);
			Memcpy(transforms, fileBuffer + channelHeader->transformsBlobOffset,
					transformsBlobSize);

			channel->jointIndex = jointIndex;
			channel->transforms = transforms;
		}
	}
}

void ReadTriangleGeometry(const u8 *fileBuffer, Triangle **triangleData, u32 *triangleCount)
{
	BakeryTriangleDataHeader *header = (BakeryTriangleDataHeader *)fileBuffer;

	*triangleCount = header->triangleCount;

	u64 trianglesBlobSize = sizeof(Triangle) * *triangleCount;
	*triangleData = (Triangle *)TransientAlloc(trianglesBlobSize);
	Memcpy(*triangleData, fileBuffer + header->trianglesBlobOffset, trianglesBlobSize);
}

void ReadPoints(const u8 *fileBuffer, v3 **pointData, u32 *pointCount)
{
	BakeryPointsHeader *header = (BakeryPointsHeader *)fileBuffer;

	*pointCount = header->pointCount;

	*pointData = (v3 *)TransientAlloc(sizeof(v3) * *pointCount);
	Memcpy(*pointData, fileBuffer + header->pointsBlobOffset, sizeof(v3) * *pointCount);
}
