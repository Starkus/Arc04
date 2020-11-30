struct GLDeviceMesh
{
	u32 vertexCount;
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

struct GLDeviceTexture
{
	GLuint texture;
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

struct GLDeviceFrameBuffer
{
	GLuint frameBuffer;
};
static_assert(sizeof(GLDeviceFrameBuffer) <= sizeof(DeviceFrameBuffer),
		"Size of GLDeviceFrameBuffer greater than handle");

PLATFORMPROC void SetUpDevice()
{
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);

	glEnable(GL_DEPTH_TEST);
}

PLATFORMPROC void EnableDepthTest()
{
	glEnable(GL_DEPTH_TEST);
}

PLATFORMPROC void DisableDepthTest()
{
	glDisable(GL_DEPTH_TEST);
}

PLATFORMPROC void EnableDepthWriting()
{
	glDepthMask(true);
}

PLATFORMPROC void DisableDepthWriting()
{
	glDepthMask(false);
}

PLATFORMPROC void EnableAlphaBlending()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

PLATFORMPROC void DisableAlphaBlending()
{
	glDisable(GL_BLEND);
}

PLATFORMPROC void ClearColorBuffer(v4 clearColor)
{
	glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
	glClear(GL_COLOR_BUFFER_BIT);
}

PLATFORMPROC void ClearDepthBuffer()
{
	glClear(GL_DEPTH_BUFFER_BIT);
}

PLATFORMPROC DeviceUniform GetUniform(DeviceProgram program, const char *name)
{
	GLDeviceProgram *glProgram = (GLDeviceProgram *)&program;

	DeviceUniform result;
	GLDeviceUniform *glUniform = (GLDeviceUniform *)&result;

	glUniform->location = glGetUniformLocation(glProgram->program, name);
	return result;
}

PLATFORMPROC void UseProgram(DeviceProgram program)
{
	GLDeviceProgram *glProgram = (GLDeviceProgram *)&program;
	glUseProgram(glProgram->program);
}

PLATFORMPROC void UniformMat4Array(DeviceUniform uniform, u32 count, const f32 *buffer)
{
	GLDeviceUniform *glUniform = (GLDeviceUniform *)&uniform;
	glUniformMatrix4fv(glUniform->location, count, false, buffer);
}

PLATFORMPROC void UniformV3Array(DeviceUniform uniform, u32 count, const f32 *buffer)
{
	GLDeviceUniform *glUniform = (GLDeviceUniform *)&uniform;
	glUniform3fv(glUniform->location, count, buffer);
}

PLATFORMPROC void UniformV4Array(DeviceUniform uniform, u32 count, const f32 *buffer)
{
	GLDeviceUniform *glUniform = (GLDeviceUniform *)&uniform;
	glUniform4fv(glUniform->location, count, buffer);
}

PLATFORMPROC void UniformInt(DeviceUniform uniform, int n)
{
	GLDeviceUniform *glUniform = (GLDeviceUniform *)&uniform;
	glUniform1i(glUniform->location, n);
}

PLATFORMPROC void UniformFloat(DeviceUniform uniform, f32 n)
{
	GLDeviceUniform *glUniform = (GLDeviceUniform *)&uniform;
	glUniform1f(glUniform->location, n);
}

PLATFORMPROC void UniformV3(DeviceUniform uniform, v3 v)
{
	GLDeviceUniform *glUniform = (GLDeviceUniform *)&uniform;
	glUniform3f(glUniform->location, v.x, v.y, v.z);
}

PLATFORMPROC void UniformV4(DeviceUniform uniform, v4 v)
{
	GLDeviceUniform *glUniform = (GLDeviceUniform *)&uniform;
	glUniform4f(glUniform->location, v.x, v.y, v.z, v.w);
}

PLATFORMPROC void RenderIndexedMesh(DeviceMesh mesh)
{
	GLDeviceMesh *glMesh = (GLDeviceMesh *)&mesh;
	glBindVertexArray(glMesh->vao);
	glDrawElements(GL_TRIANGLES, glMesh->indexCount, GL_UNSIGNED_SHORT, NULL);
}

PLATFORMPROC void RenderMesh(DeviceMesh mesh)
{
	GLDeviceMesh *glMesh = (GLDeviceMesh *)&mesh;
	glBindVertexArray(glMesh->vao);
	glDrawArrays(GL_TRIANGLES, 0, glMesh->vertexCount);
}

PLATFORMPROC void RenderLines(DeviceMesh mesh)
{
	GLDeviceMesh *glMesh = (GLDeviceMesh *)&mesh;
	glBindVertexArray(glMesh->vao);
	glDrawArrays(GL_LINES, 0, glMesh->vertexCount);
}

int GLEnableAttribs(u32 attribs, int first = 0)
{
	int stride = 0;
	if (attribs & RENDERATTRIB_POSITION)	stride += sizeof(v3);
	if (attribs & RENDERATTRIB_UV)			stride += sizeof(v2);
	if (attribs & RENDERATTRIB_NORMAL)		stride += sizeof(v3);
	if (attribs & RENDERATTRIB_INDICES)		stride += sizeof(u16) * 4;
	if (attribs & RENDERATTRIB_WEIGHTS)		stride += sizeof(f32) * 4;
	if (attribs & RENDERATTRIB_COLOR3)		stride += sizeof(v3);
	if (attribs & RENDERATTRIB_COLOR4)		stride += sizeof(v4);
	if (attribs & RENDERATTRIB_VERTEXNUM)	stride += sizeof(u8);

	for (int i = 0; i < 4; ++i)
		if (attribs & (RENDERATTRIB_1CUSTOMV3 << i)) stride += sizeof(v3);

	for (int i = 0; i < 4; ++i)
		if (attribs & (RENDERATTRIB_1CUSTOMF32 << i)) stride += sizeof(f32);

	int attribIdx = first;
	u64 offset = 0;
	// Position
	if (attribs & RENDERATTRIB_POSITION)
	{
		glVertexAttribPointer(attribIdx, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid *)offset);
		glEnableVertexAttribArray(attribIdx);
		++attribIdx;
		offset += sizeof(v3);
	}
	// UV
	if (attribs & RENDERATTRIB_UV)
	{
		glVertexAttribPointer(attribIdx, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid *)offset);
		glEnableVertexAttribArray(attribIdx);
		++attribIdx;
		offset += sizeof(v2);
	}
	// Normal
	if (attribs & RENDERATTRIB_NORMAL)
	{
		glVertexAttribPointer(attribIdx, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid *)offset);
		glEnableVertexAttribArray(attribIdx);
		++attribIdx;
		offset += sizeof(v3);
	}
	// Joint indices
	if (attribs & RENDERATTRIB_INDICES)
	{
		glVertexAttribIPointer(attribIdx, 4, GL_UNSIGNED_SHORT, stride, (GLvoid *)offset);
		glEnableVertexAttribArray(attribIdx);
		++attribIdx;
		offset += sizeof(u16) * 4;
	}
	// Joint weights
	if (attribs & RENDERATTRIB_WEIGHTS)
	{
		glVertexAttribPointer(attribIdx, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid *)offset);
		glEnableVertexAttribArray(attribIdx);
		++attribIdx;
		offset += sizeof(f32) * 4;
	}
	// Color RGB
	if (attribs & RENDERATTRIB_COLOR3)
	{
		glVertexAttribPointer(attribIdx, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid *)offset);
		glEnableVertexAttribArray(attribIdx);
		++attribIdx;
		offset += sizeof(v3);
	}
	// Color RGBA
	if (attribs & RENDERATTRIB_COLOR4)
	{
		glVertexAttribPointer(attribIdx, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid *)offset);
		glEnableVertexAttribArray(attribIdx);
		++attribIdx;
		offset += sizeof(v4);
	}
	// Vertex number
	if (attribs & RENDERATTRIB_VERTEXNUM)
	{
		glVertexAttribIPointer(attribIdx, 1, GL_UNSIGNED_BYTE, stride, (GLvoid *)offset);
		glEnableVertexAttribArray(attribIdx);
		++attribIdx;
		offset += sizeof(u8) * 1;
	}

	// Custom V3s
	for (int i = 0; i < 4; ++i)
	{
		if (attribs & (RENDERATTRIB_1CUSTOMV3 << i))
		{
			glVertexAttribPointer(attribIdx, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid *)offset);
			glEnableVertexAttribArray(attribIdx);
			++attribIdx;
			offset += sizeof(v3);
		}
	}

	// Custom F32s
	for (int i = 0; i < 4; ++i)
	{
		if (attribs & (RENDERATTRIB_1CUSTOMF32 << i))
		{
			glVertexAttribPointer(attribIdx, 1, GL_FLOAT, GL_FALSE, stride, (GLvoid *)offset);
			glEnableVertexAttribArray(attribIdx);
			++attribIdx;
			offset += sizeof(f32);
		}
	}
	return attribIdx;
}

