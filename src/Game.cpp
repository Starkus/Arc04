#include <stddef.h>
#include <memory.h>

#define IMGUI_SHOW_DEMO

#if USING_IMGUI
#include <imgui/imgui.h>
#ifdef IMGUI_SHOW_DEMO
#include <imgui/imgui_demo.cpp> // @Todo: remove
#endif
#endif

#if defined(USING_IMGUI) && DEBUG_BUILD
#define EDITOR_PRESENT 1
#else
#define EDITOR_PRESENT 0
#endif

#include "General.h"
#include "RandomTable.h"
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

#if TARGET_WINDOWS
#define GAMEDLL NOMANGLE __declspec(dllexport)
#else
#define GAMEDLL NOMANGLE __attribute__((visibility("default")))
#endif

Entity *GetEntity(GameState *gameState, EntityHandle handle)
{
	// Check handle is valid
	if (handle.generation != gameState->entityGenerations[handle.id])
		return nullptr;

	return gameState->entityPointers[handle.id];
}

EntityHandle FindEntityHandle(GameState *gameState, Entity *entityPtr)
{
	for (u32 entityId = 0; entityId < ArrayCount(gameState->entities); ++entityId)
	{
		Entity *currentPtr = gameState->entityPointers[entityId];
		if (currentPtr == entityPtr)
		{
			return { entityId, gameState->entityGenerations[entityId] };
		}
	}
	return ENTITY_HANDLE_INVALID;
}

EntityHandle AddEntity(GameState *gameState, Entity **outEntity)
{
	Entity *newEntity = &gameState->entities[gameState->entityCount++];
	*newEntity = {};
	newEntity->rot = QUATERNION_IDENTITY;

	EntityHandle newHandle = ENTITY_HANDLE_INVALID;

	for (int entityId = 0; entityId < 256; ++entityId)
	{
		Entity *ptrInIdx = gameState->entityPointers[entityId];
		if (!ptrInIdx)
		{
			// Assing vacant ID to new entity
			gameState->entityPointers[entityId] = newEntity;

			// Fill out entity handle
			newHandle.id = entityId;
			// Advance generation by one
			newHandle.generation = ++gameState->entityGenerations[entityId];

			break;
		}
	}

	Log("Added new entity at 0x%p (id %d gen %d)\n", newEntity, newHandle.id, newHandle.generation);

	*outEntity = newEntity;
	return newHandle;
}

// @Improve: delay actual deletion of entities til end of frame?
void RemoveEntity(GameState *gameState, EntityHandle handle)
{
	// Check handle is valid
	if (handle.generation != gameState->entityGenerations[handle.id])
		return;

	Entity *entityPtr = gameState->entityPointers[handle.id];
	// If entity is already deleted there's nothing to do
	if (!entityPtr)
		return;

	// Remove 'components'
	SkinnedMeshInstance *skinnedMeshInstance = entityPtr->skinnedMeshInstance;
	if (skinnedMeshInstance)
	{
		SkinnedMeshInstance *last = &gameState->skinnedMeshInstances[--gameState->skinnedMeshCount];
		Entity *lastsEntity = GetEntity(gameState, last->entityHandle);
		if (lastsEntity)
		{
			// Fix entity that was pointing to skinnedMeshInstance that we moved.
			lastsEntity->skinnedMeshInstance = skinnedMeshInstance;
		}
		*skinnedMeshInstance = *last;
	}

	// Erase from entity array by swapping with last
	Entity *ptrOfLast = &gameState->entities[--gameState->entityCount];
	Log("Moving entity at 0x%p to 0x%p\n", ptrOfLast, entityPtr);
	*entityPtr = *ptrOfLast;

	// Find and fix handle to entity we moved
	for (int entityId = 0; entityId < 256; ++entityId)
	{
		Entity *currentPtr = gameState->entityPointers[entityId];
		if (currentPtr == ptrOfLast)
		{
			gameState->entityPointers[entityId] = entityPtr;
			break;
		}
	}

	gameState->entityPointers[handle.id] = nullptr;
	//gameState->entityGenerations[handle.id] = 0;
}

#ifdef USING_IMGUI
#include "Imgui.cpp"
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

	memset(gameState, 0, sizeof(GameState));
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

		const Resource *shaderParticleRes = LoadResource(RESOURCETYPE_SHADER, "data/shaders/shader_particles.b");
		gameState->particleSystemProgram = shaderParticleRes->shader.programHandle;

