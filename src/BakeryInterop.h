struct BakeryMeshHeader
{
	u32 vertexCount;
	u32 indexCount;
	u64 vertexBlobOffset;
	u64 indexBlobOffset;
};

struct BakerySkinnedMeshHeader
{
	u32 vertexCount;
	u32 indexCount;
	u64 vertexBlobOffset;
	u64 indexBlobOffset;

	u32 jointCount;
	u64 bindPosesBlobOffset;
	u64 jointParentsBlobOffset;
	u64 restPosesBlobOffset;

	u32 animationCount;
	u64 animationBlobOffset;
};

struct BakerySkinnedMeshAnimationHeader
{
	u32 frameCount;
	u32 channelCount;
	u64 timestampsBlobOffset;
	u64 channelsBlobOffset;
};

struct BakerySkinnedMeshAnimationChannelHeader
{
	u32 jointIndex;
	u64 transformsBlobOffset;
};

struct BakeryTriangleDataHeader
{
	u32 triangleCount;
	u64 trianglesBlobOffset;
};

struct BakeryPointsHeader
{
	u32 pointCount;
	u64 pointsBlobOffset;
};
