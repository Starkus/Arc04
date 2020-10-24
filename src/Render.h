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

