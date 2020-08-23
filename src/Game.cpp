#include "SDL/SDL.h"
#include "SDL/SDL_OpenGL.h"

#include "General.h"
#include "Math.h"

namespace {
#include "OpenGL.h"

const GLchar *vertexShaderSource = "\
#version 330 core\n\
layout (location = 0) in vec3 pos;\n\
layout (location = 1) in vec3 col;\n\
uniform mat4 model;\n\
uniform mat4 view;\n\
uniform mat4 projection;\n\
out vec3 vertexColor;\n\
\n\
void main()\n\
{\n\
	gl_Position = projection * view * model * vec4(pos, 1.0);\n\
	vertexColor = col;\n\
}\n\
";

const GLchar *fragShaderSource = "\
#version 330 core\n\
in vec3 vertexColor;\n\
out vec4 fragColor;\n\
\n\
void main()\n\
{\n\
	fragColor = vec4(vertexColor, 1.0);\n\
}\n\
";

struct Vertex
{
	v3 pos;
	v3 color;
};

struct Controller
{
	bool up;
	bool down;
	bool left;
	bool right;
	bool camUp;
	bool camDown;
	bool camLeft;
	bool camRight;
};

struct GameState
{
	Controller controller;
	v3 playerPos;
	f32 playerYaw;
	v3 camPos;
	f32 camYaw;
	f32 camPitch;
};

GLuint LoadShader(const GLchar *shaderSource, GLuint shaderType)
{
	GLuint shader = glCreateShader(shaderType);
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
			SDL_Log("Error compiling shader: %s", msg);
		}
	}
#endif

	return shader;
}