PLATFORMPROC void RenderMeshInstanced(DeviceMesh mesh, DeviceMesh positions, u32 meshAttribs,
		u32 instAttribs)
{
	GLDeviceMesh *glMesh = (GLDeviceMesh *)&mesh;
	GLDeviceMesh *glPositions = (GLDeviceMesh *)&positions;

	glBindVertexArray(glPositions->vao);

	glBindBuffer(GL_ARRAY_BUFFER, glMesh->vertexBuffer);
	int firstInstAttrib = GLEnableAttribs(meshAttribs);

	glBindBuffer(GL_ARRAY_BUFFER, glPositions->vertexBuffer);
	int attribSize = GLEnableAttribs(instAttribs, firstInstAttrib);

	for (int attribIdx = 0; attribIdx < attribSize; ++attribIdx)
		glEnableVertexAttribArray(attribIdx);

	for (int attribIdx = 0; attribIdx < firstInstAttrib; ++attribIdx)
		glVertexAttribDivisor(attribIdx, 0);
	for (int attribIdx = firstInstAttrib; attribIdx < attribSize; ++attribIdx)
		glVertexAttribDivisor(attribIdx, 1);

	glDrawArraysInstanced(GL_TRIANGLES, 0, glMesh->vertexCount, glPositions->vertexCount);
}

