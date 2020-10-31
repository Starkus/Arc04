#define PLATFORM_LOG(name) void name(const char *format, ...)
typedef PLATFORM_LOG(Log_t);

#define PLATFORM_READ_ENTIRE_FILE(name) u8 *name(const char *filename, void *(*allocFunc)(u64))
typedef PLATFORM_READ_ENTIRE_FILE(ReadEntireFile_t);

#define RENDER_SET_UP_DEVICE(name) void name()
typedef RENDER_SET_UP_DEVICE(SetUpDevice_t);

#define RENDER_CLEAR_BUFFERS(name) void name(v4 clearColor)
typedef RENDER_CLEAR_BUFFERS(ClearBuffers_t);

#define RENDER_GET_UNIFORM(name) DeviceUniform name(DeviceProgram program, const char *name)
typedef RENDER_GET_UNIFORM(GetUniform_t);

#define RENDER_USE_PROGRAM(name) void name(DeviceProgram program)
typedef RENDER_USE_PROGRAM(UseProgram_t);

#define RENDER_UNIFORM_MAT4(name) void name(DeviceUniform uniform, u32 count, const f32 *buffer)
typedef RENDER_UNIFORM_MAT4(UniformMat4_t);

#define RENDER_RENDER_INDEXED_MESH(name) void name(DeviceMesh mesh)
typedef RENDER_RENDER_INDEXED_MESH(RenderIndexedMesh_t);

#define RENDER_RENDER_MESH(name) void name(DeviceMesh mesh)
typedef RENDER_RENDER_MESH(RenderMesh_t);

#define RENDER_RENDER_LINES(name) void name(DeviceMesh mesh)
typedef RENDER_RENDER_LINES(RenderLines_t);

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

#define RENDER_CREATE_DEVICE_PROGRAM(name) DeviceProgram name(DeviceShader vertexShader, \
		DeviceShader fragmentShader)
typedef RENDER_CREATE_DEVICE_PROGRAM(CreateDeviceProgram_t);

#define SET_FILL_MODE(name) void name(RenderFillMode mode)
typedef SET_FILL_MODE(SetFillMode_t);

#define RESOURCE_LOAD_MESH(name) const Resource *name(const char *filename)
typedef RESOURCE_LOAD_MESH(ResourceLoadMesh_t);

#define RESOURCE_LOAD_SKINNED_MESH(name) const Resource *name(const char *filename)
typedef RESOURCE_LOAD_SKINNED_MESH(ResourceLoadSkinnedMesh_t);

#define RESOURCE_LOAD_LEVEL_GEOMETRY_GRID(name) const Resource *name(const char *filename)
typedef RESOURCE_LOAD_LEVEL_GEOMETRY_GRID(ResourceLoadLevelGeometryGrid_t);

#define RESOURCE_LOAD_POINTS(name) const Resource *name(const char *filename)
typedef RESOURCE_LOAD_POINTS(ResourceLoadPoints_t);

#define GET_RESOURCE(name) const Resource *name(const char *filename)
typedef GET_RESOURCE(GetResource_t);

struct PlatformCode
{
	Log_t *Log;
	ReadEntireFile_t *ReadEntireFile;

	SetUpDevice_t *SetUpDevice;
	ClearBuffers_t *ClearBuffers;
	GetUniform_t *GetUniform;
	UseProgram_t *UseProgram;
	UniformMat4_t *UniformMat4;
	RenderIndexedMesh_t *RenderIndexedMesh;
	RenderMesh_t *RenderMesh;
	RenderLines_t *RenderLines;
	CreateDeviceMesh_t *CreateDeviceMesh;
	CreateDeviceIndexedMesh_t *CreateDeviceIndexedMesh;
	CreateDeviceIndexedSkinnedMesh_t *CreateDeviceIndexedSkinnedMesh;
	SendMesh_t *SendMesh;
	SendIndexedMesh_t *SendIndexedMesh;
	SendIndexedSkinnedMesh_t *SendIndexedSkinnedMesh;
	LoadShader_t *LoadShader;
	CreateDeviceProgram_t *CreateDeviceProgram;
	SetFillMode_t *SetFillMode;

	ResourceLoadMesh_t *ResourceLoadMesh;
	ResourceLoadSkinnedMesh_t *ResourceLoadSkinnedMesh;
	ResourceLoadLevelGeometryGrid_t *ResourceLoadLevelGeometryGrid;
	ResourceLoadPoints_t *ResourceLoadPoints;
	GetResource_t *GetResource;
};

#define START_GAME(name) void name(Memory *memory, PlatformCode *platformCode)
typedef START_GAME(StartGame_t);
START_GAME(StartGameStub) { (void) memory, platformCode; }

#define UPDATE_AND_RENDER_GAME(name) void name(Controller *controller, Memory *memory, \
		PlatformCode *platformCode, f32 deltaTime)
typedef UPDATE_AND_RENDER_GAME(UpdateAndRenderGame_t);
UPDATE_AND_RENDER_GAME(UpdateAndRenderGameStub) { (void) controller, memory, platformCode, deltaTime; }
