#define PLATFORM_LOG(name) void name(const char *format, ...)
typedef PLATFORM_LOG(Log_t);

#define PLATFORM_READ_ENTIRE_FILE(name) bool name(const char *filename, u8 **fileBuffer, \
		u64 *fileSize, void *(*allocFunc)(u64))
typedef PLATFORM_READ_ENTIRE_FILE(PlatformReadEntireFile_t);

#ifdef USING_IMGUI
#define PLATFORM_GET_IMGUI_CONTEXT(name) ImGuiContext *name()
typedef PLATFORM_GET_IMGUI_CONTEXT(PlatformGetImguiContext_t);
#endif

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

#define RENDER_UNIFORM_INT(name) void name(DeviceUniform uniform, int n)
typedef RENDER_UNIFORM_INT(UniformInt_t);

#define RENDER_RENDER_INDEXED_MESH(name) void name(DeviceMesh mesh)
typedef RENDER_RENDER_INDEXED_MESH(RenderIndexedMesh_t);

#define RENDER_RENDER_MESH(name) void name(DeviceMesh mesh)
typedef RENDER_RENDER_MESH(RenderMesh_t);

#define RENDER_RENDER_MESH_INSTANCED(name) void name(DeviceMesh mesh, DeviceMesh positions, \
		u32 meshAttribs, u32 instAttribs)
typedef RENDER_RENDER_MESH_INSTANCED(RenderMeshInstanced_t);

#define RENDER_RENDER_INDEXED_MESH_INSTANCED(name) void name(DeviceMesh mesh, DeviceMesh positions, \
		u32 meshAttribs, u32 instAttribs)
typedef RENDER_RENDER_INDEXED_MESH_INSTANCED(RenderIndexedMeshInstanced_t);

#define RENDER_RENDER_LINES(name) void name(DeviceMesh mesh)
typedef RENDER_RENDER_LINES(RenderLines_t);

#define RENDER_CREATE_DEVICE_MESH(name) DeviceMesh name(int attribs)
typedef RENDER_CREATE_DEVICE_MESH(CreateDeviceMesh_t);

#define RENDER_CREATE_DEVICE_INDEXED_MESH(name) DeviceMesh name(int attribs)
typedef RENDER_CREATE_DEVICE_INDEXED_MESH(CreateDeviceIndexedMesh_t);

#define RENDER_CREATE_DEVICE_TEXTURE(name) DeviceTexture name()
typedef RENDER_CREATE_DEVICE_TEXTURE(CreateDeviceTexture_t);

#define RENDER_SEND_MESH(name) void name(DeviceMesh *mesh, void *vertexData, u32 vertexCount, \
		u32 stride, bool dynamic)
typedef RENDER_SEND_MESH(SendMesh_t);

#define RENDER_SEND_INDEXED_MESH(name) void name(DeviceMesh *mesh, void *vertexData, u32 vertexCount, \
		u32 stride, void *indexData, u32 indexCount, bool dynamic)
typedef RENDER_SEND_INDEXED_MESH(SendIndexedMesh_t);

#define RENDER_SEND_TEXTURE(name) void name(DeviceTexture texture, const void *imageData, u32 width, \
		u32 height, u32 components)
typedef RENDER_SEND_TEXTURE(SendTexture_t);

#define RENDER_BIND_TEXTURE(name) void name(DeviceTexture texture, int slot)
typedef RENDER_BIND_TEXTURE(BindTexture_t);

#define RENDER_CREATE_SHADER(name) DeviceShader name(ShaderType shaderType)
typedef RENDER_CREATE_SHADER(CreateShader_t);

#define RENDER_LOAD_SHADER(name) bool name(DeviceShader *shader, const char *shaderSource)
typedef RENDER_LOAD_SHADER(LoadShader_t);

#define RENDER_ATTACH_SHADER(name) void name(DeviceProgram program, DeviceShader shader)
typedef RENDER_ATTACH_SHADER(AttachShader_t);

#define RENDER_CREATE_DEVICE_PROGRAM(name) DeviceProgram name()
typedef RENDER_CREATE_DEVICE_PROGRAM(CreateDeviceProgram_t);

#define RENDER_LINK_DEVICE_PROGRAM(name) bool name(DeviceProgram program)
typedef RENDER_LINK_DEVICE_PROGRAM(LinkDeviceProgram_t);

#define RENDER_SET_VIEWPORT(name) void name(int posX, int posY, int width, int height)
typedef RENDER_SET_VIEWPORT(SetViewport_t);

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

#define RESOURCE_LOAD_SHADER(name) const Resource *name(const char *filename)
typedef RESOURCE_LOAD_SHADER(ResourceLoadShader_t);

#define RESOURCE_LOAD_TEXTURE(name) const Resource *name(const char *filename)
typedef RESOURCE_LOAD_TEXTURE(ResourceLoadTexture_t);

#define GET_RESOURCE(name) const Resource *name(const char *filename)
typedef GET_RESOURCE(GetResource_t);

struct PlatformCode
{
	Log_t *Log;
	PlatformReadEntireFile_t *PlatformReadEntireFile;

#ifdef USING_IMGUI
	PlatformGetImguiContext_t *PlatformGetImguiContext;
#endif

	SetUpDevice_t *SetUpDevice;
	ClearBuffers_t *ClearBuffers;
	GetUniform_t *GetUniform;
	UseProgram_t *UseProgram;
	UniformMat4_t *UniformMat4;
	UniformInt_t *UniformInt;
	RenderIndexedMesh_t *RenderIndexedMesh;
	RenderMesh_t *RenderMesh;
	RenderMeshInstanced_t *RenderMeshInstanced;
	RenderIndexedMeshInstanced_t *RenderIndexedMeshInstanced;
	RenderLines_t *RenderLines;
	CreateDeviceMesh_t *CreateDeviceMesh;
	CreateDeviceIndexedMesh_t *CreateDeviceIndexedMesh;
	CreateDeviceTexture_t *CreateDeviceTexture;
	SendMesh_t *SendMesh;
	SendIndexedMesh_t *SendIndexedMesh;
	SendTexture_t *SendTexture;
	BindTexture_t *BindTexture;
	CreateShader_t *CreateShader;
	LoadShader_t *LoadShader;
	AttachShader_t *AttachShader;
	CreateDeviceProgram_t *CreateDeviceProgram;
	LinkDeviceProgram_t *LinkDeviceProgram;
	SetViewport_t *SetViewport;
	SetFillMode_t *SetFillMode;

	ResourceLoadMesh_t *ResourceLoadMesh;
	ResourceLoadSkinnedMesh_t *ResourceLoadSkinnedMesh;
	ResourceLoadLevelGeometryGrid_t *ResourceLoadLevelGeometryGrid;
	ResourceLoadPoints_t *ResourceLoadPoints;
	ResourceLoadShader_t *ResourceLoadShader;
	ResourceLoadTexture_t *ResourceLoadTexture;
	GetResource_t *GetResource;
};

#define START_GAME(name) void name(Memory *memory, PlatformCode *platformCode)
typedef START_GAME(StartGame_t);
START_GAME(StartGameStub) { (void) memory, platformCode; }

#define UPDATE_AND_RENDER_GAME(name) void name(Controller *controller, Memory *memory, \
		PlatformCode *platformCode, f32 deltaTime)
typedef UPDATE_AND_RENDER_GAME(UpdateAndRenderGame_t);
UPDATE_AND_RENDER_GAME(UpdateAndRenderGameStub) { (void) controller, memory, platformCode, deltaTime; }