#if DEBUG_BUILD
		const Resource *shaderDebugRes = LoadResource(RESOURCETYPE_SHADER, "data/shaders/shader_debug.b");
		g_debugContext->debugDrawProgram = shaderDebugRes->shader.programHandle;

		const Resource *shaderDebugCubesRes = LoadResource(RESOURCETYPE_SHADER, "data/shaders/shader_debug_cubes.b");
		g_debugContext->debugCubesProgram = shaderDebugCubesRes->shader.programHandle;

		const Resource *shaderEditorSelectedRes = LoadResource(RESOURCETYPE_SHADER, "data/shaders/shader_editor_selected.b");
		g_debugContext->editorSelectedProgram = shaderEditorSelectedRes->shader.programHandle;
#endif
	}

	// Particle mesh
	{
		// @Improve: render attribs don't matter when the mesh will be used for instanced rendering.
		// Make more consistent!!
		DeviceMesh particleMesh = CreateDeviceMesh(0);
		// @Improve: triangle strip?
		u8 data[] = { 0, 1, 2, 2, 1, 3 };
		SendMesh(&particleMesh, data, ArrayCount(data), sizeof(data[0]), false);
		gameState->particleMesh = particleMesh;
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
		Entity *playerEnt;
		EntityHandle playerEntityHandle = AddEntity(gameState, &playerEnt);
		gameState->player.entityHandle = playerEntityHandle;
		playerEnt->rot = QUATERNION_IDENTITY;
		playerEnt->mesh = 0;

		playerEnt->collider.type = COLLIDER_CAPSULE;
		playerEnt->collider.capsule.radius = 0.5f;
		playerEnt->collider.capsule.height = 1.0f;
		playerEnt->collider.capsule.offset = { 0, 0, 1 };

		gameState->player.state = PLAYERSTATE_IDLE;

		SkinnedMeshInstance *skinnedMeshInstance =
			&gameState->skinnedMeshInstances[gameState->skinnedMeshCount++];
		skinnedMeshInstance->entityHandle = playerEntityHandle;
		skinnedMeshInstance->meshRes = GetResource("data/Sparkus.b");
		skinnedMeshInstance->animationIdx = PLAYERANIM_IDLE;
		skinnedMeshInstance->animationTime = 0;
		playerEnt->skinnedMeshInstance = skinnedMeshInstance;

		ParticleSystem *particleSystem =
			&gameState->particleSystems[gameState->particleSystemCount++];
		particleSystem->entityHandle = playerEntityHandle;
		particleSystem->deviceBuffer = CreateDeviceMesh(0);
		playerEnt->particleSystem = particleSystem;
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

		Entity *testEntity;
		AddEntity(gameState, &testEntity);
		testEntity->pos = { -6.0f, 3.0f, 1.0f };
		testEntity->rot = QUATERNION_IDENTITY;
		testEntity->mesh = anvilRes;
		testEntity->collider = collider;

		AddEntity(gameState, &testEntity);
		testEntity->pos = { 5.0f, 4.0f, 1.0f };
		testEntity->rot = QUATERNION_IDENTITY;
		testEntity->mesh = anvilRes;
		testEntity->collider = collider;

		AddEntity(gameState, &testEntity);
		testEntity->pos = { 3.0f, -4.0f, 1.0f };
		testEntity->rot = QuaternionFromEuler(v3{ 0, 0, HALFPI * -0.5f });
		testEntity->mesh = anvilRes;
		testEntity->collider = collider;

		AddEntity(gameState, &testEntity);
		testEntity->pos = { -8.0f, -4.0f, 1.0f };
		testEntity->rot = QuaternionFromEuler(v3{ HALFPI * 0.5f, 0, 0 });
		testEntity->mesh = anvilRes;
		testEntity->collider = collider;

		const Resource *sphereRes = GetResource("data/sphere.b");
		AddEntity(gameState, &testEntity);
		testEntity->pos = { -6.0f, 7.0f, 1.0f };
		testEntity->rot = QUATERNION_IDENTITY;
		testEntity->mesh = sphereRes;
		testEntity->collider.type = COLLIDER_SPHERE;
		testEntity->collider.sphere.radius = 1;
		testEntity->collider.sphere.offset = {};

		const Resource *cylinderRes = GetResource("data/cylinder.b");
		AddEntity(gameState, &testEntity);
		testEntity->pos = { -3.0f, 7.0f, 1.0f };
		testEntity->rot = QUATERNION_IDENTITY;
		testEntity->mesh = cylinderRes;
		testEntity->collider.type = COLLIDER_CYLINDER;
		testEntity->collider.cylinder.radius = 1;
		testEntity->collider.cylinder.height = 2;
		testEntity->collider.cylinder.offset = {};

		const Resource *capsuleRes = GetResource("data/capsule.b");
		AddEntity(gameState, &testEntity);
		testEntity->pos = { 0.0f, 7.0f, 2.0f };
		testEntity->rot = QUATERNION_IDENTITY;
		testEntity->mesh = capsuleRes;
		testEntity->collider.type = COLLIDER_CAPSULE;
		testEntity->collider.capsule.radius = 1;
		testEntity->collider.capsule.height = 2;
		testEntity->collider.capsule.offset = {};

		EntityHandle testEntityHandle = AddEntity(gameState, &testEntity);
		testEntity->pos = { 3.0f, 4.0f, 0.0f };
		testEntity->rot = QUATERNION_IDENTITY;
		testEntity->mesh = nullptr;
		testEntity->collider.type = COLLIDER_SPHERE;
		testEntity->collider.sphere.radius = 0.3f;
		testEntity->collider.sphere.offset = { 0, 0, 1 };
		SkinnedMeshInstance *skinnedMeshInstance =
			&gameState->skinnedMeshInstances[gameState->skinnedMeshCount++];
		skinnedMeshInstance->entityHandle = testEntityHandle;
		skinnedMeshInstance->meshRes = GetResource("data/Jumper.b");
		skinnedMeshInstance->animationIdx = 0;
		skinnedMeshInstance->animationTime = 0;
		testEntity->skinnedMeshInstance = skinnedMeshInstance;
	}
}

