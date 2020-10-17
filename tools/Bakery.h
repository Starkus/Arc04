const char *dataDir = "../data/";

// My macros don't work with "XmlElement*" as type =(
typedef XMLElement* XMLElementPtr;

DECLARE_ARRAY(u16);
DECLARE_ARRAY(int);
DECLARE_ARRAY(f32);
DECLARE_ARRAY(v3);
DECLARE_ARRAY(v2);
DECLARE_ARRAY(mat4);
DECLARE_ARRAY(BakerySkinnedMeshAnimationHeader);
DECLARE_ARRAY(BakerySkinnedMeshAnimationChannelHeader);
DECLARE_DYNAMIC_ARRAY(u16);
DECLARE_DYNAMIC_ARRAY(XMLElementPtr);

enum ErrorCode
{
	ERROR_OK,
	ERROR_META_NOROOT = XML_ERROR_COUNT,
	ERROR_COLLADA_NOROOT,
	ERROR_META_WRONG_TYPE
};

enum MetaType
{
	METATYPE_INVALID = -1,
	METATYPE_MESH,
	METATYPE_SKINNED_MESH,
	METATYPE_TRIANGLE_DATA,
	METATYPE_POINTS,
	METATYPE_COUNT
};

const char *MetaTypeNames[] =
{
	"MESH",
	"SKINNED_MESH",
	"TRIANGLE_DATA",
	"POINTS"
};

struct SkinnedPosition
{
	v3 pos;
	u8 jointIndices[4];
	f32 jointWeights[4];
};
DECLARE_ARRAY(SkinnedPosition);

struct RawVertex
{
	SkinnedPosition skinnedPos;
	v2 uv;
	v3 normal;
};
DECLARE_DYNAMIC_ARRAY(RawVertex);

struct Skeleton
{
	u8 jointCount;
	char **ids;
	mat4 *bindPoses;
	u8 *jointParents;
	mat4 *restPoses;
};

struct AnimationChannel
{
	u8 jointIndex;
	mat4 *transforms;
};
DECLARE_DYNAMIC_ARRAY(AnimationChannel);

struct Animation
{
	u32 frameCount;
	f32 *timestamps;
	u32 channelCount;
	AnimationChannel *channels;
};
DECLARE_DYNAMIC_ARRAY(Animation);

struct RawGeometry
{
	Array_v3 positions;
	Array_v2 uvs;
	Array_v3 normals;
	Array_int triangleIndices;
	int verticesOffset;
	int normalsOffset;
	int uvsOffset;
	int triangleCount;
};

struct WeightData
{
	f32 *weights;
	int *weightCounts;
	int *weightIndices;
};