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

// Important! whatever attribs you choose for your mesh, they have to be in the same order as this
// enum.
enum RenderBufferAttribs
{
	RENDERATTRIB_POSITION	= 0x0001,
	RENDERATTRIB_UV			= 0x0002,
	RENDERATTRIB_NORMAL		= 0x0004,
	RENDERATTRIB_INDICES	= 0x0008,
	RENDERATTRIB_WEIGHTS	= 0x0010,
	RENDERATTRIB_COLOR		= 0x0020,

	RENDERATTRIB_1CUSTOMV3	= 0x0100,
	RENDERATTRIB_2CUSTOMV3	= 0x0200,
	RENDERATTRIB_3CUSTOMV3	= 0x0400,
	RENDERATTRIB_4CUSTOMV3	= 0x0800,

	RENDERATTRIB_1CUSTOMF32	= 0x1000,
	RENDERATTRIB_2CUSTOMF32	= 0x2000,
	RENDERATTRIB_3CUSTOMF32	= 0x4000,
	RENDERATTRIB_4CUSTOMF32	= 0x8000
};

struct DeviceMesh
{
	u32 vertexCount; // @Cleanup: remove if unnecessary.
	u32 indexCount;
	u8 reserved[12];
};

struct DeviceTexture
{
	u8 reserved[4];
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

