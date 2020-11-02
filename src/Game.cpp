#include <stddef.h>
#include <memory.h>

#include "General.h"
#include "Maths.h"
#include "MemoryAlloc.h"
#include "Render.h"
#include "Geometry.h"
#include "Primitives.h"
#include "Platform.h"
#include "Resource.h"
#include "PlatformCode.h"
#include "Containers.h"
#include "Game.h"

Memory *g_memory;
Log_t *g_log;

#define LOG(...) g_log(__VA_ARGS__)

DECLARE_ARRAY(u32);

#include "DebugDraw.cpp"
#include "MemoryAlloc.cpp"
#include "Collision.cpp"
#include "BakeryInterop.cpp"

#if TARGET_WINDOWS
#define GAMEDLL NOMANGLE __declspec(dllexport)
#else
#define GAMEDLL NOMANGLE __attribute__((visibility("default")))
#endif

GAMEDLL START_GAME(StartGame)
{
	g_memory = memory;
	g_log = platformCode->Log;

	LOG("Starting game!\n");

	GameState *gameState = (GameState *)memory->transientMem;

	// Initialize
	{
		platformCode->SetUpDevice();

		platformCode->ResourceLoadMesh("data/anvil.b");
		platformCode->ResourceLoadMesh("data/cube.b");
		platformCode->ResourceLoadMesh("data/sphere.b");
		platformCode->ResourceLoadMesh("data/cylinder.b");
		platformCode->ResourceLoadMesh("data/capsule.b");
		platformCode->ResourceLoadMesh("data/level_graphics.b");
		platformCode->ResourceLoadSkinnedMesh("data/Sparkus.b");
		platformCode->ResourceLoadLevelGeometryGrid("data/level.b");
		platformCode->ResourceLoadPoints("data/anvil_collision.b");

#if DEBUG_BUILD
		// Debug geometry buffer
		{
			gameState->debugGeometryBuffer.triangleData = (Vertex *)TransientAlloc(2048 * sizeof(Vertex));
			gameState->debugGeometryBuffer.lineData = (Vertex *)TransientAlloc(2048 * sizeof(Vertex));
			gameState->debugGeometryBuffer.triangleVertexCount = 0;
			gameState->debugGeometryBuffer.lineVertexCount = 0;
			gameState->debugGeometryBuffer.deviceMesh = platformCode->CreateDeviceMesh();
		}
#endif

		// Shaders
		const Resource *shaderRes = platformCode->ResourceLoadShader("data/shaders/shader_general.b");
		gameState->program = shaderRes->shader.programHandle;

		const Resource *shaderSkinnedRes = platformCode->ResourceLoadShader("data/shaders/shader_skinned.b");
		gameState->skinnedMeshProgram = shaderSkinnedRes->shader.programHandle;

#if DEBUG_BUILD
		const Resource *shaderDebugRes = platformCode->ResourceLoadShader("data/shaders/shader_debug.b");
		gameState->debugDrawProgram = shaderDebugRes->shader.programHandle;
#endif
	}

	// Init level
	{
		const Resource *levelGraphicsRes = platformCode->GetResource("data/level_graphics.b");
		gameState->levelGeometry.renderMesh = levelGraphicsRes;

		const Resource *levelCollisionRes = platformCode->GetResource("data/level.b");
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
		gameState->animationIdx = PLAYERANIM_IDLE;
	}

	// Init camera
	gameState->camPitch = -PI * 0.35f;

	// Test entities
	{
		Collider collider;
		collider.type = COLLIDER_CONVEX_HULL;

		const Resource *pointsRes = platformCode->GetResource("data/anvil_collision.b");
		collider.convexHull.pointCloud = pointsRes;

		const Resource *anvilRes = platformCode->GetResource("data/anvil.b");

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

		const Resource *sphereRes = platformCode->GetResource("data/sphere.b");
		testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { -6.0f, 7.0f, 1.0f };
		testEntity->fw = { 0.0f, 1.0f, 0.0f };
		testEntity->mesh = sphereRes;
		testEntity->collider.type = COLLIDER_SPHERE;
		testEntity->collider.sphere.radius = 1;
		testEntity->collider.sphere.offset = {};

		const Resource *cylinderRes = platformCode->GetResource("data/cylinder.b");
		testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { -3.0f, 7.0f, 1.0f };
		testEntity->fw = { 0.0f, 1.0f, 0.0f };
		testEntity->mesh = cylinderRes;
		testEntity->collider.type = COLLIDER_CYLINDER;
		testEntity->collider.cylinder.radius = 1;
		testEntity->collider.cylinder.height = 2;
		testEntity->collider.cylinder.offset = {};

		const Resource *capsuleRes = platformCode->GetResource("data/capsule.b");
		testEntity = &gameState->entities[gameState->entityCount++];
		testEntity->pos = { 0.0f, 7.0f, 2.0f };
		testEntity->fw = { 0.0f, 1.0f, 0.0f };
		testEntity->mesh = capsuleRes;
		testEntity->collider.type = COLLIDER_CAPSULE;
		testEntity->collider.capsule.radius = 1;
		testEntity->collider.capsule.height = 2;
		testEntity->collider.capsule.offset = {};
	}
}

