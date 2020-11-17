void *GL_GetProcAddress(const char *name)
{
	void *p = (void *)wglGetProcAddress(name);
	if(p == 0 ||
		(p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) ||
		(p == (void*)-1) )
	{
		HMODULE module = LoadLibraryA("opengl32.dll");
		p = (void *)GetProcAddress(module, name);
	}

	return p;
}

void LoadOpenGLProcs()
{
#define GL_GetProc(name) name = ((name##Proc) GL_GetProcAddress(#name))
	GL_GetProc(glGetError);
	GL_GetProc(glGetIntegerv);
	GL_GetProc(glClearColor);
	GL_GetProc(glClear);
	GL_GetProc(glViewport);
	GL_GetProc(glEnable);
	GL_GetProc(glDisable);
	GL_GetProc(glFrontFace);
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
	GL_GetProc(glBindSampler);
	GL_GetProc(glTexImage2D);
	GL_GetProc(glGenerateMipmap);
	GL_GetProc(glTexParameteri);
	GL_GetProc(glGenRenderbuffers);
	GL_GetProc(glBindRenderbuffer);
	GL_GetProc(glRenderbufferStorage);
	GL_GetProc(glCreateShader);
	GL_GetProc(glDetachShader);
	GL_GetProc(glDeleteShader);
	GL_GetProc(glShaderSource);
	GL_GetProc(glCompileShader);
	GL_GetProc(glGetShaderiv);
	GL_GetProc(glGetShaderInfoLog);
	GL_GetProc(glCreateProgram);
	GL_GetProc(glDeleteProgram);
	GL_GetProc(glAttachShader);
	GL_GetProc(glLinkProgram);
	GL_GetProc(glGetAttachedShaders);
	GL_GetProc(glGetProgramiv);
	GL_GetProc(glGetProgramInfoLog);
	GL_GetProc(glUseProgram);
	GL_GetProc(glBindAttribLocation);
	GL_GetProc(glGetUniformLocation);
	GL_GetProc(glGetAttribLocation);
	GL_GetProc(glUniformMatrix4fv);
	GL_GetProc(glUniform1i);
	GL_GetProc(glUniform1f);
	GL_GetProc(glUniform2f);
	GL_GetProc(glUniform3f);
	GL_GetProc(glUniform4f);
	GL_GetProc(glUniform1fv);
	GL_GetProc(glUniform2fv);
	GL_GetProc(glUniform3fv);
	GL_GetProc(glUniform4fv);
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
	GL_GetProc(glVertexAttribDivisor);
	GL_GetProc(glEnableVertexAttribArray);
	GL_GetProc(glDrawArrays);
	GL_GetProc(glDrawArraysInstanced);
	GL_GetProc(glDrawElements);
	GL_GetProc(glDrawElementsInstanced);
	GL_GetProc(glDrawElementsBaseVertex);
	GL_GetProc(glBlendEquation);
	GL_GetProc(glBlendEquationSeparate);
#undef GL_GetProc
}
