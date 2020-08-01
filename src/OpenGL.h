typedef GLenum (GLAPIENTRY *glGetErrorProc)();
typedef void (GLAPIENTRY *glGetIntegervProc)(GLenum pname, GLint *params);

typedef void (GLAPIENTRY *glClearColorProc)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (GLAPIENTRY *glClearProc)(GLbitfield mask);

typedef void (GLAPIENTRY *glViewportProc)(GLint x, GLint y, GLsizei width, GLsizei height);

typedef void (GLAPIENTRY *glEnableProc)(GLenum cap);
typedef void (GLAPIENTRY *glDisableProc)(GLenum cap);

typedef void (GLAPIENTRY *glDepthMaskProc)(GLboolean flag);
typedef void (GLAPIENTRY *glDepthFuncProc)(GLenum func);

typedef void (GLAPIENTRY *glBlendFuncProc)(GLenum sfactor, GLenum dfactor);
typedef void (GLAPIENTRY *glBlendFuncSeparateProc)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);

typedef void (GLAPIENTRY *glGenFramebuffersProc)(GLsizei n, GLuint *ids);
typedef void (GLAPIENTRY *glBindFramebufferProc)(GLenum target, GLuint framebuffer);
typedef void (GLAPIENTRY *glFramebufferTexture2DProc)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (GLAPIENTRY *glFramebufferRenderbufferProc)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (GLAPIENTRY *glDrawBuffersProc)(GLsizei n, const GLenum *bufs);
typedef GLenum (GLAPIENTRY *glCheckFramebufferStatusProc)(GLenum target);
typedef void (GLAPIENTRY *glBlitFramebufferProc)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

typedef void (GLAPIENTRY *glGenTexturesProc)(GLsizei n, GLuint *textures);
typedef void (GLAPIENTRY *glDeleteTexturesProc)(GLsizei n, GLuint *textures);
typedef void (GLAPIENTRY *glBindTextureProc)(GLenum target, GLuint texture);
typedef void (GLAPIENTRY *glTexImage2DProc)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *data);
typedef void (GLAPIENTRY *glGenerateMipmapProc)(GLenum target);
typedef void (GLAPIENTRY *glTexParameteriProc)(GLenum target, GLenum pname, GLint param);

typedef void (GLAPIENTRY *glGenRenderbuffersProc)(GLsizei n, GLuint *renderbuffers);
typedef void (GLAPIENTRY *glBindRenderbufferProc)(GLenum target, GLuint renderbuffer);
typedef void (GLAPIENTRY *glRenderbufferStorageProc)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);

typedef GLuint (GLAPIENTRY *glCreateShaderProc)(GLenum shaderType);
typedef void (GLAPIENTRY *glDeleteShaderProc)(GLuint shader);
typedef void (GLAPIENTRY *glShaderSourceProc)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
typedef void (GLAPIENTRY *glCompileShaderProc)(GLuint shader);
typedef void (GLAPIENTRY *glGetShaderivProc)(GLuint shader, GLenum pname, GLint *params);
typedef void (GLAPIENTRY *glGetShaderInfoLogProc)(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);

typedef GLuint (GLAPIENTRY *glCreateProgramProc)();
typedef void (GLAPIENTRY *glDeleteProgramProc)(GLuint program);
typedef void (GLAPIENTRY *glAttachShaderProc)(GLuint program, GLuint shader);
typedef void (GLAPIENTRY *glLinkProgramProc)(GLuint program);
typedef void (GLAPIENTRY *glGetProgramivProc)(GLuint program, GLenum pname, GLint *params);
typedef void (GLAPIENTRY *glGetProgramInfoLogProc)(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef void (GLAPIENTRY *glUseProgramProc)(GLuint program);
typedef void (GLAPIENTRY *glBindAttribLocationProc)(GLuint program, GLuint index, const GLchar *name);
typedef GLint (GLAPIENTRY *glGetUniformLocationProc)(GLuint program, const GLchar *name);
typedef void (GLAPIENTRY *glUniform1iProc)(GLint location, GLint v0);
typedef void (GLAPIENTRY *glUniform1fProc)(GLint location, GLfloat v0);
typedef void (GLAPIENTRY *glUniform2fProc)(GLint location, GLfloat v0, GLfloat v1);
typedef void (GLAPIENTRY *glUniform3fProc)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (GLAPIENTRY *glUniform1fvProc)(GLint location, GLsizei count, const GLfloat *v);
typedef void (GLAPIENTRY *glUniform3fvProc)(GLint location, GLsizei count, const GLfloat *v);
typedef void (GLAPIENTRY *glUniformMatrix4fvProc)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (GLAPIENTRY *glActiveTextureProc)(GLenum texture);

typedef void (GLAPIENTRY *glGenBuffersProc)(GLsizei n, GLuint *buffers);
typedef void (GLAPIENTRY *glDeleteBuffersProc)(GLsizei n, GLuint *buffers);
typedef void (GLAPIENTRY *glBindBufferProc)(GLenum target, GLuint buffer);
typedef void (GLAPIENTRY *glBufferDataProc)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
typedef void (GLAPIENTRY *glBufferSubDataProc)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);

