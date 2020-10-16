#include "SDL/SDL.h"
#include "SDL/SDL_OpenGL.h"

#include "General.h"
#include "Maths.h"
#include "Geometry.h"
#include "Primitives.h"
#include "OpenGL.h"

#include "Game.h"

#include "Collision.cpp"
#include "BakeryInterop.cpp"

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

void FreeSkeletalMesh(SkeletalMesh *ptr)
{
	free(ptr->bindPoses);
	for (u32 i = 0; i < ptr->animationCount; ++i)
	{
		for (u32 j = 0; j < ptr->animations[i].channelCount; ++j)
		{
			free(ptr->animations[i].channels[j].transforms);
		}
		free(ptr->animations[i].channels);
		free(ptr->animations[i].timestamps);
	}
	free(ptr->animations);
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

	DeviceMesh anvilMesh, cubeMesh;
	SkeletalMesh skinnedMesh = {};
	GLuint program, skinnedMeshProgram;
	{
		// Backface culling
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CW);

		// Anvil
		{
			Vertex *vertexData;
			u16 *indexData;
			u32 vertexCount;
			u32 indexCount;
			void *fileBuffer = ReadMesh("data/anvil.bin", &vertexData, &indexData, &vertexCount, &indexCount);

			anvilMesh.indexCount = indexCount;

			glGenVertexArrays(1, &anvilMesh.vao);
			glBindVertexArray(anvilMesh.vao);

			glGenBuffers(2, anvilMesh.buffers);
			glBindBuffer(GL_ARRAY_BUFFER, anvilMesh.vertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData[0]) * vertexCount, vertexData, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, anvilMesh.indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexData[0]) * indexCount, indexData, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, uv));
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, nor));
			glEnableVertexAttribArray(2);

			free(fileBuffer);
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
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, nor));
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
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, nor));
			glEnableVertexAttribArray(1);
		}

		// Skinned mesh
		{
			SkinnedVertex *vertexData;
			u16 *indexData;
			u32 vertexCount;
			void *fileBuffer = ReadSkinnedMesh("data/Sparkus.bin", &skinnedMesh, &vertexData, &indexData, &vertexCount);

			glGenVertexArrays(1, &skinnedMesh.deviceMesh.vao);
			glBindVertexArray(skinnedMesh.deviceMesh.vao);

			glGenBuffers(2, skinnedMesh.deviceMesh.buffers);
			glBindBuffer(GL_ARRAY_BUFFER, skinnedMesh.deviceMesh.vertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(SkinnedVertex) * vertexCount, vertexData, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skinnedMesh.deviceMesh.indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u16) * skinnedMesh.deviceMesh.indexCount, indexData, GL_STATIC_DRAW);

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

			free(fileBuffer);
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

		GLuint skinVertexShader = LoadShader(skinVertexShaderSource, GL_VERTEX_SHADER);
		GLuint skinFragmentShader = fragmentShader;

		skinnedMeshProgram = glCreateProgram();
		glAttachShader(skinnedMeshProgram, skinVertexShader);
		glAttachShader(skinnedMeshProgram, skinFragmentShader);
		glLinkProgram(skinnedMeshProgram);
#if defined(DEBUG_BUILD)
		{
			GLint status;
			glGetProgramiv(skinnedMeshProgram, GL_LINK_STATUS, &status);
			if (status != GL_TRUE)
			{
				char msg[256];
				GLsizei len;
				glGetProgramInfoLog(skinnedMeshProgram, sizeof(msg), &len, msg);
				SDL_Log("Error linking shader program: %s", msg);
			}
		}
