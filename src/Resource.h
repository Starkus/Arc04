enum ResourceType
{
	RESOURCETYPE_MESH,
	RESOURCETYPE_SKINNEDMESH,
	RESOURCETYPE_LEVELGEOMETRYGRID,
	RESOURCETYPE_POINTS
};

struct ResourceMesh
{
	DeviceMesh deviceMesh;
};

struct ResourceSkinnedMesh
{
	DeviceMesh deviceMesh;
	u8 jointCount;
	mat4 *bindPoses;
	u8 *jointParents;
	mat4 *restPoses;
	u32 animationCount;
	Animation *animations;
};

struct ResourceGeometryGrid
{
	v2 lowCorner;
	v2 highCorner;
	int cellsSide;
	u32 *offsets;
	Triangle *triangles;
};

struct ResourcePointCloud
{
	v3 *pointData;
	u32 pointCount;
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
		ResourcePointCloud points;
	};
};