void ChangeState(GameState *gameState, PlayerState newState, PlayerAnim newAnim)
{
	LOG("Changed state: 0x%X -> 0x%X\n", gameState->player.state, newState);
	gameState->animationIdx = newAnim;
	gameState->animationTime = 0;
	gameState->player.state = newState;
}

GAMEDLL UPDATE_AND_RENDER_GAME(UpdateAndRenderGame)
{
	g_memory = memory;
	g_log = platformCode->Log;
	GameState *gameState = (GameState *)memory->transientMem;

	if (deltaTime < 0 || deltaTime > 1)
	{
		LOG("Delta time out of range! %f\n", deltaTime);
		deltaTime = 1 / 60.0f;
	}

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
		DrawDebugCubeAA(v3{}, 0.05f); // Draw origin
#endif

#if DEBUG_BUILD
		const ResourceSkinnedMesh *skinnedMesh = &platformCode->GetResource("data/Sparkus.b")->skinnedMesh;
		if (controller->debugUp.endedDown && controller->debugUp.changed)
		{
			gameState->animationTime = 0;
			++gameState->animationIdx;
			if (gameState->animationIdx >= (i32)skinnedMesh->animationCount)
				gameState->animationIdx = 0;
		}
		if (controller->debugDown.endedDown && controller->debugDown.changed)
		{
			gameState->animationTime = 0;
			--gameState->animationIdx;
			if (gameState->animationIdx < 0)
				gameState->animationIdx = skinnedMesh->animationCount - 1;
		}
#endif

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

			const f32 deadzone = 0.17f;
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
				const f32 deadzone = 0.17f;
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
				player->vel.z = -2.0f;
			}
			player->entity->pos.z += player->vel.z * deltaTime;
		}

		// Collision
		bool touchedGround = false;

		for (u32 entityIndex = 0; entityIndex < gameState->entityCount; ++ entityIndex)
		{
			Entity *entity = &gameState->entities[entityIndex];
			if (entity != gameState->player.entity)
			{
				GJKResult gjkResult = GJKTest(gameState, player->entity, entity, platformCode);
				if (gjkResult.hit)
				{
					v3 depenetration = ComputeDepenetration(gjkResult, player->entity, entity,
							platformCode);
					// @Hack: ignoring depenetration when it's too big
					if (V3Length(depenetration) < 2.0f)
					{
						player->entity->pos += depenetration;
						// @Fix: handle velocity change upon hits properly
						if (V3Normalize(depenetration).z > 0.9f)
							touchedGround = true;
					}
					else
					{
						LOG("WARNING! Ignoring huge depenetration vector... something went wrong!\n");
					}
					break;
				}
			}
		}

		// Ray testing
		{
			v3 origin = player->entity->pos + v3{ 0, 0, 1 };
			v3 dir = { 0, 0, -1.1f };
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

#if GJK_VISUAL_DEBUGGING
		g_debugGeometryBuffer.vertexCount = 0;
		Vertex *gjkVertices;
		u32 gjkFaceCount;
		GetGJKStepGeometry(g_currentPolytopeStep, &gjkVertices, &gjkFaceCount);
		DrawDebugTriangles(gjkVertices, gjkFaceCount);

		DrawDebugCubeAA(g_GJKNewPoint[g_currentPolytopeStep], 0.03f);
#endif
#if EPA_VISUAL_DEBUGGING
		Vertex *epaVertices;
		u32 epaFaceCount;
		GetEPAStepGeometry(g_currentPolytopeStep, &epaVertices, &epaFaceCount);
		DrawDebugTriangles(epaVertices, epaFaceCount * 3);

		DrawDebugCubeAA(g_epaNewPoint[g_currentPolytopeStep], 0.03f);
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

		platformCode->UseProgram(gameState->program);
		DeviceUniform viewUniform = platformCode->GetUniform(gameState->program, "view");
		platformCode->UniformMat4(viewUniform, 1, view.m);
		DeviceUniform projUniform = platformCode->GetUniform(gameState->program, "projection");
		platformCode->UniformMat4(projUniform, 1, proj.m);

		DeviceUniform modelUniform = platformCode->GetUniform(gameState->program, "model");

		const v4 clearColor = { 0.95f, 0.88f, 0.05f, 1.0f };
		platformCode->ClearBuffers(clearColor);

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
			platformCode->UniformMat4(modelUniform, 1, model.m);

			platformCode->RenderIndexedMesh(entity->mesh->mesh.deviceMesh);
		}

		// Level
		{
			LevelGeometry *level = &gameState->levelGeometry;

			platformCode->UniformMat4(modelUniform, 1, MAT4_IDENTITY.m);
			platformCode->RenderIndexedMesh(level->renderMesh->mesh.deviceMesh);
		}

		// Skinned meshes
		platformCode->UseProgram(gameState->skinnedMeshProgram);
		viewUniform = platformCode->GetUniform(gameState->skinnedMeshProgram, "view");
		modelUniform = platformCode->GetUniform(gameState->skinnedMeshProgram, "model");
		projUniform = platformCode->GetUniform(gameState->skinnedMeshProgram, "projection");
		platformCode->UniformMat4(projUniform, 1, proj.m);

		DeviceUniform jointsUniform = platformCode->GetUniform(gameState->skinnedMeshProgram, "joints");
		platformCode->UniformMat4(viewUniform, 1, view.m);
		{
			platformCode->UniformMat4(modelUniform, 1, MAT4_IDENTITY.m);

			mat4 joints[128];
			for (int i = 0; i < 128; ++i)
			{
				joints[i] = MAT4_IDENTITY;
			}

			const Resource *skinnedMeshRes = platformCode->GetResource("data/Sparkus.b");
			const ResourceSkinnedMesh *skinnedMesh = &skinnedMeshRes->skinnedMesh;

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
				if (animation->loop)
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
			platformCode->UniformMat4(modelUniform, 1, model.m);

			platformCode->UniformMat4(jointsUniform, 128, joints[0].m);

			platformCode->RenderIndexedMesh(skinnedMesh->deviceMesh);
		}