PLATFORMPROC void RenderIndexedMeshInstanced(DeviceMesh mesh, DeviceMesh positions, u32 meshAttribs,
		u32 instAttribs)
{
	GLDeviceMesh *glMesh = (GLDeviceMesh *)&mesh;
	GLDeviceMesh *glPositions = (GLDeviceMesh *)&positions;

	glBindVertexArray(glPositions->vao);

	glBindBuffer(GL_ARRAY_BUFFER, glMesh->vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glMesh->indexBuffer);
	int firstInstAttrib = GLEnableAttribs(meshAttribs);

	glBindBuffer(GL_ARRAY_BUFFER, glPositions->vertexBuffer);
	int attribSize = GLEnableAttribs(instAttribs, firstInstAttrib);

	for (int attribIdx = 0; attribIdx < attribSize; ++attribIdx)
		glEnableVertexAttribArray(attribIdx);

	for (int attribIdx = 0; attribIdx < firstInstAttrib; ++attribIdx)
		glVertexAttribDivisor(attribIdx, 0);
	for (int attribIdx = firstInstAttrib; attribIdx < attribSize; ++attribIdx)
		glVertexAttribDivisor(attribIdx, 1);

	glDrawElementsInstanced(GL_TRIANGLES, glMesh->indexCount, GL_UNSIGNED_SHORT, NULL, glPositions->vertexCount);
}

PLATFORMPROC DeviceMesh CreateDeviceMesh(int attribs)
{
	DeviceMesh result;
	GLDeviceMesh *glMesh = (GLDeviceMesh *)&result;

	glGenVertexArrays(1, &glMesh->vao);
	glBindVertexArray(glMesh->vao);
	glGenBuffers(1, glMesh->buffers);
	glBindBuffer(GL_ARRAY_BUFFER, glMesh->vertexBuffer);

	GLEnableAttribs(attribs);

	return result;
}

