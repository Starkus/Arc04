const GLchar *vertexShaderSource = "\
#version 330 core\n\
layout (location = 0) in vec3 pos;\n\
layout (location = 1) in vec3 col;\n\
uniform mat4 model;\n\
uniform mat4 view;\n\
uniform mat4 projection;\n\
out vec3 vertexColor;\n\
\n\
void main()\n\
{\n\
	gl_Position = projection * view * model * vec4(pos, 1.0);\n\
	vertexColor = col;\n\
}\n\
";

const GLchar *fragShaderSource = "\
#version 330 core\n\
in vec3 vertexColor;\n\
out vec4 fragColor;\n\
\n\
void main()\n\
{\n\
	fragColor = vec4(vertexColor, 1.0);\n\
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
		Button camUp;
		Button camDown;
		Button camLeft;
		Button camRight;

#if EPA_VISUAL_DEBUGGING
		Button epaStepUp;
		Button epaStepDown;
#endif
	};
	Button b[10];
};

struct Entity
{
	v3 pos;
	v3 fw;
};

struct GameState
{
	Controller controller;
	v3 camPos;
	f32 camYaw;
	f32 camPitch;
	u32 entityCount;
	Entity entities[256];
	u32 playerEntity;
};
