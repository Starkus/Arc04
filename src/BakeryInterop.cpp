void *ReadMesh(const char *filename, Vertex **vertexData, u16 **indexData, u32 *vertexCount, u32 *indexCount)
{
	SDL_RWops *file = SDL_RWFromFile(filename, "rb");
	const u64 fileSize = SDL_RWsize(file);
	u8 *fileBuffer = (u8 *)malloc(fileSize);
	SDL_RWread(file, fileBuffer, sizeof(u8), fileSize);
	SDL_RWclose(file);

	u8 *fileScan = fileBuffer;

	*vertexCount = *(u32 *)fileScan;
	fileScan += sizeof(u32);
	*indexCount = *(u32 *)fileScan;
	fileScan += sizeof(u32);

	*vertexData = (Vertex *)fileScan;
	fileScan += sizeof(Vertex) * *vertexCount;
	*indexData = (u16 *)fileScan;
	fileScan += sizeof(u16) * *indexCount;

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

	u8 *fileScan = fileBuffer;

	*vertexCount = *(u32 *)fileScan;
	fileScan += sizeof(u32);
	u32 indexCount = *(u32 *)fileScan;
	fileScan += sizeof(u32);

	skinnedMesh->deviceMesh.indexCount = indexCount;

	*vertexData = (SkinnedVertex *)fileScan;
	fileScan += sizeof(SkinnedVertex) * *vertexCount;
	*indexData = (u16 *)fileScan;
	fileScan += sizeof(u16) * indexCount;

	u8 jointCount = *fileScan;
	fileScan += sizeof(u32);
	mat4 *bindPoses = (mat4 *)malloc(sizeof(mat4) * jointCount);
	memcpy(bindPoses, fileScan, sizeof(mat4) * jointCount);
	fileScan += sizeof(mat4) * jointCount;
	u8 *jointParents = (u8 *)malloc(jointCount);
	memcpy(jointParents, fileScan, jointCount);
	fileScan += jointCount;
	mat4 *restPoses = (mat4 *)malloc(sizeof(mat4) * jointCount);
	memcpy(restPoses, fileScan, sizeof(mat4) * jointCount);
	fileScan += sizeof(mat4) * jointCount;

	skinnedMesh->jointCount = jointCount;
	skinnedMesh->bindPoses = bindPoses;
	skinnedMesh->jointParents = jointParents;
	skinnedMesh->restPoses = restPoses;

	u32 animationCount = *(u32 *)fileScan;
	fileScan += sizeof(u32);

	skinnedMesh->animationCount = animationCount;
	skinnedMesh->animations = (Animation *)malloc(sizeof(Animation) * animationCount);

	for (u32 animIdx = 0; animIdx < animationCount; ++animIdx)
	{
		Animation *animation = &skinnedMesh->animations[animIdx];

		u32 frameCount = *(u32 *)fileScan;
		fileScan += sizeof(u32);
		f32 *timestamps = (f32 *)malloc(sizeof(f32) * frameCount);
		memcpy(timestamps, fileScan, sizeof(f32) * frameCount);
		fileScan += sizeof(f32) * frameCount;
		u32 channelCount = *(u32 *)fileScan;
		fileScan += sizeof(u32);

		animation->frameCount = frameCount;
		animation->timestamps = timestamps;
		animation->channelCount = channelCount;
		animation->channels = (AnimationChannel *)malloc(sizeof(AnimationChannel) *channelCount);

		for (u32 channelIdx = 0; channelIdx < channelCount; ++channelIdx)
		{
			AnimationChannel *channel = &animation->channels[channelIdx];

			u8 jointIndex = *fileScan;
			fileScan += sizeof(u32);
			mat4 *transforms = (mat4 *)malloc(sizeof(mat4) * frameCount);
			memcpy(transforms, fileScan, sizeof(mat4) * frameCount);
			fileScan += sizeof(mat4) * frameCount;

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

	u8 *fileScan = fileBuffer;

	*triangleCount = *(u32 *)fileScan;
	fileScan += sizeof(u32);

	*triangleData = (Triangle *)malloc(sizeof(Triangle) * *triangleCount);
	memcpy(*triangleData, fileScan, sizeof(Triangle) * *triangleCount);
	fileScan += sizeof(Triangle) * *triangleCount;

	free(fileBuffer);
}

void ReadPoints(const char *filename, v3 **pointData, u32 *pointCount)
{
	SDL_RWops *file = SDL_RWFromFile(filename, "rb");
	const u64 fileSize = SDL_RWsize(file);
	u8 *fileBuffer = (u8 *)malloc(fileSize);
	SDL_RWread(file, fileBuffer, sizeof(u8), fileSize);
	SDL_RWclose(file);

	u8 *fileScan = fileBuffer;

	*pointCount = *(u32 *)fileScan;
	fileScan += sizeof(u32);

	*pointData = (v3 *)malloc(sizeof(v3) * *pointCount);
	memcpy(*pointData, fileScan, sizeof(v3) * *pointCount);

	free(fileBuffer);
}
