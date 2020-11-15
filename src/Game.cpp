#include <stddef.h>
#include <memory.h>

#ifdef USING_IMGUI
#include <imgui/imgui.h>
#ifdef IMGUI_SHOW_DEMO
#include <imgui/imgui_demo.cpp> // @Todo: remove
#endif
#endif

#include "General.h"
#include "Maths.h"
#include "MemoryAlloc.h"
#include "Render.h"
#include "Geometry.h"
#include "Platform.h"
#include "Resource.h"
#include "PlatformCode.h"
#include "Containers.h"
#include "Game.h"

Memory *g_memory;
#if DEBUG_BUILD
DebugContext *g_debugContext;
#endif

DECLARE_ARRAY(u32);

#include "PlatformCodeLoad.cpp"
#include "DebugDraw.cpp"
#include "MemoryAlloc.cpp"
#include "Collision.cpp"
#include "BakeryInterop.cpp"
#include "Resource.cpp"
#ifdef USING_IMGUI
#include "Imgui.cpp"
#endif

#if TARGET_WINDOWS
#define GAMEDLL NOMANGLE __declspec(dllexport)
#else
#define GAMEDLL NOMANGLE __attribute__((visibility("default")))
#endif

GAMEDLL INIT_GAME_MODULE(InitGameModule)
{
#ifdef USING_IMGUI
	ImGui::SetAllocatorFunctions(BuddyAlloc, BuddyFree);
	ImGui::SetCurrentContext(platformContext.imguiContext);
#endif

	// Handy global pointers
	g_memory = platformContext.memory;
#if DEBUG_BUILD
	g_debugContext = (DebugContext *)((u8 *)g_memory->transientMem + sizeof(GameState));
#endif

	ImportPlatformCodeFromStruct(platformContext.platformCode);
}

GAMEDLL GAME_RESOURCE_POST_LOAD(GameResourcePostLoad)
{
	switch(resource->type)
	{
	case RESOURCETYPE_MESH:
	{
		ResourceLoadMesh(resource, fileBuffer, initialize);
		return true;
	} break;
	case RESOURCETYPE_SKINNEDMESH:
	{
		ResourceLoadSkinnedMesh(resource, fileBuffer, initialize);
		return true;
	} break;
	case RESOURCETYPE_LEVELGEOMETRYGRID:
	{
		ResourceLoadLevelGeometryGrid(resource, fileBuffer, initialize);
		return true;
	} break;
	case RESOURCETYPE_COLLISIONMESH:
	{
		ResourceLoadCollisionMesh(resource, fileBuffer, initialize);
		return true;
	} break;
	case RESOURCETYPE_TEXTURE:
	{
		ResourceLoadTexture(resource, fileBuffer, initialize);
		return true;
	} break;
	case RESOURCETYPE_SHADER:
	{
		ResourceLoadShader(resource, fileBuffer, initialize);
		return true;
	} break;
	}
	return false;
}

