#include <stddef.h>
#include <memory.h>

#include "General.h"
#include "Maths.h"
#include "Geometry.h"
#include "Primitives.h"
#include "OpenGL.h"

#include "Platform.h"
#include "Game.h"

// Global platform function pointers
Log_t *Log;
PlatformReadEntireFile_t *PlatformReadEntireFile;
SetUpDevice_t *SetUpDevice;
ClearBuffers_t *ClearBuffers;
GetUniform_t *GetUniform;
UseProgram_t *UseProgram;
UniformMat4_t *UniformMat4;
RenderIndexedMesh_t *RenderIndexedMesh;
RenderMesh_t *RenderMesh;
CreateDeviceMesh_t *CreateDeviceMesh;
CreateDeviceIndexedMesh_t *CreateDeviceIndexedMesh;
CreateDeviceIndexedSkinnedMesh_t *CreateDeviceIndexedSkinnedMesh;
SendMesh_t *SendMesh;
SendIndexedMesh_t *SendIndexedMesh;
SendIndexedSkinnedMesh_t *SendIndexedSkinnedMesh;
LoadShader_t *LoadShader;
CreateDeviceProgram_t *CreateDeviceProgram;

#include "Memory.cpp"
#include "Collision.cpp"
#include "BakeryInterop.cpp"

