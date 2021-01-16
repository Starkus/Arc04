enum ResourceType
{
	RESOURCETYPE_MESH,
	RESOURCETYPE_SKINNEDMESH,
	RESOURCETYPE_LEVELGEOMETRYGRID,
	RESOURCETYPE_COLLISIONMESH,
	RESOURCETYPE_SHADER,
	RESOURCETYPE_TEXTURE,
	RESOURCETYPE_MATERIAL
};

struct Resource;

struct ResourceMesh
{
	DeviceMesh deviceMesh;
	const Resource *materialRes;
};

struct ResourceSkinnedMesh
{
	DeviceMesh deviceMesh;
	const Resource *materialRes;
	u8 jointCount;
	Transform *bindPoses;
	u8 *jointParents;
	Transform *restPoses;
	u32 animationCount;
	Animation *animations;
};

struct ResourceGeometryGrid
{
	v2 lowCorner;
	v2 highCorner;
	int cellsSide;
	u32 *offsets;
	u32 positionCount;
	v3 *positions;
	IndexTriangle *triangles;
};

struct ResourceCollisionMesh
{
	v3 *positionData;
	u32 positionCount;
	IndexTriangle *triangleData;
	u32 triangleCount;
};

struct ResourceShader
{
	DeviceProgram programHandle;
};

struct ResourceTexture
{
	u32 width;
	u32 height;
	u32 components;
	DeviceTexture deviceTexture;
};

struct ResourceMaterial
{
	const Resource *shaderRes;
	u8 textureCount;
	const Resource *textures[8];
};

struct Resource
{
	ResourceType type;
	char filename[128];
	union
	{
		ResourceMesh mesh;
		ResourceSkinnedMesh skinnedMesh;
		ResourceGeometryGrid geometryGrid;
		ResourceCollisionMesh collisionMesh;
		ResourceShader shader;
		ResourceTexture texture;
		ResourceMaterial material;
	};
};

struct ResourceVoucher
{
	ResourceType type;
	String filename;
};