GAMEDLL START_GAME(StartGame)
{
	ASSERT(g_memory->transientMem == g_memory->transientPtr);
	GameState *gameState = (GameState *)TransientAlloc(sizeof(GameState));
#if DEBUG_BUILD
	g_debugContext = (DebugContext *)TransientAlloc(sizeof(DebugContext));
#endif

	gameState->timeMultiplier = 1.0f;
	gameState->entityCount = 0;
	gameState->skinnedMeshCount = 0;

	// Initialize
	{
		SetUpDevice();

		LoadResource(RESOURCETYPE_MESH, "data/anvil.b");
		LoadResource(RESOURCETYPE_MESH, "data/cube.b");
		LoadResource(RESOURCETYPE_MESH, "data/sphere.b");
		LoadResource(RESOURCETYPE_MESH, "data/cylinder.b");
		LoadResource(RESOURCETYPE_MESH, "data/capsule.b");
		LoadResource(RESOURCETYPE_MESH, "data/level_graphics.b");
		LoadResource(RESOURCETYPE_SKINNEDMESH, "data/Sparkus.b");
		LoadResource(RESOURCETYPE_SKINNEDMESH, "data/Jumper.b");
		LoadResource(RESOURCETYPE_LEVELGEOMETRYGRID, "data/level.b");
		LoadResource(RESOURCETYPE_COLLISIONMESH, "data/anvil_collision.b");

		const Resource *texAlb = LoadResource(RESOURCETYPE_TEXTURE, "data/sparkus_albedo.b");
		const Resource *texNor = LoadResource(RESOURCETYPE_TEXTURE, "data/sparkus_normal.b");
		BindTexture(texAlb->texture.deviceTexture, 0);
		BindTexture(texNor->texture.deviceTexture, 1);

		LoadResource(RESOURCETYPE_TEXTURE, "data/grid.b");
		LoadResource(RESOURCETYPE_TEXTURE, "data/normal_plain.b");

#if DEBUG_BUILD
		// Debug geometry buffer
		{
			int attribs = RENDERATTRIB_POSITION | RENDERATTRIB_COLOR;
			DebugGeometryBuffer *dgb = &g_debugContext->debugGeometryBuffer;
			dgb->triangleData = (DebugVertex *)TransientAlloc(2048 * sizeof(DebugVertex));
			dgb->lineData = (DebugVertex *)TransientAlloc(2048 * sizeof(DebugVertex));
			dgb->debugCubeCount = 0;
			dgb->triangleVertexCount = 0;
			dgb->lineVertexCount = 0;
			dgb->deviceMesh = CreateDeviceMesh(attribs);
			dgb->cubePositionsBuffer = CreateDeviceMesh(attribs);
		}

		// Send the cube mesh that gets instanced
		{
			u8 *fileBuffer;
			u64 fileSize;
			bool success = PlatformReadEntireFile("data/cube.b", &fileBuffer,
					&fileSize, FrameAlloc);
			ASSERT(success);

			Vertex *vertexData;
			u16 *indexData;
			u32 vertexCount;
			u32 indexCount;
			ReadMesh(fileBuffer, &vertexData, &indexData, &vertexCount, &indexCount);

			// Keep only positions
			v3 *positionBuffer = (v3 *)FrameAlloc(sizeof(v3) * vertexCount);
			for (u32 i = 0; i < vertexCount; ++i)
			{
				positionBuffer[i] = vertexData[i].pos;
			}

			int attribs = RENDERATTRIB_POSITION;
			g_debugContext->debugGeometryBuffer.cubeMesh = CreateDeviceIndexedMesh(attribs);
			SendIndexedMesh(&g_debugContext->debugGeometryBuffer.cubeMesh, positionBuffer,
					vertexCount, sizeof(v3), indexData, indexCount, false);
		}
#endif

		// Shaders
		const Resource *shaderRes = LoadResource(RESOURCETYPE_SHADER, "data/shaders/shader_general.b");
		gameState->program = shaderRes->shader.programHandle;

		const Resource *shaderSkinnedRes = LoadResource(RESOURCETYPE_SHADER, "data/shaders/shader_skinned.b");
		gameState->skinnedMeshProgram = shaderSkinnedRes->shader.programHandle;

#if DEBUG_BUILD
		const Resource *shaderDebugRes = LoadResource(RESOURCETYPE_SHADER, "data/shaders/shader_debug.b");
		g_debugContext->debugDrawProgram = shaderDebugRes->shader.programHandle;

		const Resource *shaderDebugCubesRes = LoadResource(RESOURCETYPE_SHADER, "data/shaders/shader_debug_cubes.b");
		g_debugContext->debugCubesProgram = shaderDebugCubesRes->shader.programHandle;
#endif
	}

	// Init level
	{
		const Resource *levelGraphicsRes = GetResource("data/level_graphics.b");
		gameState->levelGeometry.renderMesh = levelGraphicsRes;

		const Resource *levelCollisionRes = GetResource("data/level.b");
		gameState->levelGeometry.geometryGrid = levelCollisionRes;
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

		gameState->player.state = PLAYERSTATE_IDLE;

		SkinnedMeshInstance *skinnedMeshInstance =
			&gameState->skinnedMeshInstances[gameState->skinnedMeshCount++];
		skinnedMeshInstance->entityHandle = gameState->entityCount - 1;
		skinnedMeshInstance->meshRes = GetResource("data/Sparkus.b");
		skinnedMeshInstance->animationIdx = PLAYERANIM_IDLE;
		skinnedMeshInstance->animationTime = 0;
		playerEnt->skinnedMeshInstance = skinnedMeshInstance;
	}

	// Init camera
	gameState->camPitch = -PI * 0.35f;

	// Test entities
	{
		Collider collider;
		collider.type = COLLIDER_CONVEX_HULL;

		const Resource *collMeshRes = GetResource("data/anvil_collision.b");
		collider.convexHull.meshRes = collMeshRes;

		const Resource *anvilRes = GetResource("data/anvil.b");

		Entity *testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { -6.0f, 3.0f, 1.0f };
		testEntity->fw = { 0.0f, 1.0f, 0.0f };
		testEntity->mesh = anvilRes;
		testEntity->collider = collider;

		testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { 5.0f, 4.0f, 1.0f };
		testEntity->fw = { 0.0f, 1.0f, 0.0f };
		testEntity->mesh = anvilRes;
		testEntity->collider = collider;

		testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { 3.0f, -4.0f, 1.0f };
		testEntity->fw = { 0.707f, 0.707f, 0.0f };
		testEntity->mesh = anvilRes;
		testEntity->collider = collider;

		testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { -8.0f, -4.0f, 1.0f };
		testEntity->fw = { 0.707f, 0.0f, 0.707f };
		testEntity->mesh = anvilRes;
		testEntity->collider = collider;

		const Resource *sphereRes = GetResource("data/sphere.b");
		testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { -6.0f, 7.0f, 1.0f };
		testEntity->fw = { 0.0f, 1.0f, 0.0f };
		testEntity->mesh = sphereRes;
		testEntity->collider.type = COLLIDER_SPHERE;
		testEntity->collider.sphere.radius = 1;
		testEntity->collider.sphere.offset = {};

		const Resource *cylinderRes = GetResource("data/cylinder.b");
		testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { -3.0f, 7.0f, 1.0f };
		testEntity->fw = { 0.0f, 1.0f, 0.0f };
		testEntity->mesh = cylinderRes;
		testEntity->collider.type = COLLIDER_CYLINDER;
		testEntity->collider.cylinder.radius = 1;
		testEntity->collider.cylinder.height = 2;
		testEntity->collider.cylinder.offset = {};

		const Resource *capsuleRes = GetResource("data/capsule.b");
		testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { 0.0f, 7.0f, 2.0f };
		testEntity->fw = { 0.0f, 1.0f, 0.0f };
		testEntity->mesh = capsuleRes;
		testEntity->collider.type = COLLIDER_CAPSULE;
		testEntity->collider.capsule.radius = 1;
		testEntity->collider.capsule.height = 2;
		testEntity->collider.capsule.offset = {};

		testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { 3.0f, 4.0f, 0.0f };
		testEntity->fw = { 0.0f, 1.0f, 0.0f };
		testEntity->mesh = nullptr;
		testEntity->collider.type = COLLIDER_SPHERE;
		testEntity->collider.sphere.radius = 0.3f;
		testEntity->collider.sphere.offset = { 0, 0, 1 };
		SkinnedMeshInstance *skinnedMeshInstance =
			&gameState->skinnedMeshInstances[gameState->skinnedMeshCount++];
		skinnedMeshInstance->entityHandle = gameState->entityCount - 1;
		skinnedMeshInstance->meshRes = GetResource("data/Jumper.b");
		skinnedMeshInstance->animationIdx = 0;
		skinnedMeshInstance->animationTime = 0;
		testEntity->skinnedMeshInstance = skinnedMeshInstance;
	}
}