typedef void (GLAPIENTRY *glGenVertexArraysProc)(GLsizei n, GLuint *arrays);
typedef void (GLAPIENTRY *glDeleteVertexArraysProc)(GLsizei n, GLuint *arrays);
typedef void (GLAPIENTRY *glBindVertexArrayProc)(GLuint array);
typedef void (GLAPIENTRY *glVertexAttribPointerProc)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
typedef void (GLAPIENTRY *glVertexAttribIPointerProc)(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
typedef void (GLAPIENTRY *glEnableVertexAttribArrayProc)(GLuint index);

typedef void (GLAPIENTRY *glDrawArraysProc)(GLenum mode, GLint first, GLsizei count);
typedef void (GLAPIENTRY *glDrawElementsProc)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);

typedef void (GLAPIENTRY *glDebugOutputProc)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
typedef void (GLAPIENTRY *glDebugMessageCallbackProc)(glDebugOutputProc callback, void *userParam);
typedef void (GLAPIENTRY *glDebugMessageControlProc)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);

#define GL_DeclareProc(name) name##Proc name
GL_DeclareProc(glGetError);
GL_DeclareProc(glGetIntegerv);
GL_DeclareProc(glClearColor);
GL_DeclareProc(glClear);
GL_DeclareProc(glViewport);
GL_DeclareProc(glEnable);
GL_DeclareProc(glDisable);
GL_DeclareProc(glDepthMask);
GL_DeclareProc(glDepthFunc);
GL_DeclareProc(glBlendFunc);
GL_DeclareProc(glBlendFuncSeparate);
GL_DeclareProc(glGenFramebuffers);
GL_DeclareProc(glBindFramebuffer);
GL_DeclareProc(glFramebufferTexture2D);
GL_DeclareProc(glFramebufferRenderbuffer);
GL_DeclareProc(glDrawBuffers);
GL_DeclareProc(glCheckFramebufferStatus);
GL_DeclareProc(glBlitFramebuffer);
GL_DeclareProc(glGenTextures);
GL_DeclareProc(glDeleteTextures);
GL_DeclareProc(glBindTexture);
GL_DeclareProc(glTexImage2D);
GL_DeclareProc(glGenerateMipmap);
GL_DeclareProc(glTexParameteri);
GL_DeclareProc(glGenRenderbuffers);
GL_DeclareProc(glBindRenderbuffer);
GL_DeclareProc(glRenderbufferStorage);
GL_DeclareProc(glCreateShader);
GL_DeclareProc(glDeleteShader);
GL_DeclareProc(glShaderSource);
GL_DeclareProc(glCompileShader);
GL_DeclareProc(glGetShaderiv);
GL_DeclareProc(glGetShaderInfoLog);
GL_DeclareProc(glCreateProgram);
GL_DeclareProc(glDeleteProgram);
GL_DeclareProc(glAttachShader);
GL_DeclareProc(glLinkProgram);
GL_DeclareProc(glGetProgramiv);
GL_DeclareProc(glGetProgramInfoLog);
GL_DeclareProc(glUseProgram);
GL_DeclareProc(glBindAttribLocation);
GL_DeclareProc(glGetUniformLocation);
GL_DeclareProc(glUniformMatrix4fv);
GL_DeclareProc(glUniform1i);
GL_DeclareProc(glUniform1f);
GL_DeclareProc(glUniform2f);
GL_DeclareProc(glUniform3f);
GL_DeclareProc(glUniform1fv);
GL_DeclareProc(glUniform3fv);
GL_DeclareProc(glActiveTexture);
GL_DeclareProc(glGenBuffers);
GL_DeclareProc(glDeleteBuffers);
GL_DeclareProc(glBindBuffer);
GL_DeclareProc(glBufferData);
GL_DeclareProc(glBufferSubData);
GL_DeclareProc(glGenVertexArrays);
GL_DeclareProc(glDeleteVertexArrays);
GL_DeclareProc(glBindVertexArray);
GL_DeclareProc(glVertexAttribPointer);
GL_DeclareProc(glVertexAttribIPointer);
GL_DeclareProc(glEnableVertexAttribArray);
GL_DeclareProc(glDrawArrays);
GL_DeclareProc(glDrawElements);
#undef GL_DeclareProc