void ChangeState(GameState *gameState, PlayerState newState, PlayerAnim newAnim)
{
	//Log("Changed state: 0x%X -> 0x%X\n", gameState->player.state, newState);
	Entity *playerEntity = GetEntity(gameState, gameState->player.entityHandle);
	playerEntity->skinnedMeshInstance->animationIdx = newAnim;
	playerEntity->skinnedMeshInstance->animationTime = 0;
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
	ImguiShowEditWindow(gameState);
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
		Entity *playerEntity = GetEntity(gameState, player->entityHandle);
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

			if (V3SqrLen(worldInputDir))
			{
				f32 targetYaw = Atan2(-worldInputDir.x, worldInputDir.y);
				playerEntity->rot = QuaternionFromEuler(v3{ 0, 0, targetYaw });
			}

			const f32 playerSpeed = 5.0f;
			playerEntity->pos += worldInputDir * playerSpeed * deltaTime;

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

			playerEntity->pos += player->vel * deltaTime;
		}

		const f32 groundRayDist = (player->state & PLAYERSTATEFLAG_AIRBORNE) ? 1.0f : 1.5f;

		// Collision
		bool touchedGround = false;

		for (u32 entityIndex = 0; entityIndex < gameState->entityCount; ++entityIndex)
		{
			Entity *entity = &gameState->entities[entityIndex];
			if (entity != playerEntity)
			{
				v3 origin = playerEntity->pos + v3{ 0, 0, 1 };
				v3 dir = { 0, 0, -groundRayDist };
				v3 hit;
				v3 hitNor;
				if (RayColliderIntersection(origin, dir, entity, &hit, &hitNor))
				{
					if (hitNor.z > 0.7f)
					{
						playerEntity->pos.z = hit.z;
						touchedGround = true;
						break;
					}
				}

				GJKResult gjkResult = GJKTest(playerEntity, entity);
				if (gjkResult.hit)
				{
					v3 depenetration = ComputeDepenetration(gjkResult, playerEntity, entity);
					// @Hack: ignoring depenetration when it's too big
					if (V3Length(depenetration) < 2.0f)
					{
						playerEntity->pos += depenetration;
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
			v3 origin = playerEntity->pos + v3{ 0, 0, 1 };
			v3 dir = { 0, 0, -groundRayDist };
			v3 hit;
			Triangle triangle;
			if (HitTest(gameState, origin, dir, &hit, &triangle))
			{
				playerEntity->pos.z = hit.z;
				touchedGround = true;
			}

			origin = playerEntity->pos + v3{ 0, 0, 1 };
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
					// Ignore slopes
					if (triangle.normal.z > 0.05f)
						continue;

					f32 dot = V3Dot(triangle.normal, dirs[i] / rayLen);
					if (dot > -0.707f && dot < 0.707f)
						continue;

					hit.z = playerEntity->pos.z;

					f32 aDistAlongNormal = V3Dot(triangle.a - playerEntity->pos, triangle.normal);
					if (aDistAlongNormal < 0) aDistAlongNormal = -aDistAlongNormal;
					if (aDistAlongNormal < playerRadius)
					{
						f32 factor = playerRadius - aDistAlongNormal;
						playerEntity->pos += triangle.normal * factor;
					}
				}
			}
		}

		bool wasAirborne = player->state & PLAYERSTATEFLAG_AIRBORNE;
		if (wasAirborne && touchedGround && player->vel.z < 0)
		{
			ChangeState(gameState, PLAYERSTATE_LAND, PLAYERANIM_LAND);
		}
		else if (!wasAirborne && !touchedGround)
		{
			ChangeState(gameState, PLAYERSTATE_FALL, PLAYERANIM_FALL);
		}

		if (playerEntity->pos.z < -1)
		{
			playerEntity->pos.z = 1;
			player->vel = {};
		}

		gameState->camPos = playerEntity->pos;

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
		UniformMat4Array(viewUniform, 1, view.m);
		DeviceUniform projUniform = GetUniform(gameState->program, "projection");
		UniformMat4Array(projUniform, 1, proj.m);

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

			const mat4 model = Mat4Compose(entity->pos, entity->rot);
			UniformMat4Array(modelUniform, 1, model.m);

			RenderIndexedMesh(entity->mesh->mesh.deviceMesh);
		}

		// Level
		{
			LevelGeometry *level = &gameState->levelGeometry;

			UniformMat4Array(modelUniform, 1, MAT4_IDENTITY.m);
			RenderIndexedMesh(level->renderMesh->mesh.deviceMesh);
		}

		// Skinned meshes
		UseProgram(gameState->skinnedMeshProgram);
		viewUniform = GetUniform(gameState->skinnedMeshProgram, "view");
		modelUniform = GetUniform(gameState->skinnedMeshProgram, "model");
		projUniform = GetUniform(gameState->skinnedMeshProgram, "projection");
		UniformMat4Array(projUniform, 1, proj.m);

		const Resource *sparkusAlb = GetResource("data/sparkus_albedo.b");
		const Resource *sparkusNor = GetResource("data/sparkus_normal.b");
		BindTexture(sparkusAlb->texture.deviceTexture, 0);
		BindTexture(sparkusNor->texture.deviceTexture, 1);

		albedoUniform = GetUniform(gameState->skinnedMeshProgram, "texAlbedo");
		UniformInt(albedoUniform, 0);
		normalUniform = GetUniform(gameState->skinnedMeshProgram, "texNormal");
		UniformInt(normalUniform, 1);

		DeviceUniform jointTranslationsUniform = GetUniform(gameState->skinnedMeshProgram, "jointTranslations");
		DeviceUniform jointRotationsUniform = GetUniform(gameState->skinnedMeshProgram, "jointRotations");
		DeviceUniform jointScalesUniform = GetUniform(gameState->skinnedMeshProgram, "jointScales");
		UniformMat4Array(viewUniform, 1, view.m);

		for (u32 meshInstanceIdx = 0; meshInstanceIdx < gameState->skinnedMeshCount;
				++meshInstanceIdx)
		{
			SkinnedMeshInstance *skinnedMeshInstance =
				&gameState->skinnedMeshInstances[meshInstanceIdx];

#if DEBUG_BUILD
			// Avoid crash when adding these in the editor
			if (!skinnedMeshInstance->meshRes)
				continue;
#endif

			Transform joints[128];
			for (int i = 0; i < 128; ++i)
			{
				joints[i] = TRANSFORM_IDENTITY;
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

				Transform transform = TRANSFORM_IDENTITY;

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
						transform = TransformChain(transform, skinnedMesh->restPoses[currentJoint]);
						first = false;
						continue;
					}

					Transform a = currentChannel->transforms[currentFrame];
					Transform b = currentChannel->transforms[currentFrame + 1];

					f32 aWeight = (1 - lerpT);
					f32 bWeight = lerpT;
					bool invertRot = V4Dot(a.rotation, b.rotation) < 0;

					Transform r =
					{
						a.translation * aWeight + b.translation * bWeight,
						V4Normalize(a.rotation * aWeight + b.rotation * (invertRot ? -bWeight : bWeight)),
						a.scale * aWeight + b.scale * bWeight
					};

					mat4 T = Mat4Compose(r);

					if (first)
					{
						transform = r;
						first = false;
					}
					else
					{
						transform = TransformChain(transform, r);
					}
				}
				transform = TransformChain(transform, skinnedMesh->bindPoses[jointIdx]);

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

			Entity *entity = GetEntity(gameState, skinnedMeshInstance->entityHandle);
			const mat4 model = Mat4Compose(entity->pos, entity->rot);
			UniformMat4Array(modelUniform, 1, model.m);

			v3 ts[128];
			v4 rs[128];
			v3 ss[128];
			for (int i = 0; i < 128; ++i)
			{
				ts[i] = joints[i].translation;
				rs[i] = joints[i].rotation;
				ss[i] = joints[i].scale;
			}
			UniformV3Array(jointTranslationsUniform, 128, ts[0].v);
			UniformV4Array(jointRotationsUniform, 128, rs[0].v);
			UniformV3Array(jointScalesUniform, 128, ss[0].v);

			RenderIndexedMesh(skinnedMesh->deviceMesh);
		}

		// Particles
		UseProgram(gameState->particleSystemProgram);
		viewUniform = GetUniform(gameState->particleSystemProgram, "view");
		projUniform = GetUniform(gameState->particleSystemProgram, "projection");
		UniformMat4Array(projUniform, 1, proj.m);
		UniformMat4Array(viewUniform, 1, view.m);

		for (u32 partSysIdx = 0; partSysIdx < gameState->particleSystemCount;
				++partSysIdx)
		{
			ParticleSystem *particleSystem = &gameState->particleSystems[partSysIdx];
			Entity *entity = GetEntity(gameState, particleSystem->entityHandle);

			const int maxCount = ArrayCount(particleSystem->particles);
			const f32 spawnRate = 0.013f;
			const f32 maxLife = 2.0f;
			particleSystem->timer += deltaTime;
			while (particleSystem->timer > spawnRate)
			{
				particleSystem->timer -= spawnRate;

				// Spawn particle
				for (int i = 0; i < maxCount; ++i)
				{
					if (!particleSystem->alive[i])
					{
						particleSystem->alive[i] = true;

						particleSystem->bookkeeps[i].lifeTime = 0;
						particleSystem->bookkeeps[i].duration = GetRandom() / (f32)U32_MAX * maxLife;
						particleSystem->bookkeeps[i].velocity =
						{
							GetRandom() / (f32)U32_MAX - 0.5f,
							GetRandom() / (f32)U32_MAX - 0.5f,
							1
						};

						particleSystem->particles[i].pos = entity->pos;
						particleSystem->particles[i].size = 0.1f;
						particleSystem->particles[i].color =
						{
							GetRandom() / (f32)U32_MAX,
							GetRandom() / (f32)U32_MAX,
							GetRandom() / (f32)U32_MAX,
						};
						break;
					}
				}
			}
			for (int i = 0; i < maxCount; ++i)
			{
				if (!particleSystem->alive[i])
				{
					particleSystem->particles[i].size = 0;
					continue;
				}

				ParticleBookkeep *bookkeep = &particleSystem->bookkeeps[i];
				bookkeep->lifeTime += deltaTime;
				if (bookkeep->lifeTime > bookkeep->duration)
				{
					// Despawn
					particleSystem->alive[i] = false;
				}

				particleSystem->particles[i].pos += bookkeep->velocity * deltaTime;
				particleSystem->particles[i].size += 0.1f * deltaTime;
			}

			SendMesh(&particleSystem->deviceBuffer,
					particleSystem->particles,
					ArrayCount(particleSystem->particles),
					sizeof(particleSystem->particles[0]), true);

			u32 meshAttribs = RENDERATTRIB_VERTEXNUM;
			u32 instAttribs = RENDERATTRIB_POSITION | RENDERATTRIB_COLOR | RENDERATTRIB_1CUSTOMF32;
			RenderMeshInstanced(gameState->particleMesh, particleSystem->deviceBuffer, meshAttribs,
					instAttribs);
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
			UniformMat4Array(projUniform, 1, proj.m);
			UniformMat4Array(viewUniform, 1, view.m);

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
				UniformMat4Array(projUniform, 1, proj.m);
				UniformMat4Array(viewUniform, 1, view.m);

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

		// Selected entity
		{
			SetFillMode(RENDER_LINE);

			UseProgram(g_debugContext->editorSelectedProgram);
			viewUniform = GetUniform(g_debugContext->editorSelectedProgram, "view");
			UniformMat4Array(viewUniform, 1, view.m);
			projUniform = GetUniform(g_debugContext->editorSelectedProgram, "projection");
			UniformMat4Array(projUniform, 1, proj.m);
			modelUniform = GetUniform(g_debugContext->editorSelectedProgram, "model");

			static f32 t = 0;
			t += deltaTime;
			DeviceUniform timeUniform = GetUniform(g_debugContext->editorSelectedProgram, "time");
			UniformFloat(timeUniform, t);

			Entity *entity = &gameState->entities[g_debugContext->selectedEntityIdx];
			if (entity->mesh)
			{
				const mat4 model = Mat4Compose(entity->pos, entity->rot);
				UniformMat4Array(modelUniform, 1, model.m);

				RenderIndexedMesh(entity->mesh->mesh.deviceMesh);
			}

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
