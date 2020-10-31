struct GLDeviceMesh
{
	u32 vertexCount; // @Cleanup: remove?
	u32 indexCount;
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
};
static_assert(sizeof(GLDeviceMesh) <= sizeof(DeviceMesh),
		"Size of GLDeviceMesh greater than handle");

struct GLDeviceShader
{
	GLuint shader;
};
static_assert(sizeof(GLDeviceShader) <= sizeof(DeviceShader),
		"Size of GLDeviceShader greater than handle");

struct GLDeviceProgram
{
	GLuint program;
};
static_assert(sizeof(GLDeviceProgram) <= sizeof(DeviceProgram),
		"Size of GLDeviceProgram greater than handle");

struct GLDeviceUniform
{
	GLuint location;
};
static_assert(sizeof(GLDeviceUniform) <= sizeof(DeviceUniform),
		"Size of GLDeviceUniform greater than handle");

void SetUpDevice()
{
	//glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);

	glEnable(GL_DEPTH_TEST);
}

void ClearBuffers(v4 clearColor)
{
	glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

DeviceUniform GetUniform(DeviceProgram program, const char *name)
{
	GLDeviceProgram *glProgram = (GLDeviceProgram *)&program;

	DeviceUniform result;
	GLDeviceUniform *glUniform = (GLDeviceUniform *)&result;

	glUniform->location = glGetUniformLocation(glProgram->program, name);
	return result;
}

void UseProgram(DeviceProgram program)
{
	GLDeviceProgram *glProgram = (GLDeviceProgram *)&program;
	glUseProgram(glProgram->program);
}

void UniformMat4(DeviceUniform uniform, u32 count, const f32 *buffer)
{
	GLDeviceUniform *glUniform = (GLDeviceUniform *)&uniform;
	glUniformMatrix4fv(glUniform->location, count, false, buffer);
}

void RenderIndexedMesh(DeviceMesh mesh)
{
	GLDeviceMesh *glMesh = (GLDeviceMesh *)&mesh;
	glBindVertexArray(glMesh->vao);
	glDrawElements(GL_TRIANGLES, glMesh->indexCount, GL_UNSIGNED_SHORT, NULL);
}

void RenderMesh(DeviceMesh mesh)
{
	GLDeviceMesh *glMesh = (GLDeviceMesh *)&mesh;
	glBindVertexArray(glMesh->vao);
	glDrawArrays(GL_TRIANGLES, 0, glMesh->vertexCount);
}

void RenderLines(DeviceMesh mesh)
{
	GLDeviceMesh *glMesh = (GLDeviceMesh *)&mesh;
	glBindVertexArray(glMesh->vao);
	glDrawArrays(GL_LINES, 0, glMesh->vertexCount);
}

DeviceMesh CreateDeviceMesh()
{
	DeviceMesh result;
	GLDeviceMesh *glMesh = (GLDeviceMesh *)&result;

	glGenVertexArrays(1, &glMesh->vao);
	glBindVertexArray(glMesh->vao);
	glGenBuffers(1, glMesh->buffers);
	glBindBuffer(GL_ARRAY_BUFFER, glMesh->vertexBuffer);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, uv));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, nor));
	glEnableVertexAttribArray(2);

	return result;
}

DeviceMesh CreateDeviceIndexedMesh()
{
	DeviceMesh result;
	GLDeviceMesh *glMesh = (GLDeviceMesh *)&result;

	glGenVertexArrays(1, &glMesh->vao);
	glBindVertexArray(glMesh->vao);
	glGenBuffers(2, glMesh->buffers);
	glBindBuffer(GL_ARRAY_BUFFER, glMesh->vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glMesh->indexBuffer);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, uv));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, nor));
	glEnableVertexAttribArray(2);

	return result;
}

DeviceMesh CreateDeviceIndexedSkinnedMesh()
{
	DeviceMesh result;
	GLDeviceMesh *glMesh = (GLDeviceMesh *)&result;

	glGenVertexArrays(1, &glMesh->vao);
	glBindVertexArray(glMesh->vao);
	glGenBuffers(2, glMesh->buffers);
	glBindBuffer(GL_ARRAY_BUFFER, glMesh->vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glMesh->indexBuffer);

	// Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
			(GLvoid *)offsetof(SkinnedVertex, pos));
	glEnableVertexAttribArray(0);
	// UV
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
			(GLvoid *)offsetof(SkinnedVertex, uv));
	glEnableVertexAttribArray(1);
	// Normal
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
			(GLvoid *)offsetof(SkinnedVertex, nor));
	glEnableVertexAttribArray(2);
	// Joint indices
	glVertexAttribIPointer(3, 4, GL_UNSIGNED_SHORT, sizeof(SkinnedVertex),
			(GLvoid *)offsetof(SkinnedVertex, indices));
	glEnableVertexAttribArray(3);
	// Joint weights
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
			(GLvoid *)offsetof(SkinnedVertex, weights));
	glEnableVertexAttribArray(4);

	return result;
}