PLATFORMPROC void DestroyDeviceMesh(DeviceMesh mesh)
{
	GLDeviceMesh *glMesh = (GLDeviceMesh *)&mesh;
	glDeleteBuffers(1, glMesh->buffers);
	glDeleteVertexArrays(1, &glMesh->vao);
}

PLATFORMPROC DeviceMesh CreateDeviceIndexedMesh(int attribs)
{
	DeviceMesh result;
	GLDeviceMesh *glMesh = (GLDeviceMesh *)&result;

	glGenVertexArrays(1, &glMesh->vao);
	glBindVertexArray(glMesh->vao);
	glGenBuffers(2, glMesh->buffers);
	glBindBuffer(GL_ARRAY_BUFFER, glMesh->vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glMesh->indexBuffer);

	GLEnableAttribs(attribs);

	return result;
}

PLATFORMPROC void DestroyDeviceIndexedMesh(DeviceMesh mesh)
{
	GLDeviceMesh *glMesh = (GLDeviceMesh *)&mesh;
	glDeleteBuffers(2, glMesh->buffers);
	glDeleteVertexArrays(1, &glMesh->vao);
}

PLATFORMPROC DeviceTexture CreateDeviceTexture()
{
	DeviceTexture result;
	GLDeviceTexture *glTexture = (GLDeviceTexture *)&result;
	glGenTextures(1, &glTexture->texture);

	return result;
}

PLATFORMPROC void SendMesh(DeviceMesh *mesh, void *vertexData, u32 vertexCount, u32 stride, bool dynamic)
{
	GLDeviceMesh *glMesh = (GLDeviceMesh *)mesh;

	glBindVertexArray(glMesh->vao);

	glMesh->vertexCount = vertexCount;
	glBindBuffer(GL_ARRAY_BUFFER, glMesh->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertexCount * stride, vertexData,
			dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
}

PLATFORMPROC void SendIndexedMesh(DeviceMesh *mesh, void *vertexData, u32 vertexCount, u32 stride,
		void *indexData, u32 indexCount, bool dynamic)
{
	GLDeviceMesh *glMesh = (GLDeviceMesh *)mesh;

	glMesh->vertexCount = vertexCount;
	glMesh->indexCount = indexCount;
	glBindVertexArray(glMesh->vao);

	glBindBuffer(GL_ARRAY_BUFFER, glMesh->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, stride * vertexCount, vertexData,
			dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glMesh->indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u16) * indexCount, indexData,
			dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
}

PLATFORMPROC void SendTexture(DeviceTexture texture, const void *imageData, u32 width, u32 height,
		RenderImageComponents components)
{
	GLDeviceTexture *glTexture = (GLDeviceTexture *)&texture;
	glBindTexture(GL_TEXTURE_2D, glTexture->texture);
	GLenum mode = GL_RGB8;
	GLenum format = GL_RGB;
	switch (components)
	{
	case RENDERIMAGECOMPONENTS_1:
		mode = GL_R8;
		format = GL_RED;
		break;
	case RENDERIMAGECOMPONENTS_2:
		mode = GL_RG8;
		format = GL_RG;
		break;
	case RENDERIMAGECOMPONENTS_3:
		mode = GL_RGB8;
		format = GL_RGB;
		break;
	case RENDERIMAGECOMPONENTS_4:
		mode = GL_RGBA8;
		format = GL_RGBA;
		break;
	case RENDERIMAGECOMPONENTS_DEPTH24:
		mode = GL_DEPTH_COMPONENT24;
		format = GL_DEPTH_COMPONENT;
		break;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, mode, width, height, 0, format, GL_UNSIGNED_BYTE, imageData);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

PLATFORMPROC void BindTexture(DeviceTexture texture, int slot)
{
	GLDeviceTexture *glTexture = (GLDeviceTexture *)&texture;

	const GLenum glSlots[] =
	{
		GL_TEXTURE0,
		GL_TEXTURE1,
		GL_TEXTURE2,
		GL_TEXTURE3
	};

	glActiveTexture(glSlots[slot]);
	glBindTexture(GL_TEXTURE_2D, glTexture->texture);
}

PLATFORMPROC DeviceShader CreateShader(ShaderType shaderType)
{
	DeviceShader result;
	GLDeviceShader *glShader = (GLDeviceShader *)&result;

	GLuint glType = (GLuint)-1;
	if (shaderType == SHADERTYPE_VERTEX)
		glType = GL_VERTEX_SHADER;
	else if (shaderType == SHADERTYPE_FRAGMENT)
		glType = GL_FRAGMENT_SHADER;

	GLuint shader = glCreateShader(glType);
	glShader->shader = shader;

	return result;
}

PLATFORMPROC bool LoadShader(DeviceShader *shader, const char *shaderSource)
{
	GLDeviceShader *glShader = (GLDeviceShader *)shader;

	glShaderSource(glShader->shader, 1, &shaderSource, nullptr);
	glCompileShader(glShader->shader);

#if defined(DEBUG_BUILD)
	{
		GLint status;
		glGetShaderiv(glShader->shader, GL_COMPILE_STATUS, &status);
		if (status != GL_TRUE)
		{
			char msg[256];
			GLsizei len;
			glGetShaderInfoLog(glShader->shader, sizeof(msg), &len, msg);
			Log("Error compiling shader: %s", msg);

			return false;
		}
	}
#endif

	return true;
}

PLATFORMPROC void AttachShader(DeviceProgram program, DeviceShader shader)
{
	GLDeviceProgram *glProgram = (GLDeviceProgram *)&program;
	GLDeviceShader *glShader = (GLDeviceShader *)&shader;
	glAttachShader(glProgram->program, glShader->shader);
}

PLATFORMPROC DeviceProgram CreateDeviceProgram()
{
	DeviceProgram result;
	GLDeviceProgram *glProgram = (GLDeviceProgram *)&result;

	//GLDeviceShader *glVertexShader = (GLDeviceShader *)&vertexShader;
	//GLDeviceShader *glFragmentShader = (GLDeviceShader *)&fragmentShader;

	glProgram->program = glCreateProgram();
	//glAttachShader(glProgram->program, glVertexShader->shader);
	//glAttachShader(glProgram->program, glFragmentShader->shader);

	return result;
}

PLATFORMPROC bool LinkDeviceProgram(DeviceProgram program)
{
	GLDeviceProgram *glProgram = (GLDeviceProgram *)&program;

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

			return false;
		}
	}
#endif
	return true;
}

