struct Vertex
{
	v3 pos;
	v2 uv;
	v3 nor;
};

struct SkinnedVertex
{
	v3 pos;
	v2 uv;
	v3 nor;
	u16 indices[4];
	f32 weights[4];
};

struct Triangle
{
	union
	{
		struct
		{
			v3 a;
			v3 b;
			v3 c;
		};
		v3 corners[3];
	};
	v3 normal;
};

struct IndexTriangle
{
	union
	{
		struct
		{
			u16 a;
			u16 b;
			u16 c;
		};
		u16 corners[3];
	};
	v3 normal;
};

struct GeometryGrid
{
	v2 lowCorner;
	v2 highCorner;
	int cellsSide;
	u32 *offsets;
	v3 *positions;
	IndexTriangle *triangles;
};

struct AnimationChannel
{
	u32 jointIndex;
	Transform *transforms;
};

struct Animation
{
	u32 frameCount;
	f32 *timestamps;
	u32 channelCount;
	bool loop;
	AnimationChannel *channels;
};

struct SkinnedMesh
{
	DeviceMesh deviceMesh;
	u8 jointCount;
	Transform *bindPoses;
	u8 *jointParents;
	Transform *restPoses;
	u32 animationCount;
	Animation *animations;
};
