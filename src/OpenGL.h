//#include <GL/gl.h>
#include <GLES3/gl32.h>
//#include <GLES3/glext.h>
//#include <GLES3/wglext.h>

#if defined(__MINGW32__) && defined(GL_NO_STDCALL) || defined(UNDER_CE)
#define GLAPIENTRY 
#else
#define GLAPIENTRY __stdcall
#endif

typedef GLenum (GLAPIENTRY *glGetErrorProc)();
typedef void (GLAPIENTRY *glGetIntegervProc)(GLenum pname, GLint *params);

typedef void (GLAPIENTRY *glClearColorProc)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (GLAPIENTRY *glClearProc)(GLbitfield mask);

typedef void (GLAPIENTRY *glViewportProc)(GLint x, GLint y, GLsizei width, GLsizei height);

typedef void (GLAPIENTRY *glEnableProc)(GLenum cap);
typedef void (GLAPIENTRY *glDisableProc)(GLenum cap);

typedef void (GLAPIENTRY *glFrontFaceProc)(GLenum mode);

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
typedef void (GLAPIENTRY *glBindSamplerProc)(GLenum target, GLuint sampler);
typedef void (GLAPIENTRY *glTexImage2DProc)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *data);
typedef void (GLAPIENTRY *glGenerateMipmapProc)(GLenum target);
typedef void (GLAPIENTRY *glTexParameteriProc)(GLenum target, GLenum pname, GLint param);

typedef void (GLAPIENTRY *glGenRenderbuffersProc)(GLsizei n, GLuint *renderbuffers);
typedef void (GLAPIENTRY *glBindRenderbufferProc)(GLenum target, GLuint renderbuffer);
typedef void (GLAPIENTRY *glRenderbufferStorageProc)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);

typedef GLuint (GLAPIENTRY *glCreateShaderProc)(GLenum shaderType);
typedef void (GLAPIENTRY *glDetachShaderProc)(GLuint program, GLuint shader);
typedef void (GLAPIENTRY *glDeleteShaderProc)(GLuint shader);
typedef void (GLAPIENTRY *glShaderSourceProc)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
typedef void (GLAPIENTRY *glCompileShaderProc)(GLuint shader);
typedef void (GLAPIENTRY *glGetShaderivProc)(GLuint shader, GLenum pname, GLint *params);
typedef void (GLAPIENTRY *glGetShaderInfoLogProc)(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);

typedef GLuint (GLAPIENTRY *glCreateProgramProc)();
typedef void (GLAPIENTRY *glDeleteProgramProc)(GLuint program);
typedef void (GLAPIENTRY *glAttachShaderProc)(GLuint program, GLuint shader);
typedef void (GLAPIENTRY *glLinkProgramProc)(GLuint program);
typedef void (GLAPIENTRY *glGetAttachedShadersProc)(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders);
typedef void (GLAPIENTRY *glGetProgramivProc)(GLuint program, GLenum pname, GLint *params);
typedef void (GLAPIENTRY *glGetProgramInfoLogProc)(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef void (GLAPIENTRY *glUseProgramProc)(GLuint program);
typedef void (GLAPIENTRY *glBindAttribLocationProc)(GLuint program, GLuint index, const GLchar *name);
typedef GLint (GLAPIENTRY *glGetUniformLocationProc)(GLuint program, const GLchar *name);
typedef GLint (GLAPIENTRY *glGetAttribLocationProc)(GLuint program, const GLchar *name);
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
typedef void (GLAPIENTRY *glVertexAttribDivisorProc)(GLuint index, GLuint divisor);
typedef void (GLAPIENTRY *glEnableVertexAttribArrayProc)(GLuint index);

typedef void (GLAPIENTRY *glDrawArraysProc)(GLenum mode, GLint first, GLsizei count);
typedef void (GLAPIENTRY *glDrawArraysInstancedProc)(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
typedef void (GLAPIENTRY *glDrawElementsProc)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
typedef void (GLAPIENTRY *glDrawElementsInstancedProc)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount);
typedef void (GLAPIENTRY *glDrawElementsBaseVertexProc)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLint basevertex);

typedef void (GLAPIENTRY *glBlendEquationProc)(GLenum mode);
typedef void (GLAPIENTRY *glBlendEquationSeparateProc)(GLenum modeRGB, GLenum modeAlpha);

typedef void (GLAPIENTRY *glDebugOutputProc)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
typedef void (GLAPIENTRY *glDebugMessageCallbackProc)(glDebugOutputProc callback, void *userParam);
typedef void (GLAPIENTRY *glDebugMessageControlProc)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);

