@Ignore #include <stdlib.h>
@Ignore #include <stddef.h>
@Ignore #include <memory.h>
@Ignore #include <string.h>
@Ignore #include <stdarg.h>

#include "General.h"

#if TARGET_WINDOWS && DEBUG_BUILD
#define USING_IMGUI 1
#endif

#if USING_IMGUI && DEBUG_BUILD
#define EDITOR_PRESENT 1
#else
#define EDITOR_PRESENT 0
#endif

#define IMGUI_SHOW_DEMO

#if USING_IMGUI
@Ignore #include <imgui/imgui.h>
#ifdef IMGUI_SHOW_DEMO
@Ignore #include <imgui/imgui_demo.cpp> // @Todo: remove
#endif
#endif

@Ignore #include "TypeInfo.h"
#include "RandomTable.h"
#include "Maths.h"
#include "MemoryAlloc.h"
#include "Render.h"
#include "Geometry.h"
#include "Platform.h"
#include "Strings.h"
#include "StringStream.h"
#include "Resource.h"
@Ignore #include "PlatformCode.h"
#include "Containers.h"
#include "Entity.h"
#include "GameInterface.h"
#include "Game.h"

Memory *g_memory;
#if DEBUG_BUILD
DebugContext *g_debugContext;
#endif

DECLARE_ARRAY(u32);

@Ignore #include "PlatformCodeLoad.cpp"
#include "DebugDraw.cpp"
#include "MemoryAlloc.cpp"
#include "Collision.cpp"
#include "BakeryInterop.cpp"
#include "Resource.cpp"
#include "Entity.cpp"
#include "Parsing.cpp"
#include "Serialize.cpp"

