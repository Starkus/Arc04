inline bool IsEntityHandleValid(GameState *gameState, EntityHandle handle)
{
	return handle != ENTITY_HANDLE_INVALID &&
		handle.generation == gameState->entityGenerations[handle.id];
}

Transform *GetEntityTransform(GameState *gameState, EntityHandle handle)
{
	if (!IsEntityHandleValid(gameState, handle))
		return nullptr;

	u32 idx = gameState->entityTransforms[handle.id];
	if (idx == ENTITY_ID_INVALID)
		return nullptr;

	return &gameState->transforms[idx];
}

EntityHandle EntityHandleFromIndex(GameState *gameState, u32 index)
{
	for (u32 entityId = 0; entityId < gameState->transforms.size; ++entityId)
	{
		u32 currentIdx = gameState->entityTransforms[entityId];
		if (currentIdx == index)
		{
			return { entityId, gameState->entityGenerations[entityId] };
		}
	}
	return ENTITY_HANDLE_INVALID;
}

MeshInstance *GetEntityMesh(GameState *gameState, EntityHandle handle)
{
	if (!IsEntityHandleValid(gameState, handle))
		return nullptr;

	u32 idx = gameState->entityMeshes[handle.id];
	if (idx == ENTITY_ID_INVALID)
		return nullptr;

	return &gameState->meshInstances[idx];
}

SkinnedMeshInstance *GetEntitySkinnedMesh(GameState *gameState, EntityHandle handle)
{
	if (!IsEntityHandleValid(gameState, handle))
		return nullptr;

	u32 idx = gameState->entitySkinnedMeshes[handle.id];
	if (idx == ENTITY_ID_INVALID)
		return nullptr;
	return &gameState->skinnedMeshInstances[idx];
}

ParticleSystem *GetEntityParticleSystem(GameState *gameState, EntityHandle handle)
{
	if (!IsEntityHandleValid(gameState, handle))
		return nullptr;

	u32 idx = gameState->entityParticleSystems[handle.id];
	if (idx == ENTITY_ID_INVALID)
		return nullptr;
	return &gameState->particleSystems[idx];
}

Collider *GetEntityCollider(GameState *gameState, EntityHandle handle)
{
	if (!IsEntityHandleValid(gameState, handle))
		return nullptr;

	u32 idx = gameState->entityColliders[handle.id];
	if (idx == ENTITY_ID_INVALID)
		return nullptr;
	return &gameState->colliders[idx];
}

void EntityAssignMesh(GameState *gameState, EntityHandle entityHandle,
		MeshInstance *meshInstance)
{
	meshInstance->entityHandle = entityHandle;
	u32 idx = (u32)ArrayPointerToIndex_MeshInstance(&gameState->meshInstances, meshInstance);
	gameState->entityMeshes[entityHandle.id] = idx;
}

void EntityAssignSkinnedMesh(GameState *gameState, EntityHandle entityHandle,
		SkinnedMeshInstance *skinnedMeshInstance)
{
	skinnedMeshInstance->entityHandle = entityHandle;
	u32 idx = (u32)ArrayPointerToIndex_SkinnedMeshInstance(&gameState->skinnedMeshInstances,
			skinnedMeshInstance);
	gameState->entitySkinnedMeshes[entityHandle.id] = idx;
}

void EntityAssignParticleSystem(GameState *gameState, EntityHandle entityHandle,
		ParticleSystem *particleSystem)
{
	particleSystem->entityHandle = entityHandle;
	u32 idx = (u32)ArrayPointerToIndex_ParticleSystem(&gameState->particleSystems, particleSystem);
	gameState->entityParticleSystems[entityHandle.id] = idx;
}

void EntityAssignCollider(GameState *gameState, EntityHandle entityHandle, Collider *collider)
{
	collider->entityHandle = entityHandle;
	u32 idx = (u32)ArrayPointerToIndex_Collider(&gameState->colliders, collider);
	gameState->entityColliders[entityHandle.id] = idx;
}

