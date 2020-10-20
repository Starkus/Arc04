#define PLATFORM_LOG(name) void name(const char *format, ...)
typedef PLATFORM_LOG(Log_t);

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

struct GameMemory
{
	void *frameMem, *stackMem, *transientMem;
};

// @Cleanup: move to render.h?
enum ShaderType
{
	SHADERTYPE_VERTEX,
	SHADERTYPE_FRAGMENT
};

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
	u32 vertexCount; // @Cleanup: remove?
	u32 indexCount;
};

struct DeviceShader
{
	GLuint shader;
};

struct DeviceProgram
{
	GLuint program;
};

struct DeviceUniform
{
	GLuint location;
};

#define PLATFORM_READ_ENTIRE_FILE(name) u8 *name(const char *filename)
typedef PLATFORM_READ_ENTIRE_FILE(PlatformReadEntireFile_t);

#define RENDER_SET_UP_DEVICE(name) void name()
typedef RENDER_SET_UP_DEVICE(SetUpDevice_t);

#define RENDER_CLEAR_BUFFERS(name) void name(v4 clearColor)
typedef RENDER_CLEAR_BUFFERS(ClearBuffers_t);

#define RENDER_GET_UNIFORM(name) DeviceUniform name(DeviceProgram *program, const char *name)
typedef RENDER_GET_UNIFORM(GetUniform_t);

#define RENDER_USE_PROGRAM(name) void name(DeviceProgram *program)
typedef RENDER_USE_PROGRAM(UseProgram_t);

#define RENDER_UNIFORM_MAT4(name) void name(DeviceUniform *uniform, u32 count, const f32 *buffer)
typedef RENDER_UNIFORM_MAT4(UniformMat4_t);

#define RENDER_RENDER_INDEXED_MESH(name) void name(DeviceMesh *mesh)
typedef RENDER_RENDER_INDEXED_MESH(RenderIndexedMesh_t);

#define RENDER_RENDER_MESH(name) void name(DeviceMesh *mesh)
typedef RENDER_RENDER_MESH(RenderMesh_t);

#define RENDER_CREATE_DEVICE_MESH(name) DeviceMesh name()
typedef RENDER_CREATE_DEVICE_MESH(CreateDeviceMesh_t);

#define RENDER_CREATE_DEVICE_INDEXED_MESH(name) DeviceMesh name()
typedef RENDER_CREATE_DEVICE_INDEXED_MESH(CreateDeviceIndexedMesh_t);

#define CREATE_DEVICE_INDEXED_SKINNED_MESH(name) DeviceMesh name()
typedef CREATE_DEVICE_INDEXED_SKINNED_MESH(CreateDeviceIndexedSkinnedMesh_t);

#define RENDER_SEND_MESH(name) void name(DeviceMesh *mesh, void *vertexData, u32 vertexCount, bool dynamic)
typedef RENDER_SEND_MESH(SendMesh_t);

#define RENDER_SEND_INDEXED_MESH(name) void name(DeviceMesh *mesh, void *vertexData, u32 vertexCount, \
		void *indexData, u32 indexCount, bool dynamic)
typedef RENDER_SEND_INDEXED_MESH(SendIndexedMesh_t);

#define RENDER_SEND_INDEXED_SKINNED_MESH(name) void name(DeviceMesh *mesh, void *vertexData, \
		u32 vertexCount, void *indexData, u32 indexCount, bool dynamic)
typedef RENDER_SEND_INDEXED_SKINNED_MESH(SendIndexedSkinnedMesh_t);

#define RENDER_LOAD_SHADER(name) DeviceShader name(const GLchar *shaderSource, ShaderType shaderType)
typedef RENDER_LOAD_SHADER(LoadShader_t);

#define RENDER_CREATE_DEVICE_PROGRAM(name) DeviceProgram name(DeviceShader *vertexShader, \
		DeviceShader *fragmentShader)
typedef RENDER_CREATE_DEVICE_PROGRAM(CreateDeviceProgram_t);

struct PlatformCode
{
	Log_t *Log;
	PlatformReadEntireFile_t *PlatformReadEntireFile;

	SetUpDevice_t *SetUpDevice;
	ClearBuffers_t *ClearBuffers;
	GetUniform_t *GetUniform;
	UseProgram_t *UseProgram;
	UniformMat4_t *UniformMat4;
	RenderIndexedMesh_t *RenderIndexedMesh;
	RenderMesh_t *RenderMesh;
	CreateDeviceMesh_t *CreateDeviceMesh;
	CreateDeviceIndexedMesh_t *CreateDeviceIndexedMesh;
	CreateDeviceIndexedSkinnedMesh_t *CreateDeviceIndexedSkinnedMesh;
	SendMesh_t *SendMesh;
	SendIndexedMesh_t *SendIndexedMesh;
	SendIndexedSkinnedMesh_t *SendIndexedSkinnedMesh;
	LoadShader_t *LoadShader;
	CreateDeviceProgram_t *CreateDeviceProgram;
};

#define START_GAME(name) void name(GameMemory *gameMemory, PlatformCode *platformCode)
typedef START_GAME(StartGame_t);
START_GAME(StartGameStub) { (void) gameMemory, platformCode; }

#define UPDATE_AND_RENDER_GAME(name) void name(Controller *controller, f32 deltaTime)
typedef UPDATE_AND_RENDER_GAME(UpdateAndRenderGame_t);
UPDATE_AND_RENDER_GAME(UpdateAndRenderGameStub) { (void) controller, deltaTime; }