void StartGame()
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	// OpenGL versions
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	// OpenGL no backwards comp and debug flag
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

	struct
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
	} cubeMesh, planeMesh;
	GLuint program;
	{
		const Vertex lovelyVertices[] =
		{
			// Top
			{ { -1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { 1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { 1.0f, 1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { -1.0f, 1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f } },

			// Bottom
			{ { 1.0f, -1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
			{ { -1.0f, -1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
			{ { -1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
			{ { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },

			// Left
			{ { -1.0f, -1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
			{ { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, 1.0f } },
			{ { -1.0f, 1.0f, -1.0f }, { 0.0f, 0.0f, 1.0f } },
			{ { -1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },

			// Right
			{ { 1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 0.0f } },
			{ { 1.0f, -1.0f, 1.0f }, { 1.0f, 1.0f, 0.0f } },
			{ { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 0.0f } },
			{ { 1.0f, 1.0f, -1.0f }, { 1.0f, 1.0f, 0.0f } },

			// Back
			{ { -1.0f, -1.0f, 1.0f }, { 1.0f, 0.0f, 1.0f } },
			{ { 1.0f, -1.0f, 1.0f }, { 1.0f, 0.0f, 1.0f } },
			{ { 1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 1.0f } },
			{ { -1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 1.0f } },

			// Front
			{ { -1.0f, 1.0f, -1.0f }, { 0.0f, 1.0f, 1.0f } },
			{ { 1.0f, 1.0f, -1.0f }, { 0.0f, 1.0f, 1.0f } },
			{ { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 1.0f } },
			{ { -1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 1.0f } },
		};

		u16 lovelyIndices[] =
		{
			// Front
			0, 1, 2, 0, 2, 3,

			// Back
			4, 5, 6, 4, 6, 7,

			// Left
			8, 9, 10, 8, 10, 11,

			// Right
			12, 13, 14, 12, 14, 15,

			// Bottom
			16, 17, 18, 16, 18, 19,

			// Top
			20, 21, 22, 20, 22, 23,
		};

		const Vertex planeVertices[] =
		{
			{ { -10.0f, -10.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { 10.0f, -10.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
			{ { -10.0f, 10.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
			{ { 10.0f, 10.0f, 0.0f }, { 1.0f, 0.0f, 1.0f } }
		};

		const u16 planeIndices[] =
		{
			3, 0, 2, 3, 1, 0
		};

		// Player
		{
			glGenVertexArrays(1, &cubeMesh.vao);
			glBindVertexArray(cubeMesh.vao);

			glGenBuffers(2, cubeMesh.buffers);
			glBindBuffer(GL_ARRAY_BUFFER, cubeMesh.vertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(lovelyVertices), lovelyVertices, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeMesh.indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(lovelyIndices), lovelyIndices, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
			glEnableVertexAttribArray(0);
			GLintptr colorOffset = sizeof(v3);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)colorOffset);
			glEnableVertexAttribArray(1);
		}

		// Plane
		{
			glGenVertexArrays(1, &planeMesh.vao);
			glBindVertexArray(planeMesh.vao);

			glGenBuffers(2, planeMesh.buffers);
			glBindBuffer(GL_ARRAY_BUFFER, planeMesh.vertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeMesh.indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
			glEnableVertexAttribArray(0);
			GLintptr colorOffset = sizeof(v3);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)colorOffset);
			glEnableVertexAttribArray(1);
		}

		// Shaders
		GLuint vertexShader = LoadShader(vertexShaderSource, GL_VERTEX_SHADER);
		GLuint fragmentShader = LoadShader(fragShaderSource, GL_FRAGMENT_SHADER);

		program = glCreateProgram();
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

		glEnable(GL_DEPTH_TEST);

		const f32 fov = HALFPI;
		const f32 near = 0.01f;
		const f32 far = 2000.0f;
		const f32 aspectRatio = (16.0f / 9.0f);

		const f32 top = Tan(HALFPI - fov / 2.0f);
		const f32 right = top / aspectRatio;

		mat4 proj =
		{
			right, 0.0f, 0.0f, 0.0f,
			0.0f, top, 0.0f, 0.0f,
			0.0f, 0.0f, -(far + near) / (far - near), -1.0f,
			0.0f, 0.0f, -(2.0f * far * near) / (far - near), 0.0f
		};
		GLuint projUniform = glGetUniformLocation(program, "projection");
		glUniformMatrix4fv(projUniform, 1, false, proj.m);
	}

	GameState gameState = {};
	gameState.camPitch = -PI * 0.35f;

	u64 lastPerfCounter = SDL_GetPerformanceCounter();
	bool running = true;
	while (running)
	{
		const u64 newPerfCounter = SDL_GetPerformanceCounter();
		const f64 time = (f64)newPerfCounter / (f64)SDL_GetPerformanceFrequency();
		const f32 deltaTime = (f32)(newPerfCounter - lastPerfCounter) / (f32)SDL_GetPerformanceFrequency();

		// Check events
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
						case SDLK_w:
							gameState.controller.up = isDown;
							break;
						case SDLK_a:
							gameState.controller.left = isDown;
							break;
						case SDLK_s:
							gameState.controller.down = isDown;
							break;
						case SDLK_d:
							gameState.controller.right = isDown;
							break;

						case SDLK_UP:
							gameState.controller.camUp = isDown;
							break;
						case SDLK_DOWN:
							gameState.controller.camDown = isDown;
							break;
						case SDLK_LEFT:
							gameState.controller.camLeft = isDown;
							break;
						case SDLK_RIGHT:
							gameState.controller.camRight = isDown;
							break;
					}
				} break;
			}
		}

		// Update
		{
			if (gameState.controller.camUp)
				gameState.camPitch += 1.0f * deltaTime;
			else if (gameState.controller.camDown)
				gameState.camPitch -= 1.0f * deltaTime;

			if (gameState.controller.camLeft)
				gameState.camYaw += 1.0f * deltaTime;
			else if (gameState.controller.camRight)
				gameState.camYaw -= 1.0f * deltaTime;

			v3 fw = {};
			if (gameState.controller.up)
				fw += { Sin(gameState.camYaw), Cos(gameState.camYaw) };
			else if (gameState.controller.down)
				fw += { -Sin(gameState.camYaw), -Cos(gameState.camYaw) };

			if (gameState.controller.left)
				fw += { -Cos(gameState.camYaw), Sin(gameState.camYaw) };
			else if (gameState.controller.right)
				fw += { Cos(gameState.camYaw), -Sin(gameState.camYaw) };

			if (V3SqrLen(fw) > 1)
				fw *= 0.7071f;

			if (V3SqrLen(fw) > 0)
			{
				gameState.playerYaw = Atan2(fw.y, fw.x);

				const f32 playerSpeed = 3.0f;
				gameState.playerPos += fw * playerSpeed * deltaTime;
				gameState.camPos = gameState.playerPos;
			}
		}

		// Draw
		{
			// View matrix
			{
				mat4 view =
				{
					1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, -5.0f, 1.0f
				};
				const f32 pitch = gameState.camPitch;
				mat4 pitchMatrix =
				{
					1.0f,	0.0f,			0.0f,		0.0f,
					0.0f,	Cos(pitch),		Sin(pitch),	0.0f,
					0.0f,	-Sin(pitch),	Cos(pitch),	0.0f,
					0.0f,	0.0f,			0.0f,		1.0f
				};
				view = Mat4Multiply(pitchMatrix, view);
				const f32 yaw = gameState.camYaw;
				mat4 yawMatrix =
				{
					Cos(yaw),	Sin(yaw),	0.0f,	0.0f,
					-Sin(yaw),	Cos(yaw),	0.0f,	0.0f,
					0.0f,		0.0f,		1.0f,	0.0f,
					0.0f,		0.0f,		0.0f,	1.0f
				};
				view = Mat4Multiply(yawMatrix, view);
				const v3 pos = gameState.camPos;
				mat4 camPosMatrix =
				{
					1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					-pos.x, -pos.y, -pos.z, 1.0f
				};
				view = Mat4Multiply(camPosMatrix, view);
				GLuint viewUniform = glGetUniformLocation(program, "view");
				glUniformMatrix4fv(viewUniform, 1, false, view.m);
			}

			glClearColor(0.95f, 0.88f, 0.05f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Player
			{
				const v3 pos = gameState.playerPos;
				const v3 fw = { Cos(gameState.playerYaw), Sin(gameState.playerYaw), 0 };
				const v3 right = { fw.y, -fw.x, 0 };
				const v3 up = { 0, 0, 1 };
				// TODO double check axis system (left/right hand and so on)
				// TODO pull out function?
				const mat4 model =
				{
					right.x,	right.y,	right.z,	0.0f,
					fw.x,		fw.y,		fw.z,		0.0f,
					up.x,		up.y,		up.z,		0.0f,
					pos.x,		pos.y,		pos.z,		1.0f
				};
				const GLuint modelUniform = glGetUniformLocation(program, "model");
				glUniformMatrix4fv(modelUniform, 1, false, model.m);

				glBindVertexArray(cubeMesh.vao);
				glDrawElements(GL_TRIANGLES, 12 * 3, GL_UNSIGNED_SHORT, NULL);
			}

			// Plane
			{
				mat4 model =
				{
					1.0f,	0.0f,	0.0f,	0.0f,
					0.0f,	1.0f,	0.0f,	0.0f,
					0.0f,	0.0f,	1.0f,	0.0f,
					0.0f,	0.0f,	-1.0f,	1.0f
				};
				const GLuint modelUniform = glGetUniformLocation(program, "model");
				glUniformMatrix4fv(modelUniform, 1, false, model.m);

				glBindVertexArray(planeMesh.vao);
				glDrawElements(GL_TRIANGLES, 2 * 3, GL_UNSIGNED_SHORT, NULL);
			}

			SDL_GL_SwapWindow(window);
		}

		lastPerfCounter = newPerfCounter;
	}

	// Cleanup
	{
		glDeleteBuffers(1, &cubeMesh.vertexBuffer);
		glDeleteVertexArrays(1, &cubeMesh.vao);

		SDL_GL_DeleteContext(glContext);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	return;
}
}; // end anonymous namespace

int main(int argc, char **argv)
{
	(void) argc, argv;
	StartGame();
	return 0;
}