#endif

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

		glUseProgram(program);
		GLuint projUniform = glGetUniformLocation(program, "projection");
		glUniformMatrix4fv(projUniform, 1, false, proj.m);

		glUseProgram(skinnedMeshProgram);
		GLuint projUniformAlt = glGetUniformLocation(skinnedMeshProgram, "projection");
		glUniformMatrix4fv(projUniformAlt, 1, false, proj.m);
	}

	GameState gameState = {};

	// Init level
	{
		Vertex *vertexData;
		u16 *indexData;
		u32 vertexCount;
		u32 indexCount;
		void *fileBuffer = ReadMesh("data/level_graphics.bin", &vertexData, &indexData, &vertexCount, &indexCount);

		DeviceMesh *levelMesh = &gameState.levelGeometry.renderMesh;
		levelMesh->indexCount = indexCount;

		glGenVertexArrays(1, &levelMesh->vao);
		glBindVertexArray(levelMesh->vao);

		glGenBuffers(2, levelMesh->buffers);
		glBindBuffer(GL_ARRAY_BUFFER, levelMesh->vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertexCount, vertexData, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, levelMesh->indexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u16) * indexCount, indexData, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, uv));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, nor));
		glEnableVertexAttribArray(2);

		Triangle *triangleData;
		u32 triangleCount;
		ReadTriangleGeometry("data/level.bin", &triangleData, &triangleCount);
		gameState.levelGeometry.triangles = triangleData;
		gameState.levelGeometry.triangleCount = triangleCount;

		free(fileBuffer);
	}

	// Init player
	{
		Entity *playerEnt = &gameState.entities[gameState.entityCount++];
		gameState.player.entity = playerEnt;
		playerEnt->fw = { 0.0f, 1.0f, 0.0f };
		playerEnt->mesh = 0;

		ReadPoints("data/monkey_collision.bin", &playerEnt->collisionPoints,
			&playerEnt->collisionPointCount);

		gameState.animationIdx = 0;
		gameState.loopAnimation = true;
	}

	// Init camera
	gameState.camPitch = -PI * 0.35f;

	// Test entities
	{
		v3 *points;
		u32 pointCount;
		ReadPoints("data/anvil_collision.bin", &points, &pointCount);

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

#if DEBUG_BUILD
						case SDLK_h:
							checkButton(c.debugUp);
							break;
						case SDLK_j:
							checkButton(c.debugDown);
							break;
						case SDLK_k:
							checkButton(c.debugUp);
							break;
						case SDLK_l:
							checkButton(c.debugDown);
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

			if (gameState.controller.debugUp.endedDown && gameState.controller.debugUp.changed)
			{
				++gameState.animationIdx;
				if (gameState.animationIdx >= (i32)skinnedMesh.animationCount)
					gameState.animationIdx = 0;
			}
			if (gameState.controller.debugDown.endedDown && gameState.controller.debugDown.changed)
			{
				--gameState.animationIdx;
				if (gameState.animationIdx < 0)
					gameState.animationIdx = skinnedMesh.animationCount - 1;
			}

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

				const f32 playerSpeed = 5.0f;
				player->entity->pos += worldInputDir * playerSpeed * deltaTime;

				// Jump
				if (player->state == PLAYERSTATE_GROUNDED && gameState.controller.jump.endedDown &&
						gameState.controller.jump.changed)
				{
					player->vel.z = 15.0f;
					player->state = PLAYERSTATE_AIR;

					gameState.animationIdx = 1;
					gameState.animationTime = 0;
					gameState.loopAnimation = false;
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
					Triangle &triangle = level->triangles[triIdx];
					v3 hit;
					if (RayTriangleIntersection(origin, dir, triangle, &hit))
					{
						DRAW_AA_DEBUG_CUBE(hit, 0.2f);
						DRAW_AA_DEBUG_CUBE(triangle.a, 0.1f);
						DRAW_AA_DEBUG_CUBE(triangle.b, 0.1f);
						DRAW_AA_DEBUG_CUBE(triangle.c, 0.1f);

						player->entity->pos.z = hit.z;
						touchedGround = true;
						break;
					}
				}
			}

			PlayerState newState = touchedGround ? PLAYERSTATE_GROUNDED : PLAYERSTATE_AIR;
			if (newState != player->state)
			{
				gameState.animationIdx = newState == PLAYERSTATE_GROUNDED ? 0 : 1;
				gameState.animationTime = 0;
				gameState.loopAnimation = newState == PLAYERSTATE_GROUNDED ? true : false;
				player->state = newState;
			}

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
			mat4 view;
			{
				view = Mat4Translation({ 0.0f, 0.0f, -5.0f });
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
			}

			glUseProgram(program);
			GLuint viewUniform = glGetUniformLocation(program, "view");
			glUniformMatrix4fv(viewUniform, 1, false, view.m);

			GLuint modelUniform = glGetUniformLocation(program, "model");

			glClearColor(0.95f, 0.88f, 0.05f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Entity
			for (u32 entityIdx = 0; entityIdx < gameState.entityCount; ++entityIdx)
			{
				Entity *entity = &gameState.entities[entityIdx];
				if (!entity->mesh)
					continue;

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

			// Skinned meshes
			glUseProgram(skinnedMeshProgram);
			viewUniform = glGetUniformLocation(skinnedMeshProgram, "view");
			modelUniform = glGetUniformLocation(skinnedMeshProgram, "model");

			GLuint jointsUniform = glGetUniformLocation(skinnedMeshProgram, "joints");
			glUniformMatrix4fv(viewUniform, 1, false, view.m);
			{
				glUniformMatrix4fv(modelUniform, 1, false, MAT4_IDENTITY.m);

				mat4 joints[128];
				for (int i = 0; i < 128; ++i)
				{
					joints[i] = MAT4_IDENTITY;
				}

				Animation *animation = &skinnedMesh.animations[gameState.animationIdx];

				f32 lastTimestamp = animation->timestamps[animation->frameCount - 1];

				int currentFrame = -1;
				for (u32 i = 0; i < animation->frameCount; ++i)
				{
					if (currentFrame == -1 && gameState.animationTime <= animation->timestamps[i])
						currentFrame = i - 1;
				}
				f32 prevStamp = animation->timestamps[currentFrame];
				f32 nextStamp = animation->timestamps[currentFrame + 1];
				f32 lerpWindow = nextStamp - prevStamp;
				f32 lerpTime = gameState.animationTime - prevStamp;
				f32 lerpT = lerpTime / lerpWindow;

				for (u32 jointIdx = 0; jointIdx < skinnedMesh.jointCount; ++jointIdx)
				{
					// Find channel for joint
					AnimationChannel *channel = 0;
					for (u32 chanIdx = 0; chanIdx < animation->channelCount; ++chanIdx)
					{
						if (animation->channels[chanIdx].jointIndex == jointIdx)
						{
							channel = &animation->channels[chanIdx];
							break;
						}
					}

					mat4 transform = MAT4_IDENTITY;

					// Stack to apply transforms in reverse order
					u32 stack[32];
					int stackSize = 1;
					stack[0] = jointIdx;

					u8 parentJoint = skinnedMesh.jointParents[jointIdx];
					while (parentJoint != 0xFF)
					{
						stack[stackSize++] = parentJoint;
						parentJoint = skinnedMesh.jointParents[parentJoint];
					}
					bool first = true;
					while (stackSize)
					{
						const u32 currentJoint = stack[--stackSize];

						// Find channel for current joint
						AnimationChannel *currentChannel = 0;
						for (u32 chanIdx = 0; chanIdx < animation->channelCount; ++chanIdx)
						{
							if (animation->channels[chanIdx].jointIndex == currentJoint)
							{
								currentChannel = &animation->channels[chanIdx];
								break;
							}
						}

						if (currentChannel == nullptr)
						{
							// If this joint has no channel, it must not be moving at all,
							// even considering parents
							transform = Mat4Multiply(skinnedMesh.restPoses[currentJoint], transform);
							first = false;
							continue;
						}

						mat4 a = currentChannel->transforms[currentFrame];
						mat4 b = currentChannel->transforms[currentFrame + 1];
						mat4 T;
#if 0
						// Dumb linear interpolation of matrices
						for (int mi = 0; mi < 16; ++mi)
						{
							T.m[mi] = a.m[mi] * (1-lerpT) + b.m[mi] * lerpT;
						}
#else
						v3 aTranslation;
						v3 aScale;
						v4 aQuat;
						Mat4Decompose(a, &aTranslation, &aScale, &aQuat);

						v3 bTranslation;
						v3 bScale;
						v4 bQuat;
						Mat4Decompose(b, &bTranslation, &bScale, &bQuat);

						f32 aWeight = (1 - lerpT);
						f32 bWeight = lerpT;
						bool invertRot = V4Dot(aQuat, bQuat) < 0;

						v3 rTranslation = aTranslation * aWeight + bTranslation * lerpT;
						v3 rScale = aScale * aWeight + bScale * lerpT;
						v4 rQuat = aQuat * aWeight + bQuat * (invertRot ? -bWeight : bWeight);
						rQuat = V4Normalize(rQuat);

						T = Mat4Compose(rTranslation, rScale, rQuat);
#endif

						if (first)
						{
							transform = T;
							first = false;
						}
						else
						{
							transform = Mat4Multiply(T, transform);
						}
					}
					transform = Mat4Multiply(skinnedMesh.bindPoses[jointIdx], transform);

					joints[jointIdx] = transform;
				}
				gameState.animationTime += deltaTime;
				if (gameState.animationTime >= lastTimestamp)
				{
					if (gameState.loopAnimation)
						gameState.animationTime = 0;
					else
						gameState.animationTime = lastTimestamp;
				}

				Entity *player = gameState.player.entity;
				const v3 pos = player->pos;
				const v3 fw = player->fw;
				const v3 right = V3Cross(player->fw, {0,0,1});
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

				glUniformMatrix4fv(jointsUniform, 128, false, joints[0].m);

				glBindVertexArray(skinnedMesh.deviceMesh.vao);
				glDrawElements(GL_TRIANGLES, skinnedMesh.deviceMesh.indexCount, GL_UNSIGNED_SHORT, NULL);
			}

			SDL_GL_SwapWindow(window);
		}

		lastPerfCounter = newPerfCounter;
	}

	// Cleanup
	{
		FreeSkeletalMesh(&skinnedMesh);

		glDeleteBuffers(2, anvilMesh.buffers);
		glDeleteBuffers(2, gameState.levelGeometry.renderMesh.buffers);
		glDeleteBuffers(2, cubeMesh.buffers);
		glDeleteBuffers(2, skinnedMesh.deviceMesh.buffers);
		glDeleteVertexArrays(1, &anvilMesh.vao);
		glDeleteVertexArrays(1, &gameState.levelGeometry.renderMesh.vao);
		glDeleteVertexArrays(1, &cubeMesh.vao);
		glDeleteVertexArrays(1, &skinnedMesh.deviceMesh.vao);

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
