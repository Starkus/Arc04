#include "SDL/SDL.h"
#include "SDL/SDL_OpenGL.h"

#include "General.h"

namespace {
#include "OpenGL.h"

const GLchar *vertexShaderSource = "\
#version 330 core\n\
layout (location = 0) in vec3 pos;\n\
out vec3 vertexColor;\n\
\n\
void main()\n\
{\n\
	gl_Position = vec4(pos, 1.0);\n\
	vertexColor = pos;\n\
}\n\
";

const GLchar *fragShaderSource = "\
#version 330 core\n\
in vec3 vertexColor;\n\
out vec4 fragColor;\n\
\n\
void main()\n\
{\n\
	fragColor = vec4(vertexColor.x + 0.5, vertexColor.y + 0.5, 1.0, 1.0);\n\
}\n\
";

void StartGame()
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	i32 contextFlags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
	DEBUG_ONLY(contextFlags |= SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, contextFlags);

	SDL_Window *window = SDL_CreateWindow("GAME", 16, 16, 960, 540, SDL_WINDOW_OPENGL);
	if (window == nullptr)
		SDL_Log("No window!");

	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	if (glContext == nullptr)
		SDL_Log("No context!");

	LoadOpenGLProcs();

	GLuint vao;
	GLuint vertexBuffer;
	{
		f32 lovelyTriangle[] =
		{
			-0.5f, -0.5f, 0.0f,
			0.5f, -0.5f, 0.0f,
			0.0f, 0.5f, 0.0f
		};

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(lovelyTriangle), lovelyTriangle, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(f32) * 3, 0);

		glEnableVertexAttribArray(0);

		// Shader
		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
		glCompileShader(vertexShader);

#if defined(DEBUG_BUILD)
		{
			GLint status;
			glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
			if (status != GL_TRUE)
			{
				char msg[256];
				GLsizei len;
				glGetShaderInfoLog(vertexShader, sizeof(msg), &len, msg);
				SDL_Log("Error compiling shader: %s", msg);
			}
		}
#endif

		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, &fragShaderSource, nullptr);
		glCompileShader(fragmentShader);

#if defined(DEBUG_BUILD)
		{
			GLint status;
			glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
			if (status != GL_TRUE)
			{
				char msg[256];
				GLsizei len;
				glGetShaderInfoLog(fragmentShader, sizeof(msg), &len, msg);
				SDL_Log("Error compiling shader: %s", msg);
			}
		}
#endif

		GLuint program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);

#if defined(DEBUG_BUILD)
		{
			GLint status;
			glGetProgramiv(program, GL_LINK_STATUS, &status);
			if (status != GL_TRUE)
			{
				char msg[256];
				GLsizei len;
				glGetProgramInfoLog(program, sizeof(msg), &len, msg);
				SDL_Log("Error linking shader program: %s", msg);
			}
		}
#endif

		glUseProgram(program);
	}

	bool running = true;
	while (running)
	{
		SDL_Event evt;
		while (SDL_PollEvent(&evt) != 0)
		{
			switch (evt.type)
			{
				case SDL_QUIT:
				{
					running = false;
				} break;
				case SDL_KEYUP:
				case SDL_KEYDOWN:
				{
					const bool isDown = evt.key.state == SDL_PRESSED;
					switch (evt.key.keysym.sym)
					{
						case SDLK_q:
							running = false;
							break;
					}
				} break;
			}
		}
		glClearColor(0.95f, 0.88f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		SDL_GL_SwapWindow(window);
	}

	{
		glDeleteBuffers(1, &vertexBuffer);
		glDeleteVertexArrays(1, &vao);
	}

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return;
}
}; // end anonymous namespace

int main(int argc, char **argv)
{
	(void) argc, argv;
	StartGame();
	return 0;
}