#define GL_DeclareProc(name) name##Proc name##Pointer
GL_DeclareProc(glGetError);
GL_DeclareProc(glGetIntegerv);
GL_DeclareProc(glClearColor);
GL_DeclareProc(glClear);
GL_DeclareProc(glViewport);
GL_DeclareProc(glEnable);
GL_DeclareProc(glDisable);
GL_DeclareProc(glFrontFace);
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
GL_DeclareProc(glBindSampler);
GL_DeclareProc(glTexImage2D);
GL_DeclareProc(glGenerateMipmap);
GL_DeclareProc(glTexParameteri);
GL_DeclareProc(glGenRenderbuffers);
GL_DeclareProc(glBindRenderbuffer);
GL_DeclareProc(glRenderbufferStorage);
GL_DeclareProc(glCreateShader);
GL_DeclareProc(glDetachShader);
GL_DeclareProc(glDeleteShader);
GL_DeclareProc(glShaderSource);
GL_DeclareProc(glCompileShader);
GL_DeclareProc(glGetShaderiv);
GL_DeclareProc(glGetShaderInfoLog);
GL_DeclareProc(glCreateProgram);
GL_DeclareProc(glDeleteProgram);
GL_DeclareProc(glAttachShader);
GL_DeclareProc(glLinkProgram);
GL_DeclareProc(glGetAttachedShaders);
GL_DeclareProc(glGetProgramiv);
GL_DeclareProc(glGetProgramInfoLog);
GL_DeclareProc(glUseProgram);
GL_DeclareProc(glBindAttribLocation);
GL_DeclareProc(glGetUniformLocation);
GL_DeclareProc(glGetAttribLocation);
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
GL_DeclareProc(glVertexAttribDivisor);
GL_DeclareProc(glEnableVertexAttribArray);
GL_DeclareProc(glDrawArrays);
GL_DeclareProc(glDrawArraysInstanced);
GL_DeclareProc(glDrawElements);
GL_DeclareProc(glDrawElementsInstanced);
GL_DeclareProc(glDrawElementsBaseVertex);
GL_DeclareProc(glBlendEquation);
GL_DeclareProc(glBlendEquationSeparate);
#undef GL_DeclareProc

#define glGetError glGetErrorPointer
#define glGetIntegerv glGetIntegervPointer
#define glClearColor glClearColorPointer
#define glClear glClearPointer
#define glViewport glViewportPointer
#define glEnable glEnablePointer
#define glDisable glDisablePointer
#define glFrontFace glFrontFacePointer
#define glDepthMask glDepthMaskPointer
#define glDepthFunc glDepthFuncPointer
#define glBlendFunc glBlendFuncPointer
#define glBlendFuncSeparate glBlendFuncSeparatePointer
#define glGenFramebuffers glGenFramebuffersPointer
#define glBindFramebuffer glBindFramebufferPointer
#define glFramebufferTexture2D glFramebufferTexture2DPointer
#define glFramebufferRenderbuffer glFramebufferRenderbufferPointer
#define glDrawBuffers glDrawBuffersPointer
#define glCheckFramebufferStatus glCheckFramebufferStatusPointer
#define glBlitFramebuffer glBlitFramebufferPointer
#define glGenTextures glGenTexturesPointer
#define glDeleteTextures glDeleteTexturesPointer
#define glBindTexture glBindTexturePointer
#define glBindSampler glBindSamplerPointer
#define glTexImage2D glTexImage2DPointer
#define glGenerateMipmap glGenerateMipmapPointer
#define glTexParameteri glTexParameteriPointer
#define glGenRenderbuffers glGenRenderbuffersPointer
#define glBindRenderbuffer glBindRenderbufferPointer
#define glRenderbufferStorage glRenderbufferStoragePointer
#define glCreateShader glCreateShaderPointer
#define glDetachShader glDetachShaderPointer
#define glDeleteShader glDeleteShaderPointer
#define glShaderSource glShaderSourcePointer
#define glCompileShader glCompileShaderPointer
#define glGetShaderiv glGetShaderivPointer
#define glGetShaderInfoLog glGetShaderInfoLogPointer
#define glCreateProgram glCreateProgramPointer
#define glDeleteProgram glDeleteProgramPointer
#define glAttachShader glAttachShaderPointer
#define glLinkProgram glLinkProgramPointer
#define glGetAttachedShaders glGetAttachedShadersPointer
#define glGetProgramiv glGetProgramivPointer
#define glGetProgramInfoLog glGetProgramInfoLogPointer
#define glUseProgram glUseProgramPointer
#define glBindAttribLocation glBindAttribLocationPointer
#define glGetUniformLocation glGetUniformLocationPointer
#define glGetAttribLocation glGetAttribLocationPointer
#define glUniformMatrix4fv glUniformMatrix4fvPointer
#define glUniform1i glUniform1iPointer
#define glUniform1f glUniform1fPointer
#define glUniform2f glUniform2fPointer
#define glUniform3f glUniform3fPointer
#define glUniform1fv glUniform1fvPointer
#define glUniform3fv glUniform3fvPointer
#define glActiveTexture glActiveTexturePointer
#define glGenBuffers glGenBuffersPointer
#define glDeleteBuffers glDeleteBuffersPointer
#define glBindBuffer glBindBufferPointer
#define glBufferData glBufferDataPointer
#define glBufferSubData glBufferSubDataPointer
#define glGenVertexArrays glGenVertexArraysPointer
#define glDeleteVertexArrays glDeleteVertexArraysPointer
#define glBindVertexArray glBindVertexArrayPointer
#define glVertexAttribPointer glVertexAttribPointerPointer
#define glVertexAttribIPointer glVertexAttribIPointerPointer
#define glVertexAttribDivisor glVertexAttribDivisorPointer
#define glEnableVertexAttribArray glEnableVertexAttribArrayPointer
#define glDrawArrays glDrawArraysPointer
#define glDrawArraysInstanced glDrawArraysInstancedPointer
#define glDrawElements glDrawElementsPointer
#define glDrawElementsInstanced glDrawElementsInstancedPointer
#define glDrawElementsBaseVertex glDrawElementsBaseVertexPointer
#define glBlendEquation glBlendEquationPointer
#define glBlendEquationSeparate glBlendEquationSeparatePointer
