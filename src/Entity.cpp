Entity *GetEntity(GameState *gameState, EntityHandle handle)
{
	if (handle == ENTITY_HANDLE_INVALID)
		return nullptr;

	// Check handle is valid
	if (handle.generation != gameState->entityGenerations[handle.id])
		return nullptr;

	return gameState->entityPointers[handle.id];
}

EntityHandle FindEntityHandle(GameState *gameState, Entity *entityPtr)
{
	for (u32 entityId = 0; entityId < gameState->entities.size; ++entityId)
	{
		Entity *currentPtr = gameState->entityPointers[entityId];
		if (currentPtr == entityPtr)
		{
			return { entityId, gameState->entityGenerations[entityId] };
		}
	}
	return ENTITY_HANDLE_INVALID;
}

Collider *GetEntityCollider(GameState *gameState, EntityHandle handle)
{
	if (handle == ENTITY_HANDLE_INVALID)
		return nullptr;

	// Check handle is valid
	if (handle.generation != gameState->entityGenerations[handle.id])
		return nullptr;

	return gameState->entityColliders[handle.id];
}

void EntityAssignCollider(GameState *gameState, EntityHandle entityHandle, Collider *collider)
{
	collider->entityHandle = entityHandle;
	gameState->entityColliders[entityHandle.id] = collider;
}

EntityHandle AddEntity(GameState *gameState, Entity **outEntity)
{
	Entity *newEntity = ArrayAdd_Entity(&gameState->entities);
	*newEntity = {};
	newEntity->rotation = QUATERNION_IDENTITY;

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
		SkinnedMeshInstance *last =
			&gameState->skinnedMeshInstances[--gameState->skinnedMeshInstances.size];
		Entity *lastsEntity = GetEntity(gameState, last->entityHandle);
		if (lastsEntity)
		{
			// Fix entity that was pointing to skinnedMeshInstance that we moved.
			lastsEntity->skinnedMeshInstance = skinnedMeshInstance;
		}
		*skinnedMeshInstance = *last;
	}

	ParticleSystem *particleSystem = entityPtr->particleSystem;
	if (particleSystem)
	{
		ParticleSystem *last =
			&gameState->particleSystems[--gameState->particleSystems.size];
		Entity *lastsEntity = GetEntity(gameState, last->entityHandle);
		if (lastsEntity)
		{
			// Fix entity that was pointing to skinnedMeshInstance that we moved.
			lastsEntity->particleSystem = particleSystem;
		}
		*particleSystem = *last;
	}

	Collider *collider = GetEntityCollider(gameState, handle);
	if (collider)
	{
		Collider *last = &gameState->colliders[--gameState->colliders.size];
		// Retarget moved collider's entity to the new collider pointer.
		gameState->entityColliders[last->entityHandle.id] = collider;
		*collider = *last;

		gameState->entityColliders[handle.id] = nullptr;
	}

	// Erase from entity array by swapping with last
	Entity *ptrOfLast = &gameState->entities[gameState->entities.size - 1];
	Log("Moving entity at 0x%p to 0x%p\n", ptrOfLast, entityPtr);
	*entityPtr = *ptrOfLast;

	// Find and fix handle to entity we moved
	EntityHandle movedEntity = FindEntityHandle(gameState, ptrOfLast);
	gameState->entityPointers[movedEntity.id] = entityPtr;

	--gameState->entities.size;

	// Free pointer slot
	gameState->entityPointers[handle.id] = nullptr;
}