PLATFORMPROC void WipeDeviceProgram(DeviceProgram program)
{
	GLDeviceProgram *glProgram = (GLDeviceProgram *)&program;

	GLsizei count;
	GLuint shaders[16];
	glGetAttachedShaders(glProgram->program, 16, &count, shaders);

	for (GLsizei i = 0; i < count; ++i)
	{
		glDetachShader(glProgram->program, shaders[i]);
		glDeleteShader(shaders[i]);
	}
}

PLATFORMPROC void SetFillMode(RenderFillMode mode)
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

PLATFORMPROC void SetViewport(int posX, int posY, int width, int height)
{
	glViewport(posX, posY, width, height);
}

PLATFORMPROC DeviceFrameBuffer CreateDeviceFrameBuffer(DeviceTexture colorTex,
		DeviceTexture depthTex)
{
	DeviceFrameBuffer result;
	GLDeviceFrameBuffer *glBuffer = (GLDeviceFrameBuffer *)&result;
	GLDeviceTexture *glColorTex = (GLDeviceTexture *)&colorTex;
	GLDeviceTexture *glDepthTex = (GLDeviceTexture *)&depthTex;

	glGenFramebuffers(1, &glBuffer->frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, glBuffer->frameBuffer);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, glColorTex->texture, 0);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, glDepthTex->texture, 0);

	GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &drawBuffer);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		Log("ERROR! Creating framebuffer! glCheckFramebufferStatus returned 0x%X\n", status);
	}

	return result;
}

PLATFORMPROC void BindFrameBuffer(DeviceFrameBuffer buffer)
{
	GLDeviceFrameBuffer *glBuffer = (GLDeviceFrameBuffer *)&buffer;
	glBindFramebuffer(GL_FRAMEBUFFER, glBuffer->frameBuffer);
}

PLATFORMPROC void UnbindFrameBuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