EntityHandle AddEntity(GameState *gameState, Transform **outTransform)
{
	u32 newTransformIdx = gameState->transforms.size;
	Transform *newTransform = ArrayAdd_Transform(&gameState->transforms);
	*newTransform = {};

	EntityHandle newHandle = ENTITY_HANDLE_INVALID;

	for (int entityId = 0; entityId < MAX_ENTITIES; ++entityId)
	{
		u32 indexInId = gameState->entityTransforms[entityId];
		if (indexInId == ENTITY_ID_INVALID)
		{
			// Assing vacant ID to new entity
			gameState->entityTransforms[entityId] = newTransformIdx;

			// Fill out entity handle
			newHandle.id = entityId;
			// Advance generation by one
			newHandle.generation = ++gameState->entityGenerations[entityId];

			break;
		}
	}

	*outTransform = newTransform;
	return newHandle;
}

// @Improve: delay actual deletion of entities til end of frame?
void RemoveEntity(GameState *gameState, EntityHandle handle)
{
	// Check handle is valid
	if (!IsEntityHandleValid(gameState, handle))
		return;

	u32 transformIdx = gameState->entityTransforms[handle.id];
	Transform *transformPtr = &gameState->transforms[transformIdx];
	// If entity is already deleted there's nothing to do
	if (!transformPtr)
		return;

	// Remove 'components'
	{
		u32 last = gameState->transforms.size - 1;
		Transform *lastPtr = &gameState->transforms[last];
		// Retarget moved component's entity to the new pointer.
		EntityHandle handleOfLast = EntityHandleFromIndex(gameState, last); // @Improve
		gameState->entityTransforms[handleOfLast.id] = transformIdx;
		*transformPtr = *lastPtr;

		gameState->entityTransforms[handle.id] = ENTITY_ID_INVALID;
		--gameState->transforms.size;
	}

	MeshInstance *meshInstance = GetEntityMesh(gameState, handle);
	if (meshInstance)
	{
		MeshInstance *last = &gameState->meshInstances[--gameState->meshInstances.size];
		// Retarget moved component's entity to the new pointer.
		u32 idx = (u32)ArrayPointerToIndex_MeshInstance(&gameState->meshInstances, meshInstance);
		gameState->entityMeshes[last->entityHandle.id] = idx;
		*meshInstance = *last;

		gameState->entityMeshes[handle.id] = ENTITY_ID_INVALID;
	}

	SkinnedMeshInstance *skinnedMeshInstance = GetEntitySkinnedMesh(gameState, handle);
	if (skinnedMeshInstance)
	{
		SkinnedMeshInstance *last =
			&gameState->skinnedMeshInstances[--gameState->skinnedMeshInstances.size];
		// Retarget moved component's entity to the new pointer.
		u32 idx = (u32)ArrayPointerToIndex_SkinnedMeshInstance(&gameState->skinnedMeshInstances,
				skinnedMeshInstance);
		gameState->entitySkinnedMeshes[last->entityHandle.id] = idx;
		*skinnedMeshInstance = *last;

		gameState->entitySkinnedMeshes[handle.id] = ENTITY_ID_INVALID;
	}

	ParticleSystem *particleSystem = GetEntityParticleSystem(gameState, handle);
	if (particleSystem)
	{
		// @Note: maybe allocate buffers for max supported amount of particle sources and just use
		// them?
		DestroyDeviceMesh(particleSystem->deviceBuffer);

		ParticleSystem *last = &gameState->particleSystems[--gameState->particleSystems.size];
		// Retarget moved component's entity to the new pointer.
		u32 idx = (u32)ArrayPointerToIndex_ParticleSystem(&gameState->particleSystems, particleSystem);
		gameState->entityParticleSystems[last->entityHandle.id] = idx;
		*particleSystem = *last;

		gameState->entityParticleSystems[handle.id] = ENTITY_ID_INVALID;
	}

	Collider *collider = GetEntityCollider(gameState, handle);
	if (collider)
	{
		Collider *last = &gameState->colliders[--gameState->colliders.size];
		// Retarget moved component's entity to the new pointer.
		u32 idx = (u32)ArrayPointerToIndex_Collider(&gameState->colliders, collider);
		gameState->entityColliders[last->entityHandle.id] = idx;
		*collider = *last;

		gameState->entityColliders[handle.id] = ENTITY_ID_INVALID;
	}
}