void SendMesh(DeviceMesh *mesh, void *vertexData, u32 vertexCount, bool dynamic)
{
	GLDeviceMesh *glMesh = (GLDeviceMesh *)mesh;

	glBindVertexArray(glMesh->vao);

	glMesh->vertexCount = vertexCount;
	glBindBuffer(GL_ARRAY_BUFFER, glMesh->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertexData,
			dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
}

void SendIndexedMesh(DeviceMesh *mesh, void *vertexData, u32 vertexCount, void *indexData,
		u32 indexCount, bool dynamic)
{
	GLDeviceMesh *glMesh = (GLDeviceMesh *)mesh;

	glMesh->vertexCount = vertexCount; // @Cleanup: can we remove?
	glMesh->indexCount = indexCount;
	glBindVertexArray(glMesh->vao);

	glBindBuffer(GL_ARRAY_BUFFER, glMesh->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertexCount, vertexData,
			dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glMesh->indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u16) * indexCount, indexData,
			dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
}

void SendIndexedSkinnedMesh(DeviceMesh *mesh, void *vertexData, u32 vertexCount, void *indexData,
		u32 indexCount, bool dynamic)
{
	GLDeviceMesh *glMesh = (GLDeviceMesh *)mesh;

	glMesh->vertexCount = vertexCount; // @Cleanup: can we remove?
	glMesh->indexCount = indexCount;
	glBindVertexArray(glMesh->vao);

	glBindBuffer(GL_ARRAY_BUFFER, glMesh->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(SkinnedVertex) * vertexCount, vertexData,
			dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glMesh->indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u16) * indexCount, indexData,
			dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
}

DeviceShader LoadShader(const GLchar *shaderSource, ShaderType shaderType)
{
	DeviceShader result;
	GLDeviceShader *glShader = (GLDeviceShader *)&result;

	GLuint glType = (GLuint)-1;
	if (shaderType == SHADERTYPE_VERTEX)
		glType = GL_VERTEX_SHADER;
	else if (shaderType == SHADERTYPE_FRAGMENT)
		glType = GL_FRAGMENT_SHADER;
	GLuint shader = glCreateShader(glType);
	glShaderSource(shader, 1, &shaderSource, nullptr);
	glCompileShader(shader);

#if defined(DEBUG_BUILD)
	{
		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status != GL_TRUE)
		{
			char msg[256];
			GLsizei len;
			glGetShaderInfoLog(shader, sizeof(msg), &len, msg);
			Log("Error compiling shader: %s", msg);
		}
	}
#endif

	glShader->shader = shader;
	return result;
}

DeviceProgram CreateDeviceProgram(DeviceShader vertexShader, DeviceShader fragmentShader)
{
	DeviceProgram result;
	GLDeviceProgram *glProgram = (GLDeviceProgram *)&result;

	GLDeviceShader *glVertexShader = (GLDeviceShader *)&vertexShader;
	GLDeviceShader *glFragmentShader = (GLDeviceShader *)&fragmentShader;

	glProgram->program = glCreateProgram();
	glAttachShader(glProgram->program, glVertexShader->shader);
	glAttachShader(glProgram->program, glFragmentShader->shader);
	glLinkProgram(glProgram->program);
#if defined(DEBUG_BUILD)
	{
		GLint status;
		glGetProgramiv(glProgram->program, GL_LINK_STATUS, &status);
		if (status != GL_TRUE)
		{
			char msg[256];
			GLsizei len;
			glGetProgramInfoLog(glProgram->program, sizeof(msg), &len, msg);
			Log("Error linking shader program: %s", msg);
		}
	}
#endif
	return result;
}

void SetFillMode(RenderFillMode mode)
{
	switch(mode)
	{
		case RENDER_FILL:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		case RENDER_LINE:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case RENDER_POINT:
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
	}
}