NOMANGLE START_GAME(StartGame)
{
	frameMem = gameMemory->frameMem;
	stackMem = gameMemory->stackMem;
	transientMem = gameMemory->transientMem;

	framePtr = frameMem;
	stackPtr = stackMem;
	transientPtr = transientMem;

	// Platform functions
	Log = platformCode->Log;
	PlatformReadEntireFile = platformCode->PlatformReadEntireFile;
	SetUpDevice = platformCode->SetUpDevice;
	ClearBuffers = platformCode->ClearBuffers;
	GetUniform = platformCode->GetUniform;
	UseProgram = platformCode->UseProgram;
	UniformMat4 = platformCode->UniformMat4;
	RenderIndexedMesh = platformCode->RenderIndexedMesh;
	RenderMesh = platformCode->RenderMesh;
	CreateDeviceMesh = platformCode->CreateDeviceMesh;
	CreateDeviceIndexedMesh = platformCode->CreateDeviceIndexedMesh;
	CreateDeviceIndexedSkinnedMesh = platformCode->CreateDeviceIndexedSkinnedMesh;
	SendMesh = platformCode->SendMesh;
	SendIndexedMesh = platformCode->SendIndexedMesh;
	SendIndexedSkinnedMesh = platformCode->SendIndexedSkinnedMesh;
	LoadShader = platformCode->LoadShader;
	CreateDeviceProgram = platformCode->CreateDeviceProgram;

	Log("Starting game!\n");

	GameState *gameState = (GameState *)TransientAlloc(sizeof(GameState));

	// Initialize
	{
		SetUpDevice();

		// Anvil
		{
			Vertex *vertexData;
			u16 *indexData;
			u32 vertexCount;
			u32 indexCount;
			u8 *fileBuffer = PlatformReadEntireFile("data/anvil.bin");
			ReadMesh(fileBuffer, &vertexData, &indexData, &vertexCount, &indexCount);

			gameState->anvilMesh = CreateDeviceIndexedMesh();
			SendIndexedMesh(&gameState->anvilMesh, vertexData, vertexCount, indexData,
					indexCount, false);
		}

		// Cube
		{
			Vertex *vertexData;
			u16 *indexData;
			u32 vertexCount;
			u32 indexCount;
			u8 *fileBuffer = PlatformReadEntireFile("data/cube.bin");
			ReadMesh(fileBuffer, &vertexData, &indexData, &vertexCount, &indexCount);

			gameState->cubeMesh = CreateDeviceIndexedMesh();
			SendIndexedMesh(&gameState->cubeMesh, vertexData, vertexCount, indexData,
					indexCount, false);
		}

		// Sphere
		{
			Vertex *vertexData;
			u16 *indexData;
			u32 vertexCount;
			u32 indexCount;
			u8 *fileBuffer = PlatformReadEntireFile("data/sphere.bin");
			ReadMesh(fileBuffer, &vertexData, &indexData, &vertexCount, &indexCount);

			gameState->sphereMesh = CreateDeviceIndexedMesh();
			SendIndexedMesh(&gameState->sphereMesh, vertexData, vertexCount, indexData,
					indexCount, false);
		}

		// Cylinder
		{
			Vertex *vertexData;
			u16 *indexData;
			u32 vertexCount;
			u32 indexCount;
			u8 *fileBuffer = PlatformReadEntireFile("data/cylinder.bin");
			ReadMesh(fileBuffer, &vertexData, &indexData, &vertexCount, &indexCount);

			gameState->cylinderMesh = CreateDeviceIndexedMesh();
			SendIndexedMesh(&gameState->cylinderMesh, vertexData, vertexCount, indexData,
					indexCount, false);
		}

		// Capsule
		{
			Vertex *vertexData;
			u16 *indexData;
			u32 vertexCount;
			u32 indexCount;
			u8 *fileBuffer = PlatformReadEntireFile("data/capsule.bin");
			ReadMesh(fileBuffer, &vertexData, &indexData, &vertexCount, &indexCount);

			gameState->capsuleMesh = CreateDeviceIndexedMesh();
			SendIndexedMesh(&gameState->capsuleMesh, vertexData, vertexCount, indexData,
					indexCount, false);
		}

		// Debug geometry buffer
		{
			debugGeometryBuffer.vertexData = (Vertex *)TransientAlloc(2048 * sizeof(Vertex));
			debugGeometryBuffer.vertexCount = 0;
			debugGeometryBuffer.deviceMesh = CreateDeviceMesh();
		}

		// Skinned mesh
		{
			SkeletalMesh *skinnedMesh = &gameState->skinnedMesh;
			SkinnedVertex *vertexData;
			u16 *indexData;
			u32 vertexCount;
			u32 indexCount;
			u8 *fileBuffer = PlatformReadEntireFile("data/Sparkus.bin");
			ReadSkinnedMesh(fileBuffer, skinnedMesh, &vertexData, &indexData, &vertexCount,
					&indexCount);

			gameState->skinnedMesh.deviceMesh = CreateDeviceIndexedSkinnedMesh();
			SendIndexedSkinnedMesh(&gameState->skinnedMesh.deviceMesh, vertexData, vertexCount,
					indexData, indexCount, false);
		}

		// Shaders
		DeviceShader vertexShader = LoadShader(vertexShaderSource, SHADERTYPE_VERTEX);
		DeviceShader fragmentShader = LoadShader(fragShaderSource, SHADERTYPE_FRAGMENT);
		gameState->program = CreateDeviceProgram(&vertexShader, &fragmentShader);

		DeviceShader skinVertexShader = LoadShader(skinVertexShaderSource, SHADERTYPE_VERTEX);
		DeviceShader skinFragmentShader = fragmentShader;
		gameState->skinnedMeshProgram = CreateDeviceProgram(&skinVertexShader, &skinFragmentShader);

#if defined(DEBUG_BUILD)
		DeviceShader debugDrawVertexShader = vertexShader;
		DeviceShader debugDrawFragmentShader = LoadShader(debugDrawFragShaderSource,
				SHADERTYPE_FRAGMENT);

		gameState->debugDrawProgram = CreateDeviceProgram(&debugDrawVertexShader,
				&debugDrawFragmentShader);
#endif

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

		UseProgram(&gameState->program);
		DeviceUniform projUniform = GetUniform(&gameState->program, "projection");
		UniformMat4(&projUniform, 1, proj.m);

		UseProgram(&gameState->skinnedMeshProgram);
		projUniform = GetUniform(&gameState->skinnedMeshProgram, "projection");
		UniformMat4(&projUniform, 1, proj.m);

#if DEBUG_BUILD
		UseProgram(&gameState->debugDrawProgram);
		projUniform = GetUniform(&gameState->debugDrawProgram, "projection");
		UniformMat4(&projUniform, 1, proj.m);
#endif
	}

	// Init level
	{
		Vertex *vertexData;
		u16 *indexData;
		u32 vertexCount;
		u32 indexCount;
		u8 *fileBuffer = PlatformReadEntireFile("data/level_graphics.bin");
		ReadMesh(fileBuffer, &vertexData, &indexData, &vertexCount, &indexCount);

		DeviceMesh *levelMesh = &gameState->levelGeometry.renderMesh;
		*levelMesh = CreateDeviceIndexedMesh();
		SendIndexedMesh(levelMesh, vertexData, vertexCount, indexData, indexCount, false);

		Triangle *triangleData;
		u32 triangleCount;

		fileBuffer = PlatformReadEntireFile("data/level.bin");
		ReadTriangleGeometry(fileBuffer, &triangleData, &triangleCount);
		gameState->levelGeometry.triangles = triangleData;
		gameState->levelGeometry.triangleCount = triangleCount;
	}

	// Init player
	{
		Entity *playerEnt = &gameState->entities[gameState->entityCount++];
		gameState->player.entity = playerEnt;
		playerEnt->fw = { 0.0f, 1.0f, 0.0f };
		playerEnt->mesh = 0;

		playerEnt->collider.type = COLLIDER_CAPSULE;
		playerEnt->collider.capsule.radius = 0.5;
		playerEnt->collider.capsule.height = 1.0f;
		playerEnt->collider.capsule.offset = { 0, 0, 1 };

		gameState->animationIdx = 0;
		gameState->loopAnimation = true;
	}

	// Init camera
	gameState->camPitch = -PI * 0.35f;

	// Test entities
	{
		Collider collider;
		collider.type = COLLIDER_CONVEX_HULL;
		u8 *fileBuffer = PlatformReadEntireFile("data/anvil_collision.bin");
		ReadPoints(fileBuffer, &collider.convexHull.collisionPoints,
				&collider.convexHull.collisionPointCount);

		Entity *testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { -6.0f, 3.0f, 1.0f };
		testEntity->fw = { 0.0f, 1.0f, 0.0f };
		testEntity->mesh = &gameState->anvilMesh;
		testEntity->collider = collider;

		testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { 5.0f, 4.0f, 1.0f };
		testEntity->fw = { 0.0f, 1.0f, 0.0f };
		testEntity->mesh = &gameState->anvilMesh;
		testEntity->collider = collider;

		testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { 3.0f, -4.0f, 1.0f };
		testEntity->fw = { 0.707f, 0.707f, 0.0f };
		testEntity->mesh = &gameState->anvilMesh;
		testEntity->collider = collider;

		testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { -8.0f, -4.0f, 1.0f };
		testEntity->fw = { 0.707f, 0.0f, 0.707f };
		testEntity->mesh = &gameState->anvilMesh;
		testEntity->collider = collider;

		testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { -6.0f, 7.0f, 1.0f };
		testEntity->fw = { 0.0f, 1.0f, 0.0f };
		testEntity->mesh = &gameState->sphereMesh;
		testEntity->collider.type = COLLIDER_SPHERE;
		testEntity->collider.sphere.radius = 1;
		testEntity->collider.sphere.offset = {};

		testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { -3.0f, 7.0f, 1.0f };
		testEntity->fw = { 0.0f, 1.0f, 0.0f };
		testEntity->mesh = &gameState->cylinderMesh;
		testEntity->collider.type = COLLIDER_CYLINDER;
		testEntity->collider.cylinder.radius = 1;
		testEntity->collider.cylinder.height = 2;
		testEntity->collider.cylinder.offset = {};

		testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { 0.0f, 7.0f, 2.0f };
		testEntity->fw = { 0.0f, 1.0f, 0.0f };
		testEntity->mesh = &gameState->capsuleMesh;
		testEntity->collider.type = COLLIDER_CAPSULE;
		testEntity->collider.capsule.radius = 1;
		testEntity->collider.capsule.height = 2;
		testEntity->collider.capsule.offset = {};
	}
}

NOMANGLE UPDATE_AND_RENDER_GAME(UpdateAndRenderGame)
{
	GameState *gameState = (GameState *)transientMem;

#if DEBUG_BUILD
	debugCubeCount = 0;
#endif

	// Update
	{
#if GJK_VISUAL_DEBUGGING || EPA_VISUAL_DEBUGGING
		if (controller->debugUp.endedDown && controller->debugUp.changed)
		{
			g_currentPolytopeStep++;
			if (g_currentPolytopeStep >= 16)
				g_currentPolytopeStep = 16;
		}
		if (controller->debugDown.endedDown && controller->debugDown.changed)
		{
			g_currentPolytopeStep--;
			if (g_currentPolytopeStep < 0)
				g_currentPolytopeStep = 0;
		}
		DRAW_AA_DEBUG_CUBE(v3{}, 0.05f); // Draw origin
#endif

		if (controller->debugUp.endedDown && controller->debugUp.changed)
		{
			++gameState->animationIdx;
			if (gameState->animationIdx >= (i32)gameState->skinnedMesh.animationCount)
				gameState->animationIdx = 0;
		}
		if (controller->debugDown.endedDown && controller->debugDown.changed)
		{
			--gameState->animationIdx;
			if (gameState->animationIdx < 0)
				gameState->animationIdx = gameState->skinnedMesh.animationCount - 1;
		}

		if (controller->camUp.endedDown)
			gameState->camPitch += 1.0f * deltaTime;
		else if (controller->camDown.endedDown)
			gameState->camPitch -= 1.0f * deltaTime;

		if (controller->camLeft.endedDown)
			gameState->camYaw -= 1.0f * deltaTime;
		else if (controller->camRight.endedDown)
			gameState->camYaw += 1.0f * deltaTime;

		// Move player
		Player *player = &gameState->player;
		{
			v3 worldInputDir = {};
			if (controller->up.endedDown)
				worldInputDir += { Sin(gameState->camYaw), Cos(gameState->camYaw) };
			else if (controller->down.endedDown)
				worldInputDir += { -Sin(gameState->camYaw), -Cos(gameState->camYaw) };

			if (controller->left.endedDown)
				worldInputDir += { -Cos(gameState->camYaw), Sin(gameState->camYaw) };
			else if (controller->right.endedDown)
				worldInputDir += { Cos(gameState->camYaw), -Sin(gameState->camYaw) };

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
			if (player->state == PLAYERSTATE_GROUNDED && controller->jump.endedDown &&
					controller->jump.changed)
			{
				player->vel.z = 15.0f;
				player->state = PLAYERSTATE_AIR;

				gameState->animationIdx = 1;
				gameState->animationTime = 0;
				gameState->loopAnimation = false;
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

		for (u32 entityIndex = 0; entityIndex < gameState->entityCount; ++ entityIndex)
		{
			Entity *entity = &gameState->entities[entityIndex];
			if (entity != gameState->player.entity)
			{
				GJKResult gjkResult = GJKTest(player->entity, entity);
				if (gjkResult.hit)
				{
					v3 depenetration = ComputeDepenetration(gjkResult, player->entity, entity);
					player->entity->pos += depenetration;
					// TOFIX handle velocity change upon hits properly
					if (V3Normalize(depenetration).z > 0.9f)
						touchedGround = true;
					break;
				}
			}
		}

		// Ray testing
		{
			LevelGeometry *level = &gameState->levelGeometry;
			for (u32 triIdx = 0; triIdx < level->triangleCount; ++triIdx)
			{
				v3 origin = player->entity->pos + v3{ 0, 0, 1 };
				v3 dir = { 0, 0, -1.1f };
				Triangle &triangle = level->triangles[triIdx];
				v3 hit;
				if (RayTriangleIntersection(origin, dir, triangle, &hit))
				{
					player->entity->pos.z = hit.z;
					touchedGround = true;
					break;
				}
			}
		}

		PlayerState newState = touchedGround ? PLAYERSTATE_GROUNDED : PLAYERSTATE_AIR;
		if (newState != player->state)
		{
			gameState->animationIdx = newState == PLAYERSTATE_GROUNDED ? 0 : 1;
			gameState->animationTime = 0;
			gameState->loopAnimation = newState == PLAYERSTATE_GROUNDED ? true : false;
			player->state = newState;
		}

		gameState->camPos = player->entity->pos;

#if GJK_VISUAL_DEBUGGING
		debugGeometryBuffer.vertexCount = 0;
		Vertex *gjkVertices;
		u32 gjkFaceCount;
		GetGJKStepGeometry(g_currentPolytopeStep, &gjkVertices, &gjkFaceCount);
		DrawDebugTriangles(gjkVertices, gjkFaceCount);

		DRAW_AA_DEBUG_CUBE(g_GJKNewPoint[g_currentPolytopeStep], 0.03f);
#endif
#if EPA_VISUAL_DEBUGGING
		Vertex *epaVertices;
		u32 epaFaceCount;
		GetEPAStepGeometry(g_currentPolytopeStep, &epaVertices, &epaFaceCount);
		DrawDebugTriangles(epaVertices, epaFaceCount * 3);

		DRAW_AA_DEBUG_CUBE(g_epaNewPoint[g_currentPolytopeStep], 0.03f);
#endif
	}

	// Draw
	{
		// View matrix
		mat4 view;
		{
			view = Mat4Translation({ 0.0f, 0.0f, -5.0f });
			const f32 pitch = gameState->camPitch;
			mat4 pitchMatrix =
			{
				1.0f,	0.0f,			0.0f,		0.0f,
				0.0f,	Cos(pitch),		Sin(pitch),	0.0f,
				0.0f,	-Sin(pitch),	Cos(pitch),	0.0f,
				0.0f,	0.0f,			0.0f,		1.0f
			};
			view = Mat4Multiply(pitchMatrix, view);

			const f32 yaw = gameState->camYaw;
			mat4 yawMatrix =
			{
				Cos(yaw),	Sin(yaw),	0.0f,	0.0f,
				-Sin(yaw),	Cos(yaw),	0.0f,	0.0f,
				0.0f,		0.0f,		1.0f,	0.0f,
				0.0f,		0.0f,		0.0f,	1.0f
			};
			view = Mat4Multiply(yawMatrix, view);

			const v3 pos = gameState->camPos;
			mat4 camPosMatrix = Mat4Translation(-pos);
			view = Mat4Multiply(camPosMatrix, view);
		}

		UseProgram(&gameState->program);
		DeviceUniform viewUniform = GetUniform(&gameState->program, "view");
		UniformMat4(&viewUniform, 1, view.m);

		DeviceUniform modelUniform = GetUniform(&gameState->program, "model");

		const v4 clearColor = { 0.95f, 0.88f, 0.05f, 1.0f };
		ClearBuffers(clearColor);

		// Entity
		for (u32 entityIdx = 0; entityIdx < gameState->entityCount; ++entityIdx)
		{
			Entity *entity = &gameState->entities[entityIdx];
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
			UniformMat4(&modelUniform, 1, model.m);

			RenderIndexedMesh(entity->mesh);
		}

		// Level
		{
			LevelGeometry *level = &gameState->levelGeometry;

			UniformMat4(&modelUniform, 1, MAT4_IDENTITY.m);
			RenderIndexedMesh(&level->renderMesh);
		}

		// Skinned meshes
		UseProgram(&gameState->skinnedMeshProgram);
		viewUniform = GetUniform(&gameState->skinnedMeshProgram, "view");
		modelUniform = GetUniform(&gameState->skinnedMeshProgram, "model");

		DeviceUniform jointsUniform = GetUniform(&gameState->skinnedMeshProgram, "joints");
		UniformMat4(&viewUniform, 1, view.m);
		{
			UniformMat4(&modelUniform, 1, MAT4_IDENTITY.m);

			mat4 joints[128];
			for (int i = 0; i < 128; ++i)
			{
				joints[i] = MAT4_IDENTITY;
			}

			SkeletalMesh *skinnedMesh = &gameState->skinnedMesh;

			Animation *animation = &skinnedMesh->animations[gameState->animationIdx];

			f32 lastTimestamp = animation->timestamps[animation->frameCount - 1];

			int currentFrame = -1;
			for (u32 i = 0; i < animation->frameCount; ++i)
			{
				if (currentFrame == -1 && gameState->animationTime <= animation->timestamps[i])
					currentFrame = i - 1;
			}
			f32 prevStamp = animation->timestamps[currentFrame];
			f32 nextStamp = animation->timestamps[currentFrame + 1];
			f32 lerpWindow = nextStamp - prevStamp;
			f32 lerpTime = gameState->animationTime - prevStamp;
			f32 lerpT = lerpTime / lerpWindow;

			for (u32 jointIdx = 0; jointIdx < skinnedMesh->jointCount; ++jointIdx)
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
				int stackCount = 1;
				stack[0] = jointIdx;

				u8 parentJoint = skinnedMesh->jointParents[jointIdx];
				while (parentJoint != 0xFF)
				{
					stack[stackCount++] = parentJoint;
					parentJoint = skinnedMesh->jointParents[parentJoint];
				}
				bool first = true;
				while (stackCount)
				{
					const u32 currentJoint = stack[--stackCount];

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
						transform = Mat4Multiply(skinnedMesh->restPoses[currentJoint], transform);
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
				transform = Mat4Multiply(skinnedMesh->bindPoses[jointIdx], transform);

				joints[jointIdx] = transform;
			}
			gameState->animationTime += deltaTime;
			if (gameState->animationTime >= lastTimestamp)
			{
				if (gameState->loopAnimation)
					gameState->animationTime = 0;
				else
					gameState->animationTime = lastTimestamp;
			}

			Entity *player = gameState->player.entity;
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
			UniformMat4(&modelUniform, 1, model.m);

			UniformMat4(&jointsUniform, 128, joints[0].m);

			RenderIndexedMesh(&skinnedMesh->deviceMesh);
		}

#if DEBUG_BUILD
		UseProgram(&gameState->debugDrawProgram);
		viewUniform = GetUniform(&gameState->debugDrawProgram, "view");
		modelUniform = GetUniform(&gameState->debugDrawProgram, "model");

		UniformMat4(&viewUniform, 1, view.m);
		// Debug draws
		{
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
				UniformMat4(&modelUniform, 1, model.m);
				RenderIndexedMesh(&gameState->cubeMesh);
			}
		}

		// Debug meshes
		{
			UniformMat4(&modelUniform, 1, MAT4_IDENTITY.m);

			SendMesh(&debugGeometryBuffer.deviceMesh, debugGeometryBuffer.vertexData,
					debugGeometryBuffer.vertexCount, true);

			RenderMesh(&debugGeometryBuffer.deviceMesh);

			debugGeometryBuffer.vertexCount = 0;
		}
#endif
	}
}

void CleanupGame(GameState *gameState)
{
	(void) gameState;
	// Cleanup
	// @Cleanup: properly free device resources

	return;
}