void ChangeState(GameState *gameState, PlayerState newState, PlayerAnim newAnim)
{
	//Log("Changed state: 0x%X -> 0x%X\n", gameState->player.state, newState);
	gameState->player.entity->skinnedMeshInstance->animationIdx = newAnim;
	gameState->player.entity->skinnedMeshInstance->animationTime = 0;
	gameState->player.state = newState;
}

GAMEDLL UPDATE_AND_RENDER_GAME(UpdateAndRenderGame)
{
	GameState *gameState = (GameState *)g_memory->transientMem;

	deltaTime *= gameState->timeMultiplier;

#ifdef USING_IMGUI

#ifdef IMGUI_SHOW_DEMO
	ImGui::ShowDemoWindow();
#endif

	ImguiShowDebugWindow(gameState);
#endif

	if (deltaTime < 0 || deltaTime > 1)
	{
		Log("WARNING: Delta time out of range! %f\n", deltaTime);
		deltaTime = 1 / 60.0f;
	}

	// Update
	{
		// Move camera
		{
			const f32 camRotSpeed = 2.0f;

			if (controller->camUp.endedDown)
				gameState->camPitch += camRotSpeed * deltaTime;
			else if (controller->camDown.endedDown)
				gameState->camPitch -= camRotSpeed * deltaTime;

			if (controller->camLeft.endedDown)
				gameState->camYaw -= camRotSpeed * deltaTime;
			else if (controller->camRight.endedDown)
				gameState->camYaw += camRotSpeed * deltaTime;

			const f32 deadzone = 0.2f;
			if (Abs(controller->rightStick.x) > deadzone)
				gameState->camYaw -= controller->rightStick.x * camRotSpeed * deltaTime;
			if (Abs(controller->rightStick.y) > deadzone)
				gameState->camPitch += controller->rightStick.y * camRotSpeed * deltaTime;
		}

		// Move player
		Player *player = &gameState->player;
		{
			v2 inputDir = {};
			if (controller->up.endedDown)
				inputDir.y = 1;
			else if (controller->down.endedDown)
				inputDir.y = -1;

			if (controller->left.endedDown)
				inputDir.x = -1;
			else if (controller->right.endedDown)
				inputDir.x = 1;

			if (inputDir.x + inputDir.y == 0)
			{
				const f32 deadzone = 0.2f;
				if (Abs(controller->leftStick.x) > deadzone)
					inputDir.x = (controller->leftStick.x - deadzone) / (1 - deadzone);
				if (Abs(controller->leftStick.y) > deadzone)
					inputDir.y = (controller->leftStick.y - deadzone) / (1 - deadzone);
			}

			v3 worldInputDir =
			{
				inputDir.x * Cos(gameState->camYaw) + inputDir.y * Sin(gameState->camYaw),
				-inputDir.x * Sin(gameState->camYaw) + inputDir.y * Cos(gameState->camYaw),
				0
			};

			f32 sqlen = V3SqrLen(worldInputDir);
			if (sqlen > 1)
			{
				worldInputDir /= Sqrt(sqlen);
				player->entity->fw = worldInputDir;
			}
			else if (sqlen)
			{
				player->entity->fw = worldInputDir / Sqrt(sqlen);
			}

			const f32 playerSpeed = 5.0f;
			player->entity->pos += worldInputDir * playerSpeed * deltaTime;

			if (!(player->state & PLAYERSTATEFLAG_AIRBORNE))
			{
				if (player->state != PLAYERSTATE_RUNNING && V3SqrLen(worldInputDir))
				{
					ChangeState(gameState, PLAYERSTATE_RUNNING, PLAYERANIM_RUN);
				}
				else if (player->state == PLAYERSTATE_RUNNING && !V3SqrLen(worldInputDir))
				{
					ChangeState(gameState, PLAYERSTATE_IDLE, PLAYERANIM_IDLE);
				}
			}

			// Jump
			if (!(player->state & PLAYERSTATEFLAG_AIRBORNE) && controller->jump.endedDown &&
					controller->jump.changed)
			{
				player->vel.z = 15.0f;
				ChangeState(gameState, PLAYERSTATE_JUMP, PLAYERANIM_JUMP);
			}

			// Gravity
			if (player->state & PLAYERSTATEFLAG_AIRBORNE)
			{
				player->vel.z -= 30.0f * deltaTime;
				if (player->vel.z < -75.0f)
					player->vel.z = -75.0f;
			}
			else
			{
				player->vel.z = 0;
			}

			player->entity->pos += player->vel * deltaTime;
		}

		const f32 groundRayDist = (player->state & PLAYERSTATEFLAG_AIRBORNE) ? 1.0f : 1.5f;

		// Collision
		bool touchedGround = false;

		for (u32 entityIndex = 0; entityIndex < gameState->entityCount; ++ entityIndex)
		{
			Entity *entity = &gameState->entities[entityIndex];
			if (entity != gameState->player.entity)
			{
				v3 origin = player->entity->pos + v3{ 0, 0, 1 };
				v3 dir = { 0, 0, -groundRayDist };
				v3 hit;
				v3 hitNor;
				if (RayColliderIntersection(origin, dir, entity, &hit, &hitNor))
				{
					if (hitNor.z > 0.7f)
					{
						player->entity->pos.z = hit.z;
						touchedGround = true;
						break;
					}
				}

				GJKResult gjkResult = GJKTest(player->entity, entity);
				if (gjkResult.hit)
				{
					v3 depenetration = ComputeDepenetration(gjkResult, player->entity, entity);
					// @Hack: ignoring depenetration when it's too big
					if (V3Length(depenetration) < 2.0f)
					{
						player->entity->pos += depenetration;
					}
					else
					{
						Log("WARNING! Ignoring huge depenetration vector... something went wrong!\n");
					}

					break;
				}
			}
		}

		// Ray testing
		{
			v3 origin = player->entity->pos + v3{ 0, 0, 1 };
			v3 dir = { 0, 0, -groundRayDist };
			v3 hit;
			Triangle triangle;
			if (HitTest(gameState, origin, dir, &hit, &triangle))
			{
				player->entity->pos.z = hit.z;
				touchedGround = true;
			}

			origin = player->entity->pos + v3{ 0, 0, 1 };
			const f32 playerRadius = 0.5f;
			const f32 rayLen = playerRadius * 1.414f;
			v3 dirs[] =
			{
				{ 0, rayLen, 0 },
				{ 0, -rayLen, 0 },
				{ rayLen, 0, 0 },
				{ -rayLen, 0, 0 },
			};
			for (u32 i = 0; i < ArrayCount(dirs); ++i)
			{
				if (HitTest(gameState, origin, dirs[i], &hit, &triangle))
				{
					f32 dot = V3Dot(triangle.normal, dirs[i] / rayLen);
					if (dot > -0.707f && dot < 0.707f)
						continue;

					hit.z = player->entity->pos.z;

					f32 aDistAlongNormal = V3Dot(triangle.a - player->entity->pos, triangle.normal);
					if (aDistAlongNormal < 0) aDistAlongNormal = -aDistAlongNormal;
					if (aDistAlongNormal < playerRadius)
					{
						f32 factor = playerRadius - aDistAlongNormal;
						player->entity->pos += triangle.normal * factor;
					}
				}
			}
		}

		bool wasAirborne = player->state & PLAYERSTATEFLAG_AIRBORNE;
		if (wasAirborne && touchedGround)
		{
			ChangeState(gameState, PLAYERSTATE_LAND, PLAYERANIM_LAND);
		}
		else if (!wasAirborne && !touchedGround)
		{
			ChangeState(gameState, PLAYERSTATE_FALL, PLAYERANIM_FALL);
		}

		if (player->entity->pos.z < -1)
		{
			player->entity->pos.z = 1;
			player->vel = {};
		}

		gameState->camPos = player->entity->pos;

#if DEBUG_BUILD
		if (g_debugContext->drawGJKPolytope)
		{
			DebugVertex *gjkVertices;
			u32 gjkFaceCount;
			GetGJKStepGeometry(g_debugContext->gjkDrawStep, &gjkVertices, &gjkFaceCount);
			DrawDebugTriangles(gjkVertices, gjkFaceCount);

			DrawDebugCubeAA(g_debugContext->GJKNewPoint[g_debugContext->gjkDrawStep], 0.03f);

			DrawDebugCubeAA({}, 0.04f, {1,0,1});
		}

		if (g_debugContext->drawEPAPolytope)
		{
			DebugVertex *epaVertices;
			u32 epaFaceCount;
			GetEPAStepGeometry(g_debugContext->polytopeDrawStep, &epaVertices, &epaFaceCount);
			DrawDebugTriangles(epaVertices, epaFaceCount * 3);

			DrawDebugCubeAA(g_debugContext->epaNewPoint[g_debugContext->polytopeDrawStep], 0.03f);

			DrawDebugCubeAA({}, 0.04f, {1,0,1});
		}
#endif
	}

	// Draw
	{
		// Projection matrix
		mat4 proj;
		{
			const f32 fov = HALFPI;
			const f32 near = 0.01f;
			const f32 far = 2000.0f;
			const f32 aspectRatio = (16.0f / 9.0f);

			const f32 top = Tan(HALFPI - fov / 2.0f);
			const f32 right = top / aspectRatio;

			proj =
			{
				right, 0.0f, 0.0f, 0.0f,
				0.0f, top, 0.0f, 0.0f,
				0.0f, 0.0f, -(far + near) / (far - near), -1.0f,
				0.0f, 0.0f, -(2.0f * far * near) / (far - near), 0.0f
			};
		}

		// View matrix
		mat4 view;
		{
			view = Mat4Translation({ 0.0f, 0.0f, -4.5f });
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

		UseProgram(gameState->program);
		DeviceUniform viewUniform = GetUniform(gameState->program, "view");
		UniformMat4(viewUniform, 1, view.m);
		DeviceUniform projUniform = GetUniform(gameState->program, "projection");
		UniformMat4(projUniform, 1, proj.m);

		DeviceUniform albedoUniform = GetUniform(gameState->program, "texAlbedo");
		UniformInt(albedoUniform, 0);
		DeviceUniform normalUniform = GetUniform(gameState->program, "texNormal");
		UniformInt(normalUniform, 1);

		DeviceUniform modelUniform = GetUniform(gameState->program, "model");

		const v4 clearColor = { 0.95f, 0.88f, 0.05f, 1.0f };
		ClearBuffers(clearColor);

		const Resource *texAlb = GetResource("data/grid.b");
		const Resource *texNor = GetResource("data/normal_plain.b");
		BindTexture(texAlb->texture.deviceTexture, 0);
		BindTexture(texNor->texture.deviceTexture, 1);

		// Entity
		for (u32 entityIdx = 0; entityIdx < gameState->entityCount; ++entityIdx)
		{
			Entity *entity = &gameState->entities[entityIdx];
			if (!entity->mesh)
				continue;

			const mat4 model = Mat4ChangeOfBases(entity->fw, {0,0,1}, entity->pos);
			UniformMat4(modelUniform, 1, model.m);

			RenderIndexedMesh(entity->mesh->mesh.deviceMesh);
		}

		// Level
		{
			LevelGeometry *level = &gameState->levelGeometry;

			UniformMat4(modelUniform, 1, MAT4_IDENTITY.m);
			RenderIndexedMesh(level->renderMesh->mesh.deviceMesh);
		}

		// Skinned meshes
		UseProgram(gameState->skinnedMeshProgram);
		viewUniform = GetUniform(gameState->skinnedMeshProgram, "view");
		modelUniform = GetUniform(gameState->skinnedMeshProgram, "model");
		projUniform = GetUniform(gameState->skinnedMeshProgram, "projection");
		UniformMat4(projUniform, 1, proj.m);

		const Resource *sparkusAlb = GetResource("data/sparkus_albedo.b");
		const Resource *sparkusNor = GetResource("data/sparkus_normal.b");
		BindTexture(sparkusAlb->texture.deviceTexture, 0);
		BindTexture(sparkusNor->texture.deviceTexture, 1);

		albedoUniform = GetUniform(gameState->skinnedMeshProgram, "texAlbedo");
		UniformInt(albedoUniform, 0);
		normalUniform = GetUniform(gameState->skinnedMeshProgram, "texNormal");
		UniformInt(normalUniform, 1);

		DeviceUniform jointsUniform = GetUniform(gameState->skinnedMeshProgram, "joints");
		UniformMat4(viewUniform, 1, view.m);

		for (u32 meshInstanceIdx = 0; meshInstanceIdx < gameState->skinnedMeshCount;
				++meshInstanceIdx)
		{
			SkinnedMeshInstance *skinnedMeshInstance =
				&gameState->skinnedMeshInstances[meshInstanceIdx];

			mat4 joints[128];
			for (int i = 0; i < 128; ++i)
			{
				joints[i] = MAT4_IDENTITY;
			}

			const Resource *skinnedMeshRes = skinnedMeshInstance->meshRes;
			const ResourceSkinnedMesh *skinnedMesh = &skinnedMeshRes->skinnedMesh;

			Animation *animation = &skinnedMesh->animations[skinnedMeshInstance->animationIdx];

			f32 lastTimestamp = animation->timestamps[animation->frameCount - 1];

			f32 currentTime = skinnedMeshInstance->animationTime;

			int currentFrame = -1;
			for (u32 i = 0; i < animation->frameCount; ++i)
			{
				if (currentFrame == -1 && currentTime <= animation->timestamps[i])
					currentFrame = i - 1;
			}
			f32 prevStamp = animation->timestamps[currentFrame];
			f32 nextStamp = animation->timestamps[currentFrame + 1];
			f32 lerpWindow = nextStamp - prevStamp;
			f32 lerpTime = currentTime - prevStamp;
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
			currentTime += deltaTime;
			if (currentTime >= lastTimestamp)
			{
				if (animation->loop)
					currentTime = 0;
				else
					currentTime = lastTimestamp;
			}
			skinnedMeshInstance->animationTime = currentTime;

			// @Todo: actual, safe entity handle.
			Entity *player = &gameState->entities[skinnedMeshInstance->entityHandle];
			const mat4 model = Mat4ChangeOfBases(player->fw, {0,0,1}, player->pos);
			UniformMat4(modelUniform, 1, model.m);

			UniformMat4(jointsUniform, 128, joints[0].m);

			RenderIndexedMesh(skinnedMesh->deviceMesh);
		}

#if DEBUG_BUILD
		// Debug meshes
		{
			DebugGeometryBuffer *dgb = &g_debugContext->debugGeometryBuffer;
			if (g_debugContext->wireframeDebugDraws)
				SetFillMode(RENDER_LINE);

			DisableDepthTest();

			UseProgram(g_debugContext->debugDrawProgram);
			viewUniform = GetUniform(g_debugContext->debugDrawProgram, "view");
			projUniform = GetUniform(g_debugContext->debugDrawProgram, "projection");
			UniformMat4(projUniform, 1, proj.m);
			UniformMat4(viewUniform, 1, view.m);

			SendMesh(&dgb->deviceMesh,
					dgb->triangleData,
					dgb->triangleVertexCount, sizeof(DebugVertex), true);

			RenderMesh(dgb->deviceMesh);

			SendMesh(&dgb->deviceMesh,
					dgb->lineData,
					dgb->lineVertexCount, sizeof(DebugVertex), true);
			RenderLines(dgb->deviceMesh);

			if (dgb->debugCubeCount)
			{
				UseProgram(g_debugContext->debugCubesProgram);
				viewUniform = GetUniform(g_debugContext->debugDrawProgram, "view");
				projUniform = GetUniform(g_debugContext->debugDrawProgram, "projection");
				UniformMat4(projUniform, 1, proj.m);
				UniformMat4(viewUniform, 1, view.m);

				SendMesh(&dgb->cubePositionsBuffer,
						dgb->debugCubes,
						dgb->debugCubeCount, sizeof(DebugCube), true);

				u32 meshAttribs = RENDERATTRIB_POSITION;
				u32 instAttribs = RENDERATTRIB_POSITION | RENDERATTRIB_COLOR |
					RENDERATTRIB_1CUSTOMV3 | RENDERATTRIB_2CUSTOMV3 | RENDERATTRIB_1CUSTOMF32;
				RenderIndexedMeshInstanced(dgb->cubeMesh, dgb->cubePositionsBuffer,
						meshAttribs, instAttribs);
			}

			dgb->debugCubeCount = 0;
			dgb->triangleVertexCount = 0;
			dgb->lineVertexCount = 0;

			EnableDepthTest();
			SetFillMode(RENDER_FILL);
		}
#endif
	}
}

void CleanupGame(GameState *gameState)
{
	(void) gameState;
	return;
}
