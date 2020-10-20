void SetUpDevice()
{
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);

	glEnable(GL_DEPTH_TEST);
}

void ClearBuffers(v4 clearColor)
{
	glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

DeviceUniform GetUniform(DeviceProgram *program, const char *name)
{
	DeviceUniform result;
	result.location = glGetUniformLocation(program->program, name);
	return result;
}

void UseProgram(DeviceProgram *program)
{
	glUseProgram(program->program);
}

void UniformMat4(DeviceUniform *uniform, u32 count, const f32 *buffer)
{
	glUniformMatrix4fv(uniform->location, count, false, buffer);
}

void RenderIndexedMesh(DeviceMesh *mesh)
{
	glBindVertexArray(mesh->vao);
	glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_SHORT, NULL);
}

void RenderMesh(DeviceMesh *mesh)
{
	glBindVertexArray(mesh->vao);
	glDrawArrays(GL_TRIANGLES, 0, mesh->vertexCount);
}

DeviceMesh CreateDeviceMesh()
{
	DeviceMesh result;
	glGenVertexArrays(1, &result.vao);
	glBindVertexArray(result.vao);
	glGenBuffers(2, result.buffers);
	return result;
}

DeviceMesh CreateDeviceIndexedMesh()
{
	DeviceMesh result;
	glGenVertexArrays(1, &result.vao);
	glBindVertexArray(result.vao);
	glGenBuffers(1, result.buffers);
	return result;
}

void SendMesh(DeviceMesh *mesh, void *vertexData, u32 vertexCount, bool dynamic)
{
	glBindVertexArray(mesh->vao);

	mesh->vertexCount = vertexCount;
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertexData,
			dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

	// @Cleanup: can this go in CreateDeviceMesh? Test once everything works.
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, uv));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, nor));
	glEnableVertexAttribArray(2);
}

void SendIndexedMesh(DeviceMesh *mesh, void *vertexData, u32 vertexCount, void *indexData,
		u32 indexCount, bool dynamic)
{
	mesh->vertexCount = vertexCount; // @Cleanup: can we remove?
	mesh->indexCount = indexCount;
	glBindVertexArray(mesh->vao);

	glGenBuffers(2, mesh->buffers);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertexCount, vertexData,
			dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u16) * indexCount, indexData,
			dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

	// @Cleanup: can this go in CreateDeviceMesh? Test once everything works.
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, uv));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, nor));
	glEnableVertexAttribArray(2);
}

void SendIndexedSkinnedMesh(DeviceMesh *mesh, void *vertexData, u32 vertexCount, void *indexData,
		u32 indexCount)
{
	mesh->vertexCount = vertexCount; // @Cleanup: can we remove?
	mesh->indexCount = indexCount;
	glBindVertexArray(mesh->vao);

	glGenBuffers(2, mesh->buffers);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(SkinnedVertex) * vertexCount, vertexData, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u16) * indexCount, indexData, GL_STATIC_DRAW);

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
}

DeviceShader LoadShader(const GLchar *shaderSource, ShaderType shaderType)
{
	DeviceShader result;

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

	result.shader = shader;
	return result;
}

DeviceProgram CreateDeviceProgram(DeviceShader *vertexShader, DeviceShader *fragmentShader)
{
	DeviceProgram result;

	result.program = glCreateProgram();
	glAttachShader(result.program, vertexShader->shader);
	glAttachShader(result.program, fragmentShader->shader);
	glLinkProgram(result.program);
#if defined(DEBUG_BUILD)
	{
		GLint status;
		glGetProgramiv(result.program, GL_LINK_STATUS, &status);
		if (status != GL_TRUE)
		{
			char msg[256];
			GLsizei len;
			glGetProgramInfoLog(result.program, sizeof(msg), &len, msg);
			Log("Error linking shader program: %s", msg);
		}
	}
#endif
	return result;
}