#if DEBUG_BUILD
		platformCode->UseProgram(gameState->debugDrawProgram);
		viewUniform = platformCode->GetUniform(gameState->debugDrawProgram, "view");
		modelUniform = platformCode->GetUniform(gameState->debugDrawProgram, "model");
		projUniform = platformCode->GetUniform(gameState->debugDrawProgram, "projection");
		platformCode->UniformMat4(projUniform, 1, proj.m);

		platformCode->UniformMat4(viewUniform, 1, view.m);
		// Debug draws
		{
			for (u32 i = 0; i < gameState->debugGeometryBuffer.debugCubeCount; ++i)
			{
				DebugCube *cube = &gameState->debugGeometryBuffer.debugCubes[i];

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
				platformCode->UniformMat4(modelUniform, 1, model.m);
				platformCode->RenderIndexedMesh(gameState->cubeMesh);
			}
		}

		// Debug meshes
		{
			platformCode->SetFillMode(RENDER_LINE);

			platformCode->UniformMat4(modelUniform, 1, MAT4_IDENTITY.m);

			platformCode->SendMesh(&gameState->debugGeometryBuffer.deviceMesh,
					gameState->debugGeometryBuffer.triangleData,
					gameState->debugGeometryBuffer.triangleVertexCount, true);

			platformCode->RenderMesh(gameState->debugGeometryBuffer.deviceMesh);

			platformCode->SendMesh(&gameState->debugGeometryBuffer.deviceMesh,
					gameState->debugGeometryBuffer.lineData,
					gameState->debugGeometryBuffer.lineVertexCount, true);
			platformCode->RenderLines(gameState->debugGeometryBuffer.deviceMesh);

			gameState->debugGeometryBuffer.debugCubeCount = 0;
			gameState->debugGeometryBuffer.triangleVertexCount = 0;
			gameState->debugGeometryBuffer.lineVertexCount = 0;

			platformCode->SetFillMode(RENDER_FILL);
		}
#endif
	}
}

void CleanupGame(GameState *gameState)
{
	(void) gameState;
	return;
}