void LoadOpenGLProcs()
{
#define GL_GetProc(name) name = ((name##Proc) SDL_GL_GetProcAddress(#name))
	GL_GetProc(glGetError);
	GL_GetProc(glGetIntegerv);
	GL_GetProc(glClearColor);
	GL_GetProc(glClear);
	GL_GetProc(glViewport);
	GL_GetProc(glEnable);
	GL_GetProc(glDisable);
	GL_GetProc(glDepthMask);
	GL_GetProc(glDepthFunc);
	GL_GetProc(glBlendFunc);
	GL_GetProc(glBlendFuncSeparate);
	GL_GetProc(glGenFramebuffers);
	GL_GetProc(glBindFramebuffer);
	GL_GetProc(glFramebufferTexture2D);
	GL_GetProc(glFramebufferRenderbuffer);
	GL_GetProc(glDrawBuffers);
	GL_GetProc(glCheckFramebufferStatus);
	GL_GetProc(glBlitFramebuffer);
	GL_GetProc(glGenTextures);
	GL_GetProc(glDeleteTextures);
	GL_GetProc(glBindTexture);
	GL_GetProc(glTexImage2D);
	GL_GetProc(glGenerateMipmap);
	GL_GetProc(glTexParameteri);
	GL_GetProc(glGenRenderbuffers);
	GL_GetProc(glBindRenderbuffer);
	GL_GetProc(glRenderbufferStorage);
	GL_GetProc(glCreateShader);
	GL_GetProc(glDeleteShader);
	GL_GetProc(glShaderSource);
	GL_GetProc(glCompileShader);
	GL_GetProc(glGetShaderiv);
	GL_GetProc(glGetShaderInfoLog);
	GL_GetProc(glCreateProgram);
	GL_GetProc(glDeleteProgram);
	GL_GetProc(glAttachShader);
	GL_GetProc(glLinkProgram);
	GL_GetProc(glGetProgramiv);
	GL_GetProc(glGetProgramInfoLog);
	GL_GetProc(glUseProgram);
	GL_GetProc(glBindAttribLocation);
	GL_GetProc(glGetUniformLocation);
	GL_GetProc(glUniformMatrix4fv);
	GL_GetProc(glUniform1i);
	GL_GetProc(glUniform1f);
	GL_GetProc(glUniform2f);
	GL_GetProc(glUniform3f);
	GL_GetProc(glUniform1fv);
	GL_GetProc(glUniform3fv);
	GL_GetProc(glActiveTexture);
	GL_GetProc(glGenBuffers);
	GL_GetProc(glDeleteBuffers);
	GL_GetProc(glBindBuffer);
	GL_GetProc(glBufferData);
	GL_GetProc(glBufferSubData);
	GL_GetProc(glGenVertexArrays);
	GL_GetProc(glDeleteVertexArrays);
	GL_GetProc(glBindVertexArray);
	GL_GetProc(glVertexAttribPointer);
	GL_GetProc(glVertexAttribIPointer);
	GL_GetProc(glEnableVertexAttribArray);
	GL_GetProc(glDrawArrays);
	GL_GetProc(glDrawElements);
#undef GL_GetProc
}
