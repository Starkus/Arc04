const char *vertexShaderSource = "\
#version 330 core\n\
layout (location = 0) in vec3 pos;\n\
layout (location = 1) in vec2 uv;\n\
layout (location = 2) in vec3 nor;\n\
uniform mat4 model;\n\
uniform mat4 view;\n\
uniform mat4 projection;\n\
out vec3 normal;\n\
\n\
void main()\n\
{\n\
	gl_Position = projection * view * model * vec4(pos, 1.0);\n\
	normal = (model * vec4(nor, 0.0)).xyz;\n\
}\n\
";

const char *fragShaderSource = "\
#version 330 core\n\
in vec3 normal;\n\
out vec4 fragColor;\n\
\n\
void main()\n\
{\n\
	vec3 lightDir = normalize(vec3(1, 0.7, 1.3));\n\
	float light = dot(normal, lightDir);\n\
	fragColor = vec4(light * 0.5 + 0.5);\n\
	//fragColor = vec4(normal, 0) * 0.5 + vec4(0.5);\n\
}\n\
";

const char *skinVertexShaderSource = "\
#version 330 core\n\
layout (location = 0) in vec3 pos;\n\
layout (location = 1) in vec2 uv;\n\
layout (location = 2) in vec3 nor;\n\
layout (location = 3) in uvec4 indices;\n\
layout (location = 4) in vec4 weights;\n\
uniform mat4 model;\n\
uniform mat4 view;\n\
uniform mat4 projection;\n\
uniform mat4 joints[128];\n\
out vec3 normal;\n\
\n\
void main()\n\
{\n\
	vec4 oldPos = vec4(pos, 1.0);\n\
	vec4 newPos = (joints[indices.x] * oldPos) * weights.x;\n\
	newPos += (joints[indices.y] * oldPos) * weights.y;\n\
	newPos += (joints[indices.z] * oldPos) * weights.z;\n\
	newPos += (joints[indices.w] * oldPos) * weights.w;\n\
\n\
	vec4 oldNor = vec4(nor, 0.0);\n\
	vec4 newNor = (joints[indices.x] * oldNor) * weights.x;\n\
	newNor += (joints[indices.y] * oldNor) * weights.y;\n\
	newNor += (joints[indices.z] * oldNor) * weights.z;\n\
	newNor += (joints[indices.w] * oldNor) * weights.w;\n\
\n\
	gl_Position = projection * view * model * vec4(newPos.xyz, 1.0);\n\
	normal = (model * vec4(newNor.xyz, 0)).xyz;\n\
}\n\
";

const char *debugDrawFragShaderSource = "\
#version 330 core\n\
in vec3 normal;\n\
out vec4 fragColor;\n\
\n\
void main()\n\
{\n\
	fragColor = vec4(normal, 0) * 0.5 + vec4(0.5);\n\
}\n\
";

struct AnimationChannel
{
	u32 jointIndex;
	mat4 *transforms;
};

struct Animation
{
	u32 frameCount;
	f32 *timestamps;
	u32 channelCount;
	AnimationChannel *channels;
};

struct SkeletalMesh
{
	DeviceMesh deviceMesh;
	u8 jointCount;
	mat4 *bindPoses;
	u8 *jointParents;
	mat4 *restPoses;
	u32 animationCount;
	Animation *animations;
};


enum ColliderType
{
	COLLIDER_CONVEX_HULL,
	COLLIDER_SPHERE,
	COLLIDER_CYLINDER,
	COLLIDER_CAPSULE
};

struct Collider
{
	ColliderType type;
	union
	{
		struct
		{
			v3 *collisionPoints;
			u32 collisionPointCount;
		} convexHull;
		struct
		{
			f32 radius;
			v3 offset;
		} sphere;
		struct
		{
			f32 radius;
			f32 height;
			v3 offset;
		} cylinder, capsule;
	};
};

struct Entity
{
	v3 pos;
	v3 fw;
	DeviceMesh *mesh;
	Collider collider;
};

struct LevelGeometry
{
	DeviceMesh renderMesh;
	Triangle *triangles;
	u32 triangleCount;
};

enum PlayerState
{
	PLAYERSTATE_GROUNDED,
	PLAYERSTATE_AIR
};

struct Player
{
	Entity *entity;
	v3 vel;
	PlayerState state;
};

struct GameState
{
	v3 camPos;
	f32 camYaw;
	f32 camPitch;
	u32 entityCount;
	Entity entities[256];
	LevelGeometry levelGeometry;
	Player player;

	int animationIdx;
	f32 animationTime;
	bool loopAnimation;

	// @Cleanup: move to some Render Device Context or something?
	void *platformReadEntireFile;
	DeviceProgram program, skinnedMeshProgram, debugDrawProgram;
	DeviceMesh anvilMesh, cubeMesh, sphereMesh, cylinderMesh, capsuleMesh;
	SkeletalMesh skinnedMesh;
};

#if DEBUG_BUILD
struct DebugCube
{
	v3 pos;
	v3 fw;
	v3 up;
	f32 scale;
};
DebugCube debugCubes[2048];
u32 debugCubeCount;
#define DRAW_DEBUG_CUBE(pos, fw, up, scale) if (debugCubeCount < 2048) debugCubes[debugCubeCount++] = { pos, fw, up, scale }
#define DRAW_AA_DEBUG_CUBE(pos, scale) if (debugCubeCount < 2048) debugCubes[debugCubeCount++] = { pos, {0,1,0}, {0,0,1}, scale }

struct DebugGeometryBuffer
{
	DeviceMesh deviceMesh;
	Vertex *vertexData;
	u32 vertexCount;
};
DebugGeometryBuffer debugGeometryBuffer;
void DrawDebugTriangles(Vertex* vertices, int count)
{
	for (int i = 0; i < count; ++i)
	{
		debugGeometryBuffer.vertexData[debugGeometryBuffer.vertexCount + i] = vertices[i];
	}
	debugGeometryBuffer.vertexCount += count;
}

int g_currentPolytopeStep;
#endif
