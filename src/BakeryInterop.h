struct BakeryMeshHeader
{
	u32 vertexCount;
	u32 indexCount;
	u64 vertexBlobOffset;
	u64 indexBlobOffset;
	u64 materialNameOffset;
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

	u64 materialNameOffset;
};

struct BakerySkinnedMeshAnimationHeader
{
	u32 frameCount;
	u32 channelCount;
	bool loop;
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
	v2 lowCorner;
	v2 highCorner;
	u32 cellsSide;
	u64 offsetsBlobOffset;

	u32 positionCount;
	u64 positionsBlobOffset;

	u64 trianglesBlobOffset;
};

struct BakeryCollisionMeshHeader
{
	u32 positionCount;
	u64 positionsBlobOffset;
	u32 triangleCount;
	u64 trianglesBlobOffset;
};

struct BakeryShaderHeader
{
	u64 vertexShaderBlobOffset;
	u64 fragmentShaderBlobOffset;
};

struct BakeryImageHeader
{
	u32 width;
	u32 height;
	u32 components;
	u64 dataBlobOffset;
};

struct BakeryMaterialHeader
{
	u32 textureCount;
	u64 shaderNameOffset;
	u64 textureNamesOffset;
};

struct RawBakeryMaterial
{
	const char *shaderFilename;
	u8 textureCount;
	const char *textureFilenames[8];
};
