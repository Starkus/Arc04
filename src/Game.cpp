#include "General.h"
#include "Math.h"
#include "Geometry.h"
#include "Primitives.h"
#include "Wavefront.h"
#include "OpenGL.h"

#include "Game.h"

#include "Collision.h"

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

	DeviceMesh playerMesh, anvilMesh, cubeMesh;
	GLuint program;
	{
		// Backface culling
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CW);

		// Player
		{
			OBJLoadResult obj = LoadOBJ("data/monkey.obj");
			playerMesh.indexCount = obj.indexCount;

			glGenVertexArrays(1, &playerMesh.vao);
			glBindVertexArray(playerMesh.vao);

			glGenBuffers(2, playerMesh.buffers);
			glBindBuffer(GL_ARRAY_BUFFER, playerMesh.vertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(obj.vertices[0]) * obj.vertexCount, obj.vertices, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, playerMesh.indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(obj.indices[0]) * obj.indexCount, obj.indices, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, color));
			glEnableVertexAttribArray(1);

			free(obj.vertices);
			free(obj.indices);
		}

		// Anvil
		{
			OBJLoadResult obj = LoadOBJ("data/anvil.obj");
			anvilMesh.indexCount = obj.indexCount;

			glGenVertexArrays(1, &anvilMesh.vao);
			glBindVertexArray(anvilMesh.vao);

			glGenBuffers(2, anvilMesh.buffers);
			glBindBuffer(GL_ARRAY_BUFFER, anvilMesh.vertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(obj.vertices[0]) * obj.vertexCount, obj.vertices, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, anvilMesh.indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(obj.indices[0]) * obj.indexCount, obj.indices, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, color));
			glEnableVertexAttribArray(1);

			free(obj.vertices);
			free(obj.indices);
		}

		// Cube
		{
			cubeMesh.indexCount = 6 * 2 * 3;

			glGenVertexArrays(1, &cubeMesh.vao);
			glBindVertexArray(cubeMesh.vao);

			glGenBuffers(2, cubeMesh.buffers);
			glBindBuffer(GL_ARRAY_BUFFER, cubeMesh.vertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeMesh.indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, color));
			glEnableVertexAttribArray(1);
		}

		// Debug geometry buffer
		{
			debugGeometryBuffer.vertexData = (Vertex *)malloc(2048 * sizeof(Vertex));
			debugGeometryBuffer.vertexCount = 0;
			glGenVertexArrays(1, &debugGeometryBuffer.vao);
			glBindVertexArray(debugGeometryBuffer.vao);

			glGenBuffers(1, &debugGeometryBuffer.deviceBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, debugGeometryBuffer.deviceBuffer);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, color));
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

	// Init level
	{
		DeviceMesh *levelMesh = &gameState.levelGeometry.renderMesh;
		OBJLoadResult obj = LoadOBJ("data/level.obj");
		levelMesh->indexCount = obj.indexCount;

		glGenVertexArrays(1, &levelMesh->vao);
		glBindVertexArray(levelMesh->vao);

		glGenBuffers(2, levelMesh->buffers);
		glBindBuffer(GL_ARRAY_BUFFER, levelMesh->vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(obj.vertices[0]) * obj.vertexCount, obj.vertices, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, levelMesh->indexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(obj.indices[0]) * obj.indexCount, obj.indices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, color));
		glEnableVertexAttribArray(1);

		// Save triangles
		gameState.levelGeometry.triangles = (v3 *)malloc(obj.vertexCount * sizeof(v3));
		gameState.levelGeometry.triangleCount = obj.vertexCount / 3;
		for (u32 vertexIdx = 0; vertexIdx < obj.vertexCount; ++vertexIdx)
		{
			// FIXME we are ignoring indices here!!! this will not work if we start eliminating
			// duplicate vertices on wavefront importer!!
			Vertex *vertex = &obj.vertices[vertexIdx];
			gameState.levelGeometry.triangles[vertexIdx] = vertex->pos;
		}

		free(obj.vertices);
		free(obj.indices);
	}

	// Init player
	{
		Entity *playerEnt = &gameState.entities[gameState.entityCount++];
		gameState.player.entity = playerEnt;
		playerEnt->fw = { 0.0f, 1.0f, 0.0f };
		playerEnt->mesh = &playerMesh;

		LoadOBJAsPoints("data/monkey_collision.obj", &playerEnt->collisionPoints,
				&playerEnt->collisionPointCount);
	}

	// Init camera
	gameState.camPitch = -PI * 0.35f;

	// Test entities
	{
		v3 *points;
		u32 pointCount;
		LoadOBJAsPoints("data/anvil_collision.obj", &points, &pointCount);

		Entity *testEntity = &gameState.entities[gameState.entityCount++];
		testEntity->pos = { -5.0f, 2.0f, 1.0f };
		testEntity->fw = { 0.0f, 1.0f, 0.0f };
		testEntity->mesh = &anvilMesh;
		testEntity->collisionPoints = points;
		testEntity->collisionPointCount = pointCount;

		testEntity = &gameState.entities[gameState.entityCount++];
		testEntity->pos = { 4.0f, 3.0f, 1.0f };
		testEntity->fw = { 0.0f, 1.0f, 0.0f };
		testEntity->mesh = &anvilMesh;
		testEntity->collisionPoints = points;
		testEntity->collisionPointCount = pointCount;

		testEntity = &gameState.entities[gameState.entityCount++];
		testEntity->pos = { 2.0f, -3.0f, 1.0f };
		testEntity->fw = { 0.707f, 0.707f, 0.0f };
		testEntity->mesh = &anvilMesh;
		testEntity->collisionPoints = points;
		testEntity->collisionPointCount = pointCount;

		testEntity = &gameState.entities[gameState.entityCount++];
		testEntity->pos = { -7.0f, -3.0f, 1.0f };
		testEntity->fw = { 0.707f, 0.0f, 0.707f };
		testEntity->mesh = &anvilMesh;
		testEntity->collisionPoints = points;
		testEntity->collisionPointCount = pointCount;
	}

	u64 lastPerfCounter = SDL_GetPerformanceCounter();
	bool running = true;
	while (running)
	{
		const u64 newPerfCounter = SDL_GetPerformanceCounter();
		const f64 time = (f64)newPerfCounter / (f64)SDL_GetPerformanceFrequency();
		const f32 deltaTime = (f32)(newPerfCounter - lastPerfCounter) / (f32)SDL_GetPerformanceFrequency();

#if DEBUG_BUILD
		debugCubeCount = 0;
#endif

		// Check events
		for (int buttonIdx = 0; buttonIdx < ArrayCount(gameState.controller.b); ++buttonIdx)
			gameState.controller.b[buttonIdx].changed = false;

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

					auto checkButton = [&isDown](Button &button)
					{
						if (button.endedDown != isDown)
						{
							button.endedDown = isDown;
							button.changed = true;
						}
					};

					Controller &c = gameState.controller;
					switch (evt.key.keysym.sym)
					{
						case SDLK_q:
							running = false;
							break;
						case SDLK_w:
							checkButton(c.up);
							break;
						case SDLK_a:
							checkButton(c.left);
							break;
						case SDLK_s:
							checkButton(c.down);
							break;
						case SDLK_d:
							checkButton(c.right);
							break;

						case SDLK_SPACE:
							checkButton(c.jump);
							break;

						case SDLK_UP:
							checkButton(c.camUp);
							break;
						case SDLK_DOWN:
							checkButton(c.camDown);
							break;
						case SDLK_LEFT:
							checkButton(c.camLeft);
							break;
						case SDLK_RIGHT:
							checkButton(c.camRight);
							break;

#if EPA_VISUAL_DEBUGGING
						case SDLK_j:
							checkButton(c.epaStepUp);
							break;
						case SDLK_k:
							checkButton(c.epaStepDown);
							break;
#endif
					}
				} break;
			}
		}

		// Update
		{
#if EPA_VISUAL_DEBUGGING
			if (gameState.controller.epaStepUp.endedDown && gameState.controller.epaStepUp.changed)
			{
				g_currentPolytopeStep++;
				if (g_currentPolytopeStep >= 16)
					g_currentPolytopeStep = 16;
			}
			if (gameState.controller.epaStepDown.endedDown && gameState.controller.epaStepDown.changed)
			{
				g_currentPolytopeStep--;
				if (g_currentPolytopeStep < 0)
					g_currentPolytopeStep = 0;
			}
			DRAW_AA_DEBUG_CUBE(v3{}, 0.05f); // Draw origin
#endif

			if (gameState.controller.camUp.endedDown)
				gameState.camPitch += 1.0f * deltaTime;
			else if (gameState.controller.camDown.endedDown)
				gameState.camPitch -= 1.0f * deltaTime;

			if (gameState.controller.camLeft.endedDown)
				gameState.camYaw -= 1.0f * deltaTime;
			else if (gameState.controller.camRight.endedDown)
				gameState.camYaw += 1.0f * deltaTime;

			// Move player
			Player *player = &gameState.player;
			{
				v3 worldInputDir = {};
				if (gameState.controller.up.endedDown)
					worldInputDir += { Sin(gameState.camYaw), Cos(gameState.camYaw) };
				else if (gameState.controller.down.endedDown)
					worldInputDir += { -Sin(gameState.camYaw), -Cos(gameState.camYaw) };

				if (gameState.controller.left.endedDown)
					worldInputDir += { -Cos(gameState.camYaw), Sin(gameState.camYaw) };
				else if (gameState.controller.right.endedDown)
					worldInputDir += { Cos(gameState.camYaw), -Sin(gameState.camYaw) };

				f32 sqlen = V3SqrLen(worldInputDir);
				if (sqlen > 1)
					worldInputDir /= Sqrt(sqlen);

				if (V3SqrLen(worldInputDir) > 0)
				{
					player->entity->fw = worldInputDir;
				}

				const f32 playerSpeed = 3.0f;
				player->entity->pos += worldInputDir * playerSpeed * deltaTime;

				// Jump
				if (player->state == PLAYERSTATE_GROUNDED && gameState.controller.jump.endedDown &&
						gameState.controller.jump.changed)
				{
					player->vel.z = 15.0f;
					player->state = PLAYERSTATE_AIR;
				}

				// Gravity
				if (player->state == PLAYERSTATE_AIR)
				{
					player->vel.z -= 30.0f * deltaTime;
					if (player->vel.z < -75.0f)
						player->vel.z = -75.0f;
					player->entity->pos.z += player->vel.z * deltaTime;
				}
				else if (player->state == PLAYERSTATE_GROUNDED)
				{
					player->vel.z = 0;
				}
			}

			// Collision
			bool touchedGround = false;

			for (u32 entityIndex = 0; entityIndex < gameState.entityCount; ++ entityIndex)
			{
				Entity *entity = &gameState.entities[entityIndex];
				if (entity != gameState.player.entity)
				{
					const v3 up = { 0, 0, 1 };
					const v3 aPos = player->entity->pos;
					const v3 aFw = player->entity->fw;
					const v3 aRight = V3Normalize(V3Cross(aFw, up));
					const v3 aUp = V3Cross(aRight, aFw);
					mat4 aModelMatrix =
					{
						aRight.x,	aRight.y,	aRight.z,	0.0f,
						aFw.x,		aFw.y,		aFw.z,		0.0f,
						aUp.x,		aUp.y,		aUp.z,		0.0f,
						aPos.x,		aPos.y,		aPos.z,		1.0f
					};

					const v3 bPos = entity->pos;
					const v3 bFw = entity->fw;
					const v3 bRight = V3Normalize(V3Cross(bFw, up));
					const v3 bUp = V3Cross(bRight, bFw);
					mat4 bModelMatrix =
					{
						bRight.x,	bRight.y,	bRight.z,	0.0f,
						bFw.x,		bFw.y,		bFw.z,		0.0f,
						bUp.x,		bUp.y,		bUp.z,		0.0f,
						bPos.x,		bPos.y,		bPos.z,		1.0f
					};

					u32 vACount = player->entity->collisionPointCount;
					u32 vBCount = entity->collisionPointCount;
					v3 *vA = (v3 *)malloc(vACount * sizeof(v3));
					v3 *vB = (v3 *)malloc(vBCount * sizeof(v3));

					// Compute all points
					{
						for (u32 i = 0; i < vACount; ++i)
						{
							v3 p = player->entity->collisionPoints[i];
							v4 v = { p.x, p.y, p.z, 1.0f };
							v = Mat4TransformV4(aModelMatrix, v);
							vA[i] = { v.x, v.y, v.z };
						}

						for (u32 i = 0; i < vBCount; ++i)
						{
							v3 p = entity->collisionPoints[i];
							v4 v = { p.x, p.y, p.z, 1.0f };
							v = Mat4TransformV4(bModelMatrix, v);
							vB[i] = { v.x, v.y, v.z };
						}
					}

					GJKResult gjkResult = GJKTest(vA, vACount, vB, vBCount);
					if (gjkResult.hit)
					{
						v3 depenetration = ComputeDepenetration(gjkResult, vA, vACount, vB, vBCount);
						player->entity->pos += depenetration;
						// TOFIX handle velocity change upon hits properly
						if (V3Normalize(depenetration).z > 0.9f)
							touchedGround = true;
						break;
					}

					free(vA);
					free(vB);
				}
			}

			// Ray testing
			{
				LevelGeometry *level = &gameState.levelGeometry;
				for (u32 triIdx = 0; triIdx < level->triangleCount; ++triIdx)
				{
					v3 origin = player->entity->pos + v3{ 0, 0, 1 };
					v3 dir = { 0, 0, -1.1f };
					v3 a = level->triangles[triIdx * 3 + 0];
					v3 b = level->triangles[triIdx * 3 + 1];
					v3 c = level->triangles[triIdx * 3 + 2];
					v3 hit;
					if (RayTriangleIntersection(origin, dir, a, b, c, &hit))
					{
						DRAW_AA_DEBUG_CUBE(hit, 0.2f);
						DRAW_AA_DEBUG_CUBE(a, 0.1f);
						DRAW_AA_DEBUG_CUBE(b, 0.1f);
						DRAW_AA_DEBUG_CUBE(c, 0.1f);

						player->entity->pos.z = hit.z;
						touchedGround = true;
						break;
					}
				}
			}

			player->state = touchedGround ? PLAYERSTATE_GROUNDED : PLAYERSTATE_AIR;

			gameState.camPos = player->entity->pos;

#if DEBUG_BUILD
			// Draw a cube for each entity
			for (u32 entityIdx = 0; entityIdx < gameState.entityCount; ++entityIdx)
			{
				Entity *entity = &gameState.entities[entityIdx];
				v3 up = { 0, 0, 1 };
				DRAW_DEBUG_CUBE(entity->pos, entity->fw, up, 1.0f);
			}
#endif

#if EPA_VISUAL_DEBUGGING
			Vertex *epaVertices;
			u32 epaFaceCount;
			GetEPAStepGeometry(g_currentPolytopeStep, &epaVertices, &epaFaceCount);
			DrawDebugTriangles(epaVertices, epaFaceCount * 3);
#endif
		}

		// Draw
		{
			// View matrix
			{
				mat4 view = Mat4Translation({ 0.0f, 0.0f, -5.0f });
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
				mat4 camPosMatrix = Mat4Translation(-pos);
				view = Mat4Multiply(camPosMatrix, view);

				GLuint viewUniform = glGetUniformLocation(program, "view");
				glUniformMatrix4fv(viewUniform, 1, false, view.m);
			}

			const GLuint modelUniform = glGetUniformLocation(program, "model");

			glClearColor(0.95f, 0.88f, 0.05f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Entity
			for (u32 entityIdx = 0; entityIdx < gameState.entityCount; ++entityIdx)
			{
				Entity *entity = &gameState.entities[entityIdx];
				const v3 pos = entity->pos;
				const v3 fw = entity->fw;
				const v3 right = V3Cross(entity->fw, {0,0,1});
				const v3 up = V3Cross(right, fw);
				// TODO pull out function?
				const mat4 model =
				{
					right.x,	right.y,	right.z,	0.0f,
					fw.x,		fw.y,		fw.z,		0.0f,
					up.x,		up.y,		up.z,		0.0f,
					pos.x,		pos.y,		pos.z,		1.0f
				};
				glUniformMatrix4fv(modelUniform, 1, false, model.m);

				glBindVertexArray(entity->mesh->vao);
				glDrawElements(GL_TRIANGLES, entity->mesh->indexCount, GL_UNSIGNED_SHORT, NULL);
			}

			// Level
			{
				LevelGeometry *level = &gameState.levelGeometry;

				glUniformMatrix4fv(modelUniform, 1, false, MAT4_IDENTITY.m);
				glBindVertexArray(level->renderMesh.vao);
				glDrawElements(GL_TRIANGLES, level->renderMesh.indexCount, GL_UNSIGNED_SHORT, NULL);
			}

#if 0//DEBUG_BUILD
			// Debug draws
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

				for (u32 i = 0; i < debugCubeCount; ++i)
				{
					DebugCube *cube = &debugCubes[i];

					v3 pos = cube->pos;
					v3 fw = cube->fw * cube->scale;
					v3 right = V3Normalize(V3Cross(cube->fw, cube->up));
					v3 up = V3Cross(right, cube->fw) * cube->scale;
					right *= cube->scale;
					mat4 model =
					{
						right.x,	right.y,	right.z,	0.0f,
						fw.x,		fw.y,		fw.z,		0.0f,
						up.x,		up.y,		up.z,		0.0f,
						pos.x,		pos.y,		pos.z,		1.0f
					};
					glUniformMatrix4fv(modelUniform, 1, false, model.m);
					glBindVertexArray(cubeMesh.vao);
					glDrawElements(GL_TRIANGLES, cubeMesh.indexCount, GL_UNSIGNED_SHORT, NULL);
				}

				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}

			// Debug meshes
			{
				//glDisable(GL_CULL_FACE);
				glUniformMatrix4fv(modelUniform, 1, false, MAT4_IDENTITY.m);

				glBindVertexArray(debugGeometryBuffer.vao);

				glBindBuffer(GL_ARRAY_BUFFER, debugGeometryBuffer.deviceBuffer);
				glBufferData(GL_ARRAY_BUFFER, debugGeometryBuffer.vertexCount * sizeof(Vertex),
						debugGeometryBuffer.vertexData, GL_DYNAMIC_DRAW);

				glDrawArrays(GL_TRIANGLES, 0, debugGeometryBuffer.vertexCount);

				debugGeometryBuffer.vertexCount = 0;
				glEnable(GL_CULL_FACE);
			}
#endif

			SDL_GL_SwapWindow(window);
		}

		lastPerfCounter = newPerfCounter;
	}

	// Cleanup
	{
		glDeleteBuffers(2, playerMesh.buffers);
		glDeleteBuffers(2, anvilMesh.buffers);
		glDeleteBuffers(2, gameState.levelGeometry.renderMesh.buffers);
		glDeleteBuffers(2, cubeMesh.buffers);
		glDeleteVertexArrays(1, &playerMesh.vao);
		glDeleteVertexArrays(1, &anvilMesh.vao);
		glDeleteVertexArrays(1, &gameState.levelGeometry.renderMesh.vao);
		glDeleteVertexArrays(1, &cubeMesh.vao);

#if DEBUG_BUILD
		glDeleteBuffers(1, &debugGeometryBuffer.deviceBuffer);
		glDeleteVertexArrays(1, &debugGeometryBuffer.vao);
#endif

		SDL_GL_DeleteContext(glContext);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	return;
}

int main(int argc, char **argv)
{
	(void) argc, argv;
	StartGame();
	return 0;
}