#if TARGET_WINDOWS
#define GAMEDLL NOMANGLE __declspec(dllexport)
#else
#define GAMEDLL NOMANGLE __attribute__((visibility("default")))
#endif

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
	case RESOURCETYPE_MATERIAL:
	{
		ResourceLoadMaterial(resource, fileBuffer, initialize);
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
	*g_debugContext = {};
#endif

	// Init game state
	memset(gameState, 0, sizeof(GameState));
	gameState->timeMultiplier = 1.0f;
	ArrayInit_Transform(&gameState->transforms, 4096, TransientAlloc);
	ArrayInit_MeshInstance(&gameState->meshInstances, 4096, TransientAlloc);
	ArrayInit_SkinnedMeshInstance(&gameState->skinnedMeshInstances, 2048, TransientAlloc);
	ArrayInit_ParticleSystem(&gameState->particleSystems, 1024, TransientAlloc);
	ArrayInit_Collider(&gameState->colliders, 4096, TransientAlloc);

	// @Hack: Hmmm
	memset(gameState->entityTransforms, 0xFF, sizeof(gameState->entityTransforms));
	memset(gameState->entityMeshes, 0xFF, sizeof(gameState->entityMeshes));
	memset(gameState->entitySkinnedMeshes, 0xFF, sizeof(gameState->entitySkinnedMeshes));
	memset(gameState->entityParticleSystems, 0xFF, sizeof(gameState->entityParticleSystems));
	memset(gameState->entityColliders, 0xFF, sizeof(gameState->entityColliders));

	// Initialize
	{
		SetUpDevice();

		// Render buffer
		{
			u32 w, h;
			GetWindowSize(&w, &h);

			gameState->frameBufferColorTex = CreateDeviceTexture();
			SendTexture(gameState->frameBufferColorTex, nullptr, w, h, RENDERIMAGECOMPONENTS_3);

			gameState->frameBufferDepthTex = CreateDeviceTexture();
			SendTexture(gameState->frameBufferDepthTex, nullptr, w, h, RENDERIMAGECOMPONENTS_DEPTH24);

			gameState->frameBuffer = CreateDeviceFrameBuffer(gameState->frameBufferColorTex,
					gameState->frameBufferDepthTex);
		}

		LoadResource(RESOURCETYPE_MESH, "anvil.b");
		LoadResource(RESOURCETYPE_MESH, "cube.b");
		LoadResource(RESOURCETYPE_MESH, "sphere.b");
		LoadResource(RESOURCETYPE_MESH, "cylinder.b");
		LoadResource(RESOURCETYPE_MESH, "capsule.b");
		LoadResource(RESOURCETYPE_MESH, "level_graphics.b");
		LoadResource(RESOURCETYPE_SKINNEDMESH, "Sparkus.b");
		LoadResource(RESOURCETYPE_SKINNEDMESH, "Jumper.b");
		LoadResource(RESOURCETYPE_LEVELGEOMETRYGRID, "level.b");
		LoadResource(RESOURCETYPE_COLLISIONMESH, "anvil_collision.b");
		LoadResource(RESOURCETYPE_TEXTURE, "particle_atlas.b");

		LoadResource(RESOURCETYPE_MATERIAL, "material_default.b");
		LoadResource(RESOURCETYPE_MATERIAL, "material_default_skinned.b");

#if EDITOR_PRESENT
		LoadResource(RESOURCETYPE_MESH, "editor_arrow.b");
		LoadResource(RESOURCETYPE_COLLISIONMESH, "editor_arrow_collision.b");
		LoadResource(RESOURCETYPE_MESH, "editor_circle.b");
		LoadResource(RESOURCETYPE_COLLISIONMESH, "editor_circle_collision.b");
#endif

#if DEBUG_BUILD
		// Debug geometry buffer
		{
			int attribs = RENDERATTRIB_POSITION | RENDERATTRIB_COLOR3;
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
			const char *materialName;
			ReadMesh(fileBuffer, &vertexData, &indexData, &vertexCount, &indexCount, &materialName);

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
		const Resource *shaderRes = LoadResource(RESOURCETYPE_SHADER, "shaders/shader_general.b");
		gameState->program = shaderRes->shader.programHandle;

		const Resource *shaderSkinnedRes = LoadResource(RESOURCETYPE_SHADER, "shaders/shader_skinned.b");
		gameState->skinnedMeshProgram = shaderSkinnedRes->shader.programHandle;

		const Resource *shaderParticleRes = LoadResource(RESOURCETYPE_SHADER, "shaders/shader_particles.b");
		gameState->particleSystemProgram = shaderParticleRes->shader.programHandle;

		const Resource *shaderFrameBufferRes = LoadResource(RESOURCETYPE_SHADER, "shaders/shader_framebuffer.b");
		gameState->frameBufferProgram = shaderFrameBufferRes->shader.programHandle;

#if DEBUG_BUILD
		const Resource *shaderDebugRes = LoadResource(RESOURCETYPE_SHADER, "shaders/shader_debug.b");
		g_debugContext->debugDrawProgram = shaderDebugRes->shader.programHandle;

		const Resource *shaderDebugCubesRes = LoadResource(RESOURCETYPE_SHADER, "shaders/shader_debug_cubes.b");
		g_debugContext->debugCubesProgram = shaderDebugCubesRes->shader.programHandle;
#endif

#if EDITOR_PRESENT
		const Resource *shaderEditorSelectedRes = LoadResource(RESOURCETYPE_SHADER, "shaders/shader_editor_selected.b");
		g_debugContext->editorSelectedProgram = shaderEditorSelectedRes->shader.programHandle;

		const Resource *shaderEditorGizmoRes = LoadResource(RESOURCETYPE_SHADER, "shaders/shader_editor_gizmo.b");
		g_debugContext->editorGizmoProgram = shaderEditorGizmoRes->shader.programHandle;
#endif
	}

	// Particle mesh
	{
		// @Improve: render attribs don't matter when the mesh will be used for instanced rendering.
		// Make more consistent!!
		DeviceMesh particleMesh = CreateDeviceMesh(RENDERATTRIB_VERTEXNUM);
		// @Improve: triangle strip?
		u8 data[] = { 0, 1, 2, 2, 1, 3 };
		SendMesh(&particleMesh, data, ArrayCount(data), sizeof(data[0]), false);
		gameState->particleMesh = particleMesh;
	}

	// Init level
	{
		const Resource *levelGraphicsRes = GetResource("level_graphics.b");
		gameState->levelGeometry.renderMesh = levelGraphicsRes;

		const Resource *levelCollisionRes = GetResource("level.b");
		gameState->levelGeometry.geometryGrid = levelCollisionRes;
	}

	// Init player
	{
		Transform *playerT;
		EntityHandle playerEntityHandle = AddEntity(gameState, &playerT);
		gameState->player.entityHandle = playerEntityHandle;
		playerT->rotation = QUATERNION_IDENTITY;

		Collider *collider = ArrayAdd_Collider(&gameState->colliders);
		collider->type = COLLIDER_CAPSULE;
		collider->capsule.radius = 0.5f;
		collider->capsule.height = 1.0f;
		collider->capsule.offset = { 0, 0, 1 };
		EntityAssignCollider(gameState, playerEntityHandle, collider);

		gameState->player.state = PLAYERSTATE_IDLE;

		SkinnedMeshInstance *skinnedMeshInstance =
			ArrayAdd_SkinnedMeshInstance(&gameState->skinnedMeshInstances);
		skinnedMeshInstance->meshRes = GetResource("Sparkus.b");
		skinnedMeshInstance->animationIdx = PLAYERANIM_IDLE;
		skinnedMeshInstance->animationTime = 0;
		EntityAssignSkinnedMesh(gameState, playerEntityHandle, skinnedMeshInstance);
	}

	// Init camera
	{
		Transform *playerTransform = GetEntityTransform(gameState,
				gameState->player.entityHandle);
		gameState->camPos = playerTransform->translation + v3{ 0, -3, 3};
		gameState->camPitch = PI * 0.6f;
	}

	// Test entities
	{
		const Resource *anvilRes = GetResource("anvil.b");
		MeshInstance anvilMesh;
		anvilMesh.meshRes = anvilRes;

		Collider anvilCollider;
		anvilCollider.type = COLLIDER_CONVEX_HULL;

		const Resource *collMeshRes = GetResource("anvil_collision.b");
		anvilCollider.convexHull.meshRes = collMeshRes;
		anvilCollider.convexHull.scale = 1.0f;

		Transform *transform;
		EntityHandle testEntityHandle = AddEntity(gameState, &transform);
		transform->translation = { -6.0f, 3.0f, 1.0f };
		transform->rotation = QUATERNION_IDENTITY;
		MeshInstance *meshInstance = ArrayAdd_MeshInstance(&gameState->meshInstances);
		*meshInstance = anvilMesh;
		EntityAssignMesh(gameState, testEntityHandle, meshInstance);
		Collider *collider = ArrayAdd_Collider(&gameState->colliders);
		*collider = anvilCollider;
		EntityAssignCollider(gameState, testEntityHandle, collider);

		testEntityHandle = AddEntity(gameState, &transform);
		transform->translation = { 5.0f, 4.0f, 1.0f };
		transform->rotation = QUATERNION_IDENTITY;
		meshInstance = ArrayAdd_MeshInstance(&gameState->meshInstances);
		*meshInstance = anvilMesh;
		EntityAssignMesh(gameState, testEntityHandle, meshInstance);
		collider = ArrayAdd_Collider(&gameState->colliders);
		*collider = anvilCollider;
		EntityAssignCollider(gameState, testEntityHandle, collider);

		testEntityHandle = AddEntity(gameState, &transform);
		transform->translation = { 3.0f, -4.0f, 1.0f };
		transform->rotation = QuaternionFromEulerZYX(v3{ 0, 0, HALFPI * -0.5f });
		meshInstance = ArrayAdd_MeshInstance(&gameState->meshInstances);
		*meshInstance = anvilMesh;
		EntityAssignMesh(gameState, testEntityHandle, meshInstance);
		collider = ArrayAdd_Collider(&gameState->colliders);
		*collider = anvilCollider;
		EntityAssignCollider(gameState, testEntityHandle, collider);

		testEntityHandle = AddEntity(gameState, &transform);
		transform->translation = { -8.0f, -4.0f, 1.0f };
		transform->rotation = QuaternionFromEulerZYX(v3{ HALFPI * 0.5f, 0, 0 });
		meshInstance = ArrayAdd_MeshInstance(&gameState->meshInstances);
		*meshInstance = anvilMesh;
		EntityAssignMesh(gameState, testEntityHandle, meshInstance);
		collider = ArrayAdd_Collider(&gameState->colliders);
		*collider = anvilCollider;
		EntityAssignCollider(gameState, testEntityHandle, collider);

		const Resource *cubeRes = GetResource("cube.b");
		testEntityHandle = AddEntity(gameState, &transform);
		transform->translation = { -3.0f, 5.0f, 1.0f };
		transform->rotation = QUATERNION_IDENTITY;
		meshInstance = ArrayAdd_MeshInstance(&gameState->meshInstances);
		meshInstance->meshRes = cubeRes;
		EntityAssignMesh(gameState, testEntityHandle, meshInstance);
		collider = ArrayAdd_Collider(&gameState->colliders);
		collider->type = COLLIDER_CUBE;
		collider->cube.radius = 1;
		collider->cube.offset = {};
		EntityAssignCollider(gameState, testEntityHandle, collider);

		testEntityHandle = AddEntity(gameState, &transform);
		transform->translation = { 0.0f, 5.0f, 1.0f };
		transform->rotation = QuaternionFromEulerZYX(v3{ 0, HALFPI, 0 });
		meshInstance = ArrayAdd_MeshInstance(&gameState->meshInstances);
		meshInstance->meshRes = cubeRes;
		EntityAssignMesh(gameState, testEntityHandle, meshInstance);
		collider = ArrayAdd_Collider(&gameState->colliders);
		collider->type = COLLIDER_CUBE;
		collider->cube.radius = 1;
		collider->cube.offset = {};
		EntityAssignCollider(gameState, testEntityHandle, collider);

		testEntityHandle = AddEntity(gameState, &transform);
		transform->translation = { 3.0f, 5.0f, 1.0f };
		transform->rotation = QuaternionFromEulerZYX(v3{ HALFPI, 0, 0 });
		meshInstance = ArrayAdd_MeshInstance(&gameState->meshInstances);
		meshInstance->meshRes = cubeRes;
		EntityAssignMesh(gameState, testEntityHandle, meshInstance);
		collider = ArrayAdd_Collider(&gameState->colliders);
		collider->type = COLLIDER_CUBE;
		collider->cube.radius = 1;
		collider->cube.offset = {};
		EntityAssignCollider(gameState, testEntityHandle, collider);

		const Resource *sphereRes = GetResource("sphere.b");
		testEntityHandle = AddEntity(gameState, &transform);
		transform->translation = { -6.0f, 7.0f, 1.0f };
		transform->rotation = QUATERNION_IDENTITY;
		meshInstance = ArrayAdd_MeshInstance(&gameState->meshInstances);
		meshInstance->meshRes = sphereRes;
		EntityAssignMesh(gameState, testEntityHandle, meshInstance);
		collider = ArrayAdd_Collider(&gameState->colliders);
		collider->type = COLLIDER_SPHERE;
		collider->sphere.radius = 1;
		collider->sphere.offset = {};
		EntityAssignCollider(gameState, testEntityHandle, collider);

		const Resource *cylinderRes = GetResource("cylinder.b");
		testEntityHandle = AddEntity(gameState, &transform);
		transform->translation = { -3.0f, 7.0f, 1.0f };
		transform->rotation = QUATERNION_IDENTITY;
		meshInstance = ArrayAdd_MeshInstance(&gameState->meshInstances);
		meshInstance->meshRes = cylinderRes;
		EntityAssignMesh(gameState, testEntityHandle, meshInstance);
		collider = ArrayAdd_Collider(&gameState->colliders);
		collider->type = COLLIDER_CYLINDER;
		collider->cylinder.radius = 1;
		collider->cylinder.height = 2;
		collider->cylinder.offset = {};
		EntityAssignCollider(gameState, testEntityHandle, collider);

		const Resource *capsuleRes = GetResource("capsule.b");
		testEntityHandle = AddEntity(gameState, &transform);
		transform->translation = { 0.0f, 7.0f, 2.0f };
		transform->rotation = QUATERNION_IDENTITY;
		meshInstance = ArrayAdd_MeshInstance(&gameState->meshInstances);
		meshInstance->meshRes = capsuleRes;
		EntityAssignMesh(gameState, testEntityHandle, meshInstance);
		collider = ArrayAdd_Collider(&gameState->colliders);
		collider->type = COLLIDER_CAPSULE;
		collider->capsule.radius = 1;
		collider->capsule.height = 2;
		collider->capsule.offset = {};
		EntityAssignCollider(gameState, testEntityHandle, collider);

		testEntityHandle = AddEntity(gameState, &transform);
		transform->translation = { 3.0f, 4.0f, 0.0f };
		transform->rotation = QUATERNION_IDENTITY;
		collider = ArrayAdd_Collider(&gameState->colliders);
		collider->type = COLLIDER_SPHERE;
		collider->sphere.radius = 0.3f;
		collider->sphere.offset = { 0, 0, 1 };
		EntityAssignCollider(gameState, testEntityHandle, collider);
		SkinnedMeshInstance *skinnedMeshInstance =
			ArrayAdd_SkinnedMeshInstance(&gameState->skinnedMeshInstances);
		skinnedMeshInstance->meshRes = GetResource("Jumper.b");
		skinnedMeshInstance->animationIdx = 0;
		skinnedMeshInstance->animationTime = 0;
		EntityAssignSkinnedMesh(gameState, testEntityHandle, skinnedMeshInstance);

		gameState->jumper = {};
		gameState->jumper.entityHandle = testEntityHandle;
	}
}

void ChangeState(GameState *gameState, PlayerState newState, PlayerAnim newAnim)
{
	//Log("Changed state: 0x%X -> 0x%X\n", gameState->player.state, newState);
	SkinnedMeshInstance *playerMesh = GetEntitySkinnedMesh(gameState, gameState->player.entityHandle);
	if (newAnim != PLAYERANIM_INVALID)
	{
		playerMesh->animationIdx = newAnim;
		playerMesh->animationTime = 0;
	}

	if (gameState->player.state & PLAYERSTATEFLAG_AIRBORNE &&
			!(newState & PLAYERSTATEFLAG_AIRBORNE))
	{
		// Landing
		Transform *transform = GetEntityTransform(gameState, gameState->player.entityHandle);
		const v2 vel = gameState->player.vel.xy;
		const v3 playerFw = QuaternionRotateVector(transform->rotation, v3{0,1,0});
		gameState->player.speed = (V2Dot(vel, playerFw.xy));
	}
	else if (!(gameState->player.state & PLAYERSTATEFLAG_AIRBORNE) &&
			newState & PLAYERSTATEFLAG_AIRBORNE)
	{
		// Going airborne
		Transform *transform = GetEntityTransform(gameState, gameState->player.entityHandle);
		const f32 speed = gameState->player.speed;
		const v3 playerFw = QuaternionRotateVector(transform->rotation, v3{0,1,0});
		gameState->player.vel.xy = playerFw.xy * speed;
	}

	gameState->player.state = newState;
}

DeviceProgram BindMaterial(GameState *gameState, const Resource *materialRes, const mat4 *modelMatrix)
{
	const Resource *shaderRes = materialRes->material.shaderRes;

	DeviceProgram programHandle = shaderRes->shader.programHandle;
	UseProgram(programHandle);
	DeviceUniform viewUniform = GetUniform(programHandle, "view");
	DeviceUniform projUniform = GetUniform(programHandle, "projection");
	DeviceUniform modelUniform = GetUniform(programHandle, "model");
	DeviceUniform albedoUniform = GetUniform(programHandle, "texAlbedo");
	DeviceUniform normalUniform = GetUniform(programHandle, "texNormal");

	UniformMat4Array(viewUniform, 1, gameState->viewMatrix.m);
	UniformMat4Array(projUniform, 1, gameState->projMatrix.m);
	UniformMat4Array(modelUniform, 1, modelMatrix->m);
	UniformInt(albedoUniform, 0);
	UniformInt(normalUniform, 1);

	for (int i = 0; i < materialRes->material.textureCount; ++i)
	{
		const Resource *tex = materialRes->material.textures[i];
		BindTexture(tex->texture.deviceTexture, i);
	}

	return programHandle;
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
	ImguiShowGameStateWindow(gameState);
	ImguiShowEditWindow(gameState);
	ImguiShowSceneWindow(gameState);
#endif

	if (deltaTime < 0 || deltaTime > 1)
	{
		Log("WARNING: Delta time out of range! %f\n", deltaTime);
		deltaTime = 1 / 60.0f;
	}

	// Update
	{
		// Move player
		Player *player = &gameState->player;
		Transform *playerTransform = GetEntityTransform(gameState, player->entityHandle);
#if EDITOR_PRESENT
		if (!g_debugContext->onFreeCam) // @Improve: change for editorTakingInput?
#endif
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

			if (inputDir.x == 0 && inputDir.y == 0)
			{
				const f32 deadzone = 0.2f;
				if (Abs(controller->leftStick.x) > deadzone)
					inputDir.x = (controller->leftStick.x - deadzone) / (1 - deadzone);
				if (Abs(controller->leftStick.y) > deadzone)
					inputDir.y = (controller->leftStick.y - deadzone) / (1 - deadzone);
			}

			f32 inputSqrLen = V2SqrLen(inputDir);
			if (inputSqrLen > 1)
			{
				inputDir /= Sqrt(inputSqrLen);
			}

			const f32 camYawSin = Sin(gameState->camYaw);
			const f32 camYawCos = Cos(gameState->camYaw);
			v3 worldInputDir =
			{
				 inputDir.x * camYawCos + inputDir.y * camYawSin,
				-inputDir.x * camYawSin + inputDir.y * camYawCos,
				0
			};

			// Turn
			if (inputSqrLen)
			{
				const f32 maxRotPerSecond = PI * 6.0f;
				const f32 maxRot = player->speed < 0.5f ? PI2 : maxRotPerSecond * deltaTime;
				v3 playerFw = QuaternionRotateVector(playerTransform->rotation, v3{0,1,0});
				f32 playerRelativeInputY = V2Dot(worldInputDir.xy, playerFw.xy);
				f32 playerRelativeInputX = V2Dot(worldInputDir.xy, v2{ playerFw.y, -playerFw.x });

				f32 deltaYaw = Atan2(-playerRelativeInputX, playerRelativeInputY);
				deltaYaw = Max(-maxRot, Min(maxRot, deltaYaw));

				v4 rot = QuaternionFromEulerZYX(v3{ 0, 0, deltaYaw });
				playerTransform->rotation = QuaternionMultiply(playerTransform->rotation, rot);
			}
			v3 playerFw = QuaternionRotateVector(playerTransform->rotation, v3{0,1,0});

			// Move
			bool moveInputNeutral = inputDir.x == 0 && inputDir.y == 0;
			if (player->state & PLAYERSTATEFLAG_AIRBORNE)
			{
				// Acceleration
				player->vel.xy += worldInputDir.xy * 20.0f * deltaTime;

				// Drag
				player->vel.xy *= Pow(0.1f, deltaTime);
			}
			else
			{
				f32 inputLen = V2Length(inputDir);

				// On the ground
				if (player->state != PLAYERSTATE_RUNNING && !moveInputNeutral)
				{
					ChangeState(gameState, PLAYERSTATE_RUNNING, PLAYERANIM_RUN);
					player->speed = Max(player->speed, inputLen * 1.0f);
				}
				else
				{
					if (player->speed <= 0 && player->state != PLAYERSTATE_IDLE)
					{
						ChangeState(gameState, PLAYERSTATE_IDLE, PLAYERANIM_IDLE);
						player->speed = 0;
					}
					else
					{
						// Acceleration
						player->speed += deltaTime * 45.0f * inputLen;

						// Drag
						player->speed *= Pow(0.014f, deltaTime);
						player->speed -= deltaTime * 10.0f * (1.0f - inputLen) * Sign(player->speed);
					}
				}
			}

			if (player->state & PLAYERSTATEFLAG_AIRBORNE)
			{
				player->speed = V2Length(player->vel.xy);
			}
			else
			{
				player->vel.xy = playerFw.xy * player->speed;
			}

			// Jump
			if (!(player->state & PLAYERSTATEFLAG_AIRBORNE) && controller->jump.endedDown &&
					controller->jump.changed)
			{
				player->vel.z = 18.0f;
				ChangeState(gameState, PLAYERSTATE_JUMP, PLAYERANIM_JUMP);
			}
			else if (player->state == PLAYERSTATE_JUMP)
			{
				if (!controller->jump.endedDown && player->vel.z > 0)
				{
					player->vel.z *= 0.5f;
					ChangeState(gameState, PLAYERSTATE_FALL, PLAYERANIM_INVALID);
				}
			}

			// Gravity
			if (player->state & PLAYERSTATEFLAG_AIRBORNE)
			{
				player->vel.z -= 40.0f * deltaTime;
				if (player->vel.z < -75.0f)
					player->vel.z = -75.0f;
			}
			else
			{
				player->vel.z = 0;
			}

			playerTransform->translation += player->vel * deltaTime;
		}

		const f32 groundRayDist = (player->state & PLAYERSTATEFLAG_AIRBORNE) ? 1.0f : 1.5f;

		// Collision
		bool touchedGround = false;

		Collider *playerCollider = GetEntityCollider(gameState, player->entityHandle);
		for (u32 colliderIdx = 0; colliderIdx < gameState->colliders.size; ++colliderIdx)
		{
			Collider *collider = &gameState->colliders[colliderIdx];
			Transform *transform = GetEntityTransform(gameState, collider->entityHandle);
			ASSERT(transform); // There should never be an orphaned collider.

			if (collider != playerCollider)
			{
				v3 origin = playerTransform->translation + v3{ 0, 0, 1 };
				v3 dir = { 0, 0, -groundRayDist };
				v3 hit;
				v3 hitNor;
				if (RayColliderIntersection(origin, dir, false, transform, collider, &hit, &hitNor) &&
						hitNor.z > 0.7f)
				{
					playerTransform->translation.z = hit.z;
					touchedGround = true;
				}
				else
				{
					GJKResult gjkResult = GJKTest(playerTransform, transform,
							playerCollider, collider);
					if (gjkResult.hit)
					{
						// @Improve: better collision against multiple colliders?
						v3 depenetration = ComputeDepenetration(gjkResult, playerTransform,
								transform, playerCollider, collider);
						playerTransform->translation += depenetration;
					}
				}
			}
		}

		// Ray testing
		{
			v3 origin = playerTransform->translation + v3{ 0, 0, 1 };
			v3 dir = { 0, 0, -groundRayDist };
			v3 hit;
			Triangle triangle;
			if (HitTest(gameState, origin, dir, false, &hit, &triangle))
			{
				playerTransform->translation.z = hit.z;
				touchedGround = true;
			}

			origin = playerTransform->translation + v3{ 0, 0, 1 };
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
				if (HitTest(gameState, origin, dirs[i], false, &hit, &triangle))
				{
					// Ignore slopes
					if (triangle.normal.z > 0.05f)
						continue;

					f32 dot = V3Dot(triangle.normal, dirs[i] / rayLen);
					if (dot > -0.707f && dot < 0.707f)
						continue;

					hit.z = playerTransform->translation.z;

					f32 aDistAlongNormal = V3Dot(triangle.a - playerTransform->translation, triangle.normal);
					if (aDistAlongNormal < 0) aDistAlongNormal = -aDistAlongNormal;
					if (aDistAlongNormal < playerRadius)
					{
						f32 factor = playerRadius - aDistAlongNormal;
						playerTransform->translation += triangle.normal * factor;
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

		if (playerTransform->translation.z < -1)
		{
			playerTransform->translation.z = 1;
			player->vel = {};
		}

		// Move camera
#if EDITOR_PRESENT
		if (!g_debugContext->onFreeCam)
#endif
		{
			const f32 camRotSpeed = 4.0f;

			f32 pitchDelta = 0;
			f32 yawDelta = 0;

			if (controller->camUp.endedDown)
				pitchDelta =  camRotSpeed * deltaTime;
			else if (controller->camDown.endedDown)
				pitchDelta = -camRotSpeed * deltaTime;

			if (controller->camLeft.endedDown)
				yawDelta = -camRotSpeed * deltaTime;
			else if (controller->camRight.endedDown)
				yawDelta =  camRotSpeed * deltaTime;

			const f32 deadzone = 0.2f;
			if (Abs(controller->rightStick.x) > deadzone)
				yawDelta =   controller->rightStick.x * camRotSpeed * deltaTime;
			if (Abs(controller->rightStick.y) > deadzone)
				pitchDelta = controller->rightStick.y * camRotSpeed * deltaTime;

			gameState->camPos.z = playerTransform->translation.z + 3;
			v3 local = gameState->camPos - playerTransform->translation;
			local = QuaternionRotateVector(QuaternionFromEulerZYX(v3{0,0,yawDelta}), local);
			gameState->camPos = local + playerTransform->translation;

			// Look at
			v3 dir = playerTransform->translation - gameState->camPos;
			f32 targetYaw = Atan2(dir.x, dir.y);
			gameState->camYaw = targetYaw;

			// Follow
			f32 dist = V2Length(dir.xy);
			const f32 followMult = 3.0f;
			gameState->camPos += v3{ dir.x, dir.y, 0 } * followMult * (dist - 3) * deltaTime;
		}

		// Update enemies
		//if (0) // ANNOYING
		{
			Transform *jumper = GetEntityTransform(gameState, gameState->jumper.entityHandle);

			// Ground detection
			v3 origin = jumper->translation + v3{ 0, 0, 0.7f };
			const f32 rayDist = 0.9f;
			v3 rayDir = { 0, 0, -rayDist };
			v3 hit;
			Triangle triangle;
			if (HitTest(gameState, origin, rayDir, false, &hit, &triangle))
			{
				jumper->translation.z = hit.z;
				gameState->jumper.zVel = 0;
			}
			else
			{
				gameState->jumper.zVel -= 20.0f * deltaTime; // @Fix: not frame-independant
				jumper->translation.z += gameState->jumper.zVel * deltaTime;
			}

			if (gameState->jumper.state == JUMPERSTATE_IDLE)
			{
				f32 dist = V3Length(playerTransform->translation - jumper->translation);
				if (dist < 8.0f && dist > 1.0f)
				{
					gameState->jumper.target = gameState->player.entityHandle;
					gameState->jumper.state = JUMPERSTATE_CHASING;
				}
			}
			else if (gameState->jumper.state == JUMPERSTATE_CHASING)
			{
				f32 dist = INFINITY;
				v3 dir;

				Transform *target = GetEntityTransform(gameState, gameState->jumper.target);
				if (target)
				{
					dir = target->translation - jumper->translation;
					dist = V3Length(dir);
				}

				if (dist > 10.0f || dist < 0.8f)
				{
					gameState->jumper.target = ENTITY_HANDLE_INVALID;
					gameState->jumper.state = JUMPERSTATE_IDLE;
				}
				else
				{
					dir /= dist;
					jumper->translation.xy += dir.xy * 3.0f * deltaTime;
					jumper->rotation = QuaternionFromAxisAngle(v3{0,0,1}, -Atan2(dir.x, dir.y));
				}
			}
		}

		// Update skinned meshes
		for (u32 meshInstanceIdx = 0; meshInstanceIdx < gameState->skinnedMeshInstances.size;
				++meshInstanceIdx)
		{
			SkinnedMeshInstance *skinnedMeshInstance = &gameState->skinnedMeshInstances[meshInstanceIdx];

#if DEBUG_BUILD
			// Avoid crash when adding these in the editor
			if (!skinnedMeshInstance->meshRes)
				continue;
#endif

			for (int i = 0; i < 128; ++i)
				skinnedMeshInstance->jointTranslations[i] = {};
			for (int i = 0; i < 128; ++i)
				skinnedMeshInstance->jointRotations[i] = QUATERNION_IDENTITY;
			for (int i = 0; i < 128; ++i)
				skinnedMeshInstance->jointScales[i] = v3{ 1, 1, 1 };

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

				skinnedMeshInstance->jointTranslations[jointIdx] = transform.translation;
				skinnedMeshInstance->jointRotations[jointIdx] = transform.rotation;
				skinnedMeshInstance->jointScales[jointIdx] = transform.scale;
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
		}

		// Update particle systems
		auto SimulateParticle = [](ParticleSystem *particleSystem, int i, f32 dt)
		{
			Particle *particle = &particleSystem->particles[i];
			ParticleBookkeep *bookkeep = &particleSystem->bookkeeps[i];

			bookkeep->lifeTime += dt;
			if (bookkeep->lifeTime > bookkeep->duration)
			{
				// Despawn
				particleSystem->alive[i] = false;
			}

			bookkeep->velocity += particleSystem->acceleration * dt;
			particle->pos += bookkeep->velocity * dt;
			particle->color += particleSystem->colorDelta * dt;
			particle->size += particleSystem->sizeDelta * dt;
		};

		auto SpawnParticle = [](ParticleSystem *particleSystem, Transform *transform)
		{
			// Spawn particle
			int newParticleIdx = -1;
			const int maxCount = ArrayCount(particleSystem->particles);
			for (int i = 0; i < maxCount; ++i)
			{
				if (!particleSystem->alive[i])
				{
					newParticleIdx = i;
					break;
				}
			}

			if (newParticleIdx != -1)
			{
				particleSystem->alive[newParticleIdx] = true;
				Particle *p = &particleSystem->particles[newParticleIdx];
				ParticleBookkeep *b = &particleSystem->bookkeeps[newParticleIdx];

				b->lifeTime = 0;
				b->duration = particleSystem->maxLife;

				v3 spread =
				{
					GetRandomF32() - 0.5f,
					GetRandomF32() - 0.5f,
					GetRandomF32() - 0.5f
				};
				spread = V3Scale(spread, particleSystem->initialVelSpread);
				b->velocity = particleSystem->initialVel + spread;
				// @Speed: rotate velocity outside loop (might need fw, right, up vectors for spread)
				b->velocity = QuaternionRotateVector(transform->rotation, b->velocity);

				p->pos = transform->translation + particleSystem->offset;

				p->size = particleSystem->initialSize +
					(GetRandomF32() - 0.5f) * particleSystem->sizeSpread;

				v4 colorSpread =
				{
					GetRandomF32() - 0.5f,
					GetRandomF32() - 0.5f,
					GetRandomF32() - 0.5f,
					GetRandomF32() - 0.5f
				};
				colorSpread = V4Scale(colorSpread, particleSystem->colorSpread);
				p->color = particleSystem->initialColor +
					colorSpread;

				// Simulate a bit to compensate spawn delay
				//f32 t = particleSystem->timer;
				//SimulateParticle(particleSystem, newParticleIdx, t);
			}
			return newParticleIdx;
		};

		for (u32 partSysIdx = 0; partSysIdx < gameState->particleSystems.size; ++partSysIdx)
		{
			ParticleSystem *particleSystem = &gameState->particleSystems[partSysIdx];
			Transform *transform = GetEntityTransform(gameState, particleSystem->entityHandle);

			const int maxCount = ArrayCount(particleSystem->particles);

			if (particleSystem->timer == 0)
			{
				// Burst
				i32 spawned = 0;
				while (spawned < particleSystem->burstCount)
				{
					i32 newParticleIdx = SpawnParticle(particleSystem, transform);
					if (newParticleIdx == -1)
						break;
					++spawned;
				}
			}

			particleSystem->timer += deltaTime;
			for (int i = 0; i < maxCount; ++i)
			{
				if (!particleSystem->alive[i])
				{
					particleSystem->particles[i].size = 0;
					continue;
				}

				SimulateParticle(particleSystem, i, deltaTime);
			}

			if (particleSystem->spawnRate <= 0)
			{
				continue;
			}

			while (particleSystem->timer > particleSystem->spawnRate)
			{
				i32 newParticleIdx = SpawnParticle(particleSystem, transform);
				if (newParticleIdx == -1)
					break;

				// Simulate a bit to compensate spawn delay
				f32 t = particleSystem->timer;
				SimulateParticle(particleSystem, newParticleIdx, t);

				particleSystem->timer -= particleSystem->spawnRate;
			}
		}

#if EDITOR_PRESENT
		// Free cam
		if (controller->f1.endedDown && controller->f1.changed)
		{
			g_debugContext->onFreeCam = !g_debugContext->onFreeCam;
		}
		if (g_debugContext->onFreeCam)
		{
			if (controller->mouseRight.endedDown)
			{
				static v2 oldMousePos = controller->mousePos;

				if (!controller->mouseRight.changed)
				{
					v2 delta = controller->mousePos - oldMousePos;
					gameState->camYaw += delta.x;
					gameState->camPitch -= delta.y;
				}

				oldMousePos = controller->mousePos;

				v3 camFw = -Mat4ColumnV3(gameState->viewMatrix, 2);
				if (controller->up.endedDown)
				{
					gameState->camPos += camFw * 8.0f * deltaTime;
				}
				else if (controller->down.endedDown)
				{
					gameState->camPos -= camFw * 8.0f * deltaTime;
				}

				v3 camRight = Mat4ColumnV3(gameState->viewMatrix, 0);
				if (controller->right.endedDown)
				{
					gameState->camPos += camRight * 8.0f * deltaTime;
				}
				else if (controller->left.endedDown)
				{
					gameState->camPos -= camRight * 8.0f * deltaTime;
				}
			}
		}

		// Mouse picking
		PickingResult pickingResult = PICKING_NOTHING;
		{
			v4 worldCursorPos = { controller->mousePos.x, controller->mousePos.y, -1, 1 };
			mat4 invViewMatrix = gameState->invViewMatrix;
			mat4 invProjMatrix = Mat4Inverse(gameState->projMatrix);
			worldCursorPos = Mat4TransformV4(invProjMatrix, worldCursorPos);
			worldCursorPos /= worldCursorPos.w;
			worldCursorPos = Mat4TransformV4(invViewMatrix, worldCursorPos);
			v3 cursorXYZ = { worldCursorPos.x, worldCursorPos.y, worldCursorPos.z };

			v3 camPos = gameState->camPos;
			v3 origin = camPos;
			v3 dir = cursorXYZ - origin;
			g_debugContext->hoveredEntity = ENTITY_HANDLE_INVALID;
			f32 closestDistance = INFINITY;
			for (u32 colliderIdx = 0; colliderIdx < gameState->colliders.size; ++colliderIdx)
			{
				Collider *collider = &gameState->colliders[colliderIdx];
				Transform *transform = GetEntityTransform(gameState, collider->entityHandle);
				ASSERT(transform);
				v3 hit;
				v3 hitNor;
				if (RayColliderIntersection(origin, dir, true, transform,
							collider, &hit, &hitNor))
				{
					f32 dist = V3SqrLen(hit - camPos);
					if (dist < closestDistance)
					{
						closestDistance = dist;
						g_debugContext->hoveredEntity = collider->entityHandle;
						pickingResult = PICKING_ENTITY;
					}
				}
			}

			// @Hack: hard coded gizmo colliders!
			if (Transform *selectedEntity = GetEntityTransform(gameState, g_debugContext->selectedEntity))
			{
				Collider gizmoCollider = {};
				gizmoCollider.type = COLLIDER_CONVEX_HULL;
				gizmoCollider.convexHull.meshRes = GetResource("editor_arrow_collision.b");

				f32 dist = V3Length(gameState->camPos - selectedEntity->translation);
				gizmoCollider.convexHull.scale = 0.2f * dist;

				Transform gizmoZT = *selectedEntity;
				Transform gizmoXT = gizmoZT;
				Transform gizmoYT = gizmoZT;

				gizmoXT.rotation = QuaternionMultiply(gizmoXT.rotation,
						QuaternionFromEulerZYX({ 0, HALFPI, 0 }));
				gizmoYT.rotation = QuaternionMultiply(gizmoYT.rotation,
						QuaternionFromEulerZYX({ -HALFPI, 0, 0 }));

				v3 hit;
				v3 hitNor;
				f32 hitSqrDistToCam = INFINITY;
				if (RayColliderIntersection(origin, dir, true, &gizmoXT, &gizmoCollider, &hit, &hitNor))
				{
					pickingResult = PICKING_GIZMO_X;
					hitSqrDistToCam = V3SqrLen(hit - camPos);
				}
				if (RayColliderIntersection(origin, dir, true, &gizmoYT, &gizmoCollider, &hit, &hitNor))
				{
					f32 newSqrDist = V3SqrLen(hit - camPos);
					if (newSqrDist < hitSqrDistToCam)
					{
						pickingResult = PICKING_GIZMO_Y;
						hitSqrDistToCam = newSqrDist;
					}
				}
				if (RayColliderIntersection(origin, dir, true, &gizmoZT, &gizmoCollider, &hit, &hitNor))
				{
					f32 newSqrDist = V3SqrLen(hit - camPos);
					if (newSqrDist < hitSqrDistToCam)
					{
						pickingResult = PICKING_GIZMO_Z;
						hitSqrDistToCam = newSqrDist;
					}
				}

				gizmoCollider.type = COLLIDER_CONVEX_HULL;
				gizmoCollider.convexHull.meshRes = GetResource("editor_circle_collision.b");

				if (RayColliderIntersection(origin, dir, true, &gizmoXT, &gizmoCollider, &hit, &hitNor))
				{
					f32 newSqrDist = V3SqrLen(hit - camPos);
					if (newSqrDist < hitSqrDistToCam)
					{
						pickingResult = PICKING_GIZMO_ROT_X;
						hitSqrDistToCam = newSqrDist;
					}
				}
				if (RayColliderIntersection(origin, dir, true, &gizmoYT, &gizmoCollider, &hit, &hitNor))
				{
					f32 newSqrDist = V3SqrLen(hit - camPos);
					if (newSqrDist < hitSqrDistToCam)
					{
						pickingResult = PICKING_GIZMO_ROT_Y;
						hitSqrDistToCam = newSqrDist;
					}
				}
				if (RayColliderIntersection(origin, dir, true, &gizmoZT, &gizmoCollider, &hit, &hitNor))
				{
					f32 newSqrDist = V3SqrLen(hit - camPos);
					if (newSqrDist < hitSqrDistToCam)
					{
						pickingResult = PICKING_GIZMO_ROT_Z;
						hitSqrDistToCam = newSqrDist;
					}
				}
			}
		}

		//Editor stuff
		{
			static bool grabbingX = false;
			static bool grabbingY = false;
			static bool grabbingZ = false;
			static bool rotatingX = false;
			static bool rotatingY = false;
			static bool rotatingZ = false;
			static v2 oldMousePos = controller->mousePos;

			if (controller->mouseLeft.endedDown && controller->mouseLeft.changed)
			{
				if (pickingResult == PICKING_GIZMO_X)
					grabbingX = true;
				else if (pickingResult == PICKING_GIZMO_Y)
					grabbingY = true;
				else if (pickingResult == PICKING_GIZMO_Z)
					grabbingZ = true;
				else if (pickingResult == PICKING_GIZMO_ROT_X)
					rotatingX = true;
				else if (pickingResult == PICKING_GIZMO_ROT_Y)
					rotatingY = true;
				else if (pickingResult == PICKING_GIZMO_ROT_Z)
					rotatingZ = true;
				else if (pickingResult == PICKING_NOTHING)
					g_debugContext->selectedEntity = ENTITY_HANDLE_INVALID;
				else
					g_debugContext->selectedEntity = g_debugContext->hoveredEntity;
			}
			else if (!controller->mouseLeft.endedDown)
			{
				grabbingX = false;
				grabbingY = false;
				grabbingZ = false;
				rotatingX = false;
				rotatingY = false;
				rotatingZ = false;
			}

			if (Transform *selectedEntity = GetEntityTransform(gameState, g_debugContext->selectedEntity))
			{
				v4 entityScreenPos = V4Point(selectedEntity->translation);
				entityScreenPos = Mat4TransformV4(gameState->viewMatrix, entityScreenPos);
				entityScreenPos = Mat4TransformV4(gameState->projMatrix, entityScreenPos);
				entityScreenPos /= entityScreenPos.w;

				if (grabbingX || grabbingY || grabbingZ)
				{
					v3 dir = { (f32)grabbingX, (f32)grabbingY, (f32)grabbingZ };
					v3 worldDir = QuaternionRotateVector(selectedEntity->rotation, dir);

					v4 offsetScreenPos = V4Point(selectedEntity->translation + worldDir);
					offsetScreenPos = Mat4TransformV4(gameState->viewMatrix, offsetScreenPos);
					offsetScreenPos = Mat4TransformV4(gameState->projMatrix, offsetScreenPos);
					offsetScreenPos /= offsetScreenPos.w;

					v2 screenSpaceDragDir = offsetScreenPos.xy - entityScreenPos.xy;

					v2 mouseDelta = controller->mousePos - oldMousePos;
					f32 delta = V2Dot(screenSpaceDragDir, mouseDelta) / V2SqrLen(screenSpaceDragDir);
					selectedEntity->translation += worldDir * delta;
				}
				else if (rotatingX || rotatingY || rotatingZ)
				{
					// Draw out two vectors on the screen, one from entity to the old cursor
					// position, another from entity to new cursor position. Then calculate angle
					// between them and rotate entity by that angle delta.
					v2 oldDelta = oldMousePos - entityScreenPos.xy;
					v2 newDelta = controller->mousePos - entityScreenPos.xy;
					f32 theta = Atan2(newDelta.y, newDelta.x) - Atan2(oldDelta.y, oldDelta.x);

					v3 euler = { rotatingX * theta, rotatingY * theta, rotatingZ * theta };

					selectedEntity->rotation = QuaternionMultiply(selectedEntity->rotation,
							QuaternionFromEulerZYX(euler));
				}
			}

			oldMousePos = controller->mousePos;
		}
#endif

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
		BindFrameBuffer(gameState->frameBuffer);
		{
			static u32 lastW, lastH;
			u32 w, h;
			GetWindowSize(&w, &h);
			if (lastW != w || lastH != h)
			{
				SetViewport(0, 0, w, h);

				// Update size of frame buffer textures
				SendTexture(gameState->frameBufferColorTex, nullptr, w, h, RENDERIMAGECOMPONENTS_3);
				SendTexture(gameState->frameBufferDepthTex, nullptr, w, h, RENDERIMAGECOMPONENTS_DEPTH24);

				lastW = w;
				lastH = h;
			}
		}

		// Projection matrix
		mat4 *proj = &gameState->projMatrix;
		{
			const f32 fov = HALFPI;
			const f32 near = 0.01f;
			const f32 far = 2000.0f;
			const f32 aspectRatio = (16.0f / 9.0f);

			const f32 top = Tan(HALFPI - fov / 2.0f);
			const f32 right = top / aspectRatio;

			*proj =
			{
				right, 0.0f, 0.0f, 0.0f,
				0.0f, top, 0.0f, 0.0f,
				0.0f, 0.0f, -(far + near) / (far - near), -1.0f,
				0.0f, 0.0f, -(2.0f * far * near) / (far - near), 0.0f
			};
		}

		// View matrix
		mat4 *view = &gameState->viewMatrix;
		{
			f32 yawSin = Sin(gameState->camYaw);
			f32 yawCos = Cos(gameState->camYaw);
			f32 pitchSin = Sin(gameState->camPitch);
			f32 pitchCos = Cos(gameState->camPitch);
			//Log("%f, %f\n", pitchSin, pitchCos);
			// NOTE: 0 yaw means looking towards +Y!
			v3 camFw = { yawSin * pitchSin, yawCos * pitchSin, pitchCos };

			const v3 right = V3Normalize(V3Cross(camFw, v3{0,0,1}));
			const v3 up = V3Cross(right, camFw);
			const v3 pos = gameState->camPos;
			gameState->invViewMatrix =
			{
				right.x,	right.y,	right.z,	0.0f,
				up.x,		up.y,		up.z,		0.0f,
				-camFw.x,	-camFw.y,	-camFw.z,	0.0f,
				pos.x,		pos.y,		pos.z,		1.0f
			};

			*view = Mat4Inverse(gameState->invViewMatrix);
		}

		const v4 clearColor = { 0.95f, 0.88f, 0.05f, 1.0f };
		ClearColorBuffer(clearColor);
		ClearDepthBuffer();

		// Meshes
		for (u32 meshInstanceIdx = 0; meshInstanceIdx < gameState->meshInstances.size;
				++meshInstanceIdx)
		{
			MeshInstance *meshInstance = &gameState->meshInstances[meshInstanceIdx];
			const Resource *meshRes = meshInstance->meshRes;
#if EDITOR_PRESENT
			// While editing this resource can be null. Don't check otherwise, to avoid an
			// unnecessary branch.
			if (meshRes)
#endif
			{
				Transform *transform = GetEntityTransform(gameState, meshInstance->entityHandle);

				const mat4 model = Mat4Compose(transform->translation, transform->rotation);

				// @Improve: don't rebind program, textures and all for every mesh! Maybe sort by
				// material.
				const Resource *materialRes = meshRes->mesh.materialRes;
#if DEBUG_BUILD
				if (!materialRes)
				{
					materialRes = GetResource("material_default.b");
				}
#endif

				BindMaterial(gameState, materialRes, &model);

				RenderIndexedMesh(meshRes->mesh.deviceMesh);
			}
		}

		// Level
		{
			LevelGeometry *level = &gameState->levelGeometry;

			const Resource *materialRes = level->renderMesh->mesh.materialRes;
#if DEBUG_BUILD
			if (!materialRes)
			{
				materialRes = GetResource("material_default.b");
			}
#endif
			BindMaterial(gameState, materialRes, &MAT4_IDENTITY);

			RenderIndexedMesh(level->renderMesh->mesh.deviceMesh);
		}

		// Draw skinned meshes
		{
			for (u32 meshInstanceIdx = 0; meshInstanceIdx < gameState->skinnedMeshInstances.size;
					++meshInstanceIdx)
			{
				SkinnedMeshInstance *skinnedMeshInstance = &gameState->skinnedMeshInstances[meshInstanceIdx];

#if EDITOR_PRESENT
				// Avoid crash when adding these in the editor
				if (!skinnedMeshInstance->meshRes)
					continue;
#endif
				const Resource *skinnedMeshRes = skinnedMeshInstance->meshRes;
				const ResourceSkinnedMesh *skinnedMesh = &skinnedMeshRes->skinnedMesh;

				Transform *transform = GetEntityTransform(gameState, skinnedMeshInstance->entityHandle);
				const mat4 model = Mat4Compose(transform->translation, transform->rotation);

				// @Improve: don't rebind program, textures and all for every mesh! Maybe sort by
				// material.
				const Resource *materialRes = skinnedMesh->materialRes;
#if DEBUG_BUILD
				if (!materialRes)
				{
					materialRes = GetResource("material_default_skinned.b");
				}
#endif
				DeviceProgram program = BindMaterial(gameState, materialRes, &model);

				DeviceUniform jointTranslationsUniform = GetUniform(program, "jointTranslations");
				DeviceUniform jointRotationsUniform = GetUniform(program, "jointRotations");
				DeviceUniform jointScalesUniform = GetUniform(program, "jointScales");

				UniformV3Array(jointTranslationsUniform, 128, skinnedMeshInstance->jointTranslations[0].v);
				UniformV4Array(jointRotationsUniform, 128, skinnedMeshInstance->jointRotations[0].v);
				UniformV3Array(jointScalesUniform, 128, skinnedMeshInstance->jointScales[0].v);

				RenderIndexedMesh(skinnedMesh->deviceMesh);
			}
		}

		// Draw particles
		{
			UseProgram(gameState->particleSystemProgram);
			DeviceUniform viewUniform = GetUniform(gameState->particleSystemProgram, "view");
			DeviceUniform projUniform = GetUniform(gameState->particleSystemProgram, "projection");
			UniformMat4Array(projUniform, 1, proj->m);
			UniformMat4Array(viewUniform, 1, view->m);

			DeviceUniform texUniform = GetUniform(gameState->particleSystemProgram, "tex");
			UniformInt(texUniform, 0);
			DeviceUniform depthBufferUniform = GetUniform(gameState->particleSystemProgram, "depthBuffer");
			UniformInt(depthBufferUniform, 1);

			DeviceUniform atlasIdxUniform = GetUniform(gameState->particleSystemProgram, "atlasIdx");

			const Resource *tex = GetResource("particle_atlas.b");
			BindTexture(tex->texture.deviceTexture, 0);
			BindTexture(gameState->frameBufferDepthTex, 1);

			DisableDepthTest();
			EnableAlphaBlending();
			const u32 particleMeshAttribs = RENDERATTRIB_VERTEXNUM;
			const u32 particleInstAttribs = RENDERATTRIB_POSITION | RENDERATTRIB_COLOR4 | RENDERATTRIB_1CUSTOMF32;

			for (u32 partSysIdx = 0; partSysIdx < gameState->particleSystems.size;
					++partSysIdx)
			{
				ParticleSystem *particleSystem = &gameState->particleSystems[partSysIdx];

				UniformInt(atlasIdxUniform, particleSystem->atlasIdx);

				const int maxCount = ArrayCount(particleSystem->particles);

				// Sort!
				static v3 camPos;
				camPos = gameState->camPos;
				auto compareParticles = [](const void *a, const void *b)
				{
					f32 aDist = V3SqrLen(((Particle *)a)->pos - camPos);
					f32 bDist = V3SqrLen(((Particle *)b)->pos - camPos);
					return (int)((aDist < bDist) - (aDist > bDist));
				};
				Particle *particlesCopy = (Particle *)FrameAlloc(sizeof(particleSystem->particles));
				memcpy(particlesCopy, particleSystem->particles, sizeof(particleSystem->particles));
				qsort(particlesCopy, maxCount, sizeof(Particle), compareParticles);

				SendMesh(&particleSystem->deviceBuffer,
						particlesCopy,
						maxCount,
						sizeof(particleSystem->particles[0]), true);

				RenderMeshInstanced(gameState->particleMesh, particleSystem->deviceBuffer,
						particleMeshAttribs, particleInstAttribs);
			}

			DisableAlphaBlending();
			EnableDepthTest();
		}

#if DEBUG_BUILD
		// Debug meshes
		{
			DebugGeometryBuffer *dgb = &g_debugContext->debugGeometryBuffer;
			if (g_debugContext->wireframeDebugDraws)
				SetFillMode(RENDER_LINE);

			DisableDepthTest();

			UseProgram(g_debugContext->debugDrawProgram);
			DeviceUniform viewUniform = GetUniform(g_debugContext->debugDrawProgram, "view");
			DeviceUniform projUniform = GetUniform(g_debugContext->debugDrawProgram, "projection");
			UniformMat4Array(projUniform, 1, proj->m);
			UniformMat4Array(viewUniform, 1, view->m);

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
				UniformMat4Array(projUniform, 1, proj->m);
				UniformMat4Array(viewUniform, 1, view->m);

				SendMesh(&dgb->cubePositionsBuffer,
						dgb->debugCubes,
						dgb->debugCubeCount, sizeof(DebugCube), true);

				u32 meshAttribs = RENDERATTRIB_POSITION;
				u32 instAttribs = RENDERATTRIB_POSITION | RENDERATTRIB_COLOR3 |
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

#if EDITOR_PRESENT
		// Selected entity
		if (Transform *selectedEntity = GetEntityTransform(gameState, g_debugContext->selectedEntity))
		{
			SetFillMode(RENDER_LINE);

			UseProgram(g_debugContext->editorSelectedProgram);
			DeviceUniform viewUniform = GetUniform(g_debugContext->editorSelectedProgram, "view");
			UniformMat4Array(viewUniform, 1, view->m);
			DeviceUniform projUniform = GetUniform(g_debugContext->editorSelectedProgram, "projection");
			UniformMat4Array(projUniform, 1, proj->m);
			DeviceUniform modelUniform = GetUniform(g_debugContext->editorSelectedProgram, "model");

			DeviceUniform isSkinnedUniform = GetUniform(g_debugContext->editorSelectedProgram, "skinned");

			static f32 t = 0;
			t += deltaTime;
			DeviceUniform timeUniform = GetUniform(g_debugContext->editorSelectedProgram, "time");
			UniformFloat(timeUniform, t);

			const mat4 model = Mat4Compose(selectedEntity->translation, selectedEntity->rotation);
			UniformMat4Array(modelUniform, 1, model.m);

			MeshInstance *meshInstance = GetEntityMesh(gameState, g_debugContext->selectedEntity);
			if (meshInstance)
			{
				const Resource *meshRes = meshInstance->meshRes;
				if (meshRes)
				{
					UniformInt(isSkinnedUniform, false);
					RenderIndexedMesh(meshInstance->meshRes->mesh.deviceMesh);
				}
			}

			SkinnedMeshInstance *skinnedMeshInstance = GetEntitySkinnedMesh(gameState, g_debugContext->selectedEntity);
			if (skinnedMeshInstance)
			{
				UniformInt(isSkinnedUniform, true);

				const Resource *skinnedMeshRes = skinnedMeshInstance->meshRes;
				if (skinnedMeshRes)
				{
					const ResourceSkinnedMesh *skinnedMesh = &skinnedMeshRes->skinnedMesh;

					DeviceUniform jointTranslationsUniform = GetUniform(g_debugContext->editorSelectedProgram, "jointTranslations");
					DeviceUniform jointRotationsUniform = GetUniform(g_debugContext->editorSelectedProgram, "jointRotations");
					DeviceUniform jointScalesUniform = GetUniform(g_debugContext->editorSelectedProgram, "jointScales");

					UniformV3Array(jointTranslationsUniform, 128, skinnedMeshInstance->jointTranslations[0].v);
					UniformV4Array(jointRotationsUniform, 128, skinnedMeshInstance->jointRotations[0].v);
					UniformV3Array(jointScalesUniform, 128, skinnedMeshInstance->jointScales[0].v);

					RenderIndexedMesh(skinnedMesh->deviceMesh);
				}
			}

			SetFillMode(RENDER_FILL);
		}
#endif

		UnbindFrameBuffer();
		{
			static u32 lastW, lastH;
			u32 w, h;
			GetWindowSize(&w, &h);
			if (lastW != w || lastH != h)
			{
				SetViewport(0, 0, w, h);
				lastW = w;
				lastH = h;
			}
		}

		DisableDepthTest();

		UseProgram(gameState->frameBufferProgram);

		DeviceUniform colorUniform = GetUniform(gameState->frameBufferProgram, "texColor");
		UniformInt(colorUniform, 0);
		DeviceUniform depthUniform = GetUniform(gameState->frameBufferProgram, "texDepth");
		UniformInt(depthUniform, 1);

		BindTexture(gameState->frameBufferColorTex, 0);
		BindTexture(gameState->frameBufferDepthTex, 1);
		RenderMesh(gameState->particleMesh);

		EnableDepthTest();

		// Gizmos
#if EDITOR_PRESENT
		{
			if (Transform *selectedEntity = GetEntityTransform(gameState, g_debugContext->selectedEntity))
			{
				ClearDepthBuffer();

				UseProgram(g_debugContext->editorGizmoProgram);
				DeviceUniform viewUniform = GetUniform(g_debugContext->editorGizmoProgram, "view");
				UniformMat4Array(viewUniform, 1, view->m);
				DeviceUniform projUniform = GetUniform(g_debugContext->editorGizmoProgram, "projection");
				UniformMat4Array(projUniform, 1, proj->m);
				DeviceUniform modelUniform = GetUniform(g_debugContext->editorGizmoProgram, "model");
				colorUniform = GetUniform(g_debugContext->editorGizmoProgram, "color");

				const Resource *arrowRes = GetResource("editor_arrow.b");
				const Resource *circleRes = GetResource("editor_circle.b");

				v3 pos = selectedEntity->translation;
				v4 rot = selectedEntity->rotation;

				f32 gizmoSize = 0.2f;
				f32 dist = V3Length(gameState->camPos - pos);
				gizmoSize *= dist;

				mat4 gizmoModel = Mat4Compose(pos, rot, gizmoSize);

				UniformMat4Array(modelUniform, 1, gizmoModel.m);
				UniformV4(colorUniform, { 0, 0, 1, 1 });
				RenderIndexedMesh(arrowRes->mesh.deviceMesh);
				RenderIndexedMesh(circleRes->mesh.deviceMesh);

				v4 xRot = QuaternionMultiply(rot, QuaternionFromEulerZYX({ 0, HALFPI, 0 }));
				gizmoModel = Mat4Compose(pos, xRot, gizmoSize);
				UniformMat4Array(modelUniform, 1, gizmoModel.m);
				UniformV4(colorUniform, { 1, 0, 0, 1 });
				RenderIndexedMesh(arrowRes->mesh.deviceMesh);
				RenderIndexedMesh(circleRes->mesh.deviceMesh);

				v4 yRot = QuaternionMultiply(rot, QuaternionFromEulerZYX({ -HALFPI, 0, 0 }));
				gizmoModel = Mat4Compose(pos, yRot, gizmoSize);
				UniformMat4Array(modelUniform, 1, gizmoModel.m);
				UniformV4(colorUniform, { 0, 1, 0, 1 });
				RenderIndexedMesh(arrowRes->mesh.deviceMesh);
				RenderIndexedMesh(circleRes->mesh.deviceMesh);
			}
		}
#endif
	}
}

void CleanupGame(GameState *gameState)
{
	(void) gameState;
	return;
}

@Ignore #include "TypeInfo.cpp"
