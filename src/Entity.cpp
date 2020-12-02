inline bool IsEntityHandleValid(GameState *gameState, EntityHandle handle)
{
	return handle != ENTITY_HANDLE_INVALID &&
		handle.generation == gameState->entityGenerations[handle.id];
}

Transform *GetEntityTransform(GameState *gameState, EntityHandle handle)
{
	if (!IsEntityHandleValid(gameState, handle))
		return nullptr;

	return gameState->entityTransforms[handle.id];
}

EntityHandle FindEntityHandle(GameState *gameState, Transform *transformPtr)
{
	for (u32 entityId = 0; entityId < gameState->transforms.size; ++entityId) // @Check: transforms.size???
	{
		Transform *currentPtr = gameState->entityTransforms[entityId];
		if (currentPtr == transformPtr)
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

	return gameState->entityMeshes[handle.id];
}

SkinnedMeshInstance *GetEntitySkinnedMesh(GameState *gameState, EntityHandle handle)
{
	if (!IsEntityHandleValid(gameState, handle))
		return nullptr;

	return gameState->entitySkinnedMeshes[handle.id];
}

ParticleSystem *GetEntityParticleSystem(GameState *gameState, EntityHandle handle)
{
	if (!IsEntityHandleValid(gameState, handle))
		return nullptr;

	return gameState->entityParticleSystems[handle.id];
}

Collider *GetEntityCollider(GameState *gameState, EntityHandle handle)
{
	if (!IsEntityHandleValid(gameState, handle))
		return nullptr;

	return gameState->entityColliders[handle.id];
}

void EntityAssignMesh(GameState *gameState, EntityHandle entityHandle,
		MeshInstance *meshInstance)
{
	meshInstance->entityHandle = entityHandle;
	gameState->entityMeshes[entityHandle.id] = meshInstance;
}

void EntityAssignSkinnedMesh(GameState *gameState, EntityHandle entityHandle,
		SkinnedMeshInstance *skinnedMeshInstance)
{
	skinnedMeshInstance->entityHandle = entityHandle;
	gameState->entitySkinnedMeshes[entityHandle.id] = skinnedMeshInstance;
}

void EntityAssignParticleSystem(GameState *gameState, EntityHandle entityHandle,
		ParticleSystem *particleSystem)
{
	particleSystem->entityHandle = entityHandle;
	gameState->entityParticleSystems[entityHandle.id] = particleSystem;
}

void EntityAssignCollider(GameState *gameState, EntityHandle entityHandle, Collider *collider)
{
	collider->entityHandle = entityHandle;
	gameState->entityColliders[entityHandle.id] = collider;
}

EntityHandle AddEntity(GameState *gameState, Transform **outTransform)
{
	Transform *newTransform = ArrayAdd_Transform(&gameState->transforms);
	*newTransform = {};
	newTransform->rotation = QUATERNION_IDENTITY;

	EntityHandle newHandle = ENTITY_HANDLE_INVALID;

	for (int entityId = 0; entityId < 256; ++entityId)
	{
		Transform *ptrInIdx = gameState->entityTransforms[entityId];
		if (!ptrInIdx)
		{
			// Assing vacant ID to new entity
			gameState->entityTransforms[entityId] = newTransform;

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

	Transform *transformPtr = gameState->entityTransforms[handle.id];
	// If entity is already deleted there's nothing to do
	if (!transformPtr)
		return;

	// Remove 'components'
	{
		Transform *last = &gameState->transforms[gameState->transforms.size - 1];
		// Retarget moved component's entity to the new pointer.
		EntityHandle handleOfLast = FindEntityHandle(gameState, last); // @Improve
		gameState->entityTransforms[handleOfLast.id] = transformPtr;
		*transformPtr = *last;

		gameState->entityTransforms[handle.id] = nullptr;
		--gameState->transforms.size;
	}

	MeshInstance *meshInstance = GetEntityMesh(gameState, handle);
	if (meshInstance)
	{
		MeshInstance *last =
			&gameState->meshInstances[--gameState->meshInstances.size];
		// Retarget moved component's entity to the new pointer.
		gameState->entityMeshes[last->entityHandle.id] = meshInstance;
		*meshInstance = *last;

		gameState->entityMeshes[handle.id] = nullptr;
	}

	SkinnedMeshInstance *skinnedMeshInstance = GetEntitySkinnedMesh(gameState, handle);
	if (skinnedMeshInstance)
	{
		SkinnedMeshInstance *last =
			&gameState->skinnedMeshInstances[--gameState->skinnedMeshInstances.size];
		// Retarget moved component's entity to the new pointer.
		gameState->entitySkinnedMeshes[last->entityHandle.id] = skinnedMeshInstance;
		*skinnedMeshInstance = *last;

		gameState->entitySkinnedMeshes[handle.id] = nullptr;
	}

	ParticleSystem *particleSystem = GetEntityParticleSystem(gameState, handle);
	if (particleSystem)
	{
		DestroyDeviceMesh(particleSystem->deviceBuffer);

		ParticleSystem *last = &gameState->particleSystems[--gameState->particleSystems.size];
		// Retarget moved component's entity to the new pointer.
		gameState->entityParticleSystems[last->entityHandle.id] = particleSystem;
		*particleSystem = *last;

		gameState->entityParticleSystems[handle.id] = nullptr;
	}

	Collider *collider = GetEntityCollider(gameState, handle);
	if (collider)
	{
		Collider *last = &gameState->colliders[--gameState->colliders.size];
		// Retarget moved component's entity to the new pointer.
		gameState->entityColliders[last->entityHandle.id] = collider;
		*collider = *last;

		gameState->entityColliders[handle.id] = nullptr;
	}
}
