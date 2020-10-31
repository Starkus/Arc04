enum ShaderType
{
	SHADERTYPE_VERTEX,
	SHADERTYPE_FRAGMENT
};

enum RenderFillMode
{
	RENDER_FILL,
	RENDER_LINE,
	RENDER_POINT
};

struct DeviceMesh
{
	u32 vertexCount; // @Cleanup: remove if unnecessary.
	u32 indexCount;
	u8 reserved[12];
};

struct DeviceShader
{
	u8 reserved[4];
};

struct DeviceProgram
{
	u8 reserved[4];
};

struct DeviceUniform
{
	u8 reserved[4];
};

