const GLchar *vertexShaderSource = "\
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

const GLchar *fragShaderSource = "\
#version 330 core\n\
in vec3 normal;\n\
out vec4 fragColor;\n\
\n\
void main()\n\
{\n\
	vec3 lightDir = normalize(vec3(1, 0.7, 1.3));\n\
	float light = dot(normal, lightDir);\n\
	fragColor = vec4(light * 0.5 + 0.5);\n\
}\n\
";

const GLchar *skinVertexShaderSource = "\
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

struct DeviceMesh
{
	GLuint vao;
	union
	{
		struct
		{
			GLuint vertexBuffer;
			GLuint indexBuffer;
		};
		GLuint buffers[2];
	};
	u32 indexCount;
};

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

struct Button
{
	bool endedDown;
	bool changed;
};

union Controller
{
	struct
	{
		Button up;
		Button down;
		Button left;
		Button right;
		Button jump;
		Button camUp;
		Button camDown;
		Button camLeft;
		Button camRight;

#if DEBUG_BUILD
		Button debugUp;
		Button debugDown;
		Button debugLeft;
		Button debugRight;
#endif
	};
	Button b[13];
};

struct Entity
{
	v3 pos;
	v3 fw;
	DeviceMesh *mesh;
	v3 *collisionPoints;
	u32 collisionPointCount;
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
	Controller controller;
	v3 camPos;
	f32 camYaw;
	f32 camPitch;
	u32 entityCount;
	Entity entities[256];
	LevelGeometry levelGeometry;
	Player player;

	int animationIdx;
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
	GLuint vao;
	GLuint deviceBuffer;
	Vertex *vertexData;
	u32 vertexCount;
};
DebugGeometryBuffer debugGeometryBuffer;
void DrawDebugTriangles(Vertex* vertices, int count)
{
	memcpy(debugGeometryBuffer.vertexData + debugGeometryBuffer.vertexCount, vertices,
			count * sizeof(Vertex));
	debugGeometryBuffer.vertexCount += count;
}

int g_currentPolytopeStep;
#endif
