const char *protectedMemoryErrorStr = "Can't read memory!";

bool StructMemberHasTag(const StructMember *memberInfo, const char *tag)
{
	for (u32 tagIdx = 0; tagIdx < memberInfo->tagCount; ++tagIdx)
	{
		if (strcmp(tag, memberInfo->tags[tagIdx]) == 0)
		{
			return true;
		}
	}
	return false;
}

void ImguiShowDebugWindow(GameState *gameState)
{
#if DEBUG_BUILD && USING_IMGUI
	ImGui::SetNextWindowPos(ImVec2(8, 340), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(315, 425), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Debug things", nullptr, 0))
	{
		ImGui::End();
		return;
	}

	if (ImGui::CollapsingHeader("Memory usage"))
	{
		static f32 frameMemSamples[32];
		static f32 transMemSamples[32];
		static f32 buddyMemSamples[32];
		static int memSamplesIdx = 0;
		frameMemSamples[memSamplesIdx] = (f32)(g_memory->lastFrameUsage);
		transMemSamples[memSamplesIdx] = (f32)((u8 *)g_memory->transientPtr -
				(u8 *)g_memory->transientMem);
		buddyMemSamples[memSamplesIdx] = (f32)(g_memory->buddyMemoryUsage);
		++memSamplesIdx;
		if (memSamplesIdx >= ArrayCount(frameMemSamples))
			memSamplesIdx = 0;

		ImGui::PlotHistogram("Frame alloc", frameMemSamples, ArrayCount(frameMemSamples), memSamplesIdx, "", 0,
				Memory::frameSize, ImVec2(0, 80.0f));

		ImGui::PlotHistogram("Transient alloc", transMemSamples, ArrayCount(transMemSamples), memSamplesIdx, "", 0,
				Memory::transientSize, ImVec2(0, 80.0f));

		ImGui::PlotHistogram("Buddy alloc", buddyMemSamples, ArrayCount(buddyMemSamples), memSamplesIdx, "", 0,
				Memory::buddySize, ImVec2(0, 80.0f));
	}

	ImGui::Checkbox("Debug draws in wireframe", &g_debugContext->wireframeDebugDraws);
	ImGui::SliderFloat("Time speed", &gameState->timeMultiplier, 0.001f, 10.0f, "factor = %.3f", ImGuiSliderFlags_Logarithmic);

	if (ImGui::CollapsingHeader("Collision debug"))
	{
		ImGui::Checkbox("Draw AABBs", &g_debugContext->drawAABBs);
		ImGui::Checkbox("Draw GJK support function results", &g_debugContext->drawSupports);
		ImGui::Checkbox("Enable verbose collision logging", &g_debugContext->verboseCollisionLogging);

		ImGui::Separator();

		ImGui::Text("GJK:");
		ImGui::Checkbox("Draw polytope", &g_debugContext->drawGJKPolytope);
		ImGui::SameLine();
		ImGui::Checkbox("Freeze", &g_debugContext->freezeGJKGeom);
		ImGui::InputInt("Step", &g_debugContext->gjkDrawStep);
		if (g_debugContext->gjkDrawStep >= g_debugContext->gjkStepCount)
			g_debugContext->gjkDrawStep = g_debugContext->gjkStepCount;

		ImGui::Separator();

		ImGui::Text("EPA:");
		ImGui::Checkbox("Draw polytope##", &g_debugContext->drawEPAPolytope);
		ImGui::SameLine();
		ImGui::Checkbox("Freeze##", &g_debugContext->freezePolytopeGeom);
		ImGui::InputInt("Step##", &g_debugContext->polytopeDrawStep);
		if (g_debugContext->polytopeDrawStep >= g_debugContext->epaStepCount)
			g_debugContext->polytopeDrawStep = g_debugContext->epaStepCount;
	}

	ImGui::End();
#else
	(void)gameState;
#endif
}

bool ImguiStructAsControls(GameState *gameState, void *object, const StructInfo *structInfo);

// Returns true if value changed
bool ImguiMemberAsControl(GameState *gameState, void *object, const StructMember *memberInfo)
{
	if (StructMemberHasTag(memberInfo, "Hidden"))
		return false;

	if (!PlatformCanReadMemory(object))
	{
		ImGui::Text(protectedMemoryErrorStr);
		return false;
	}

	// Special treatment for resource pointers
	if (memberInfo->type == TYPE_STRUCT && memberInfo->pointerLevels == 1 &&
			memberInfo->typeInfo == &typeInfo_Resource)
	{
		static char resInputName[128];
		const Resource **resourcePtr = (const Resource**)object;

		if (*resourcePtr)
		{
			if (PlatformCanReadMemory(*resourcePtr))
				strcpy(resInputName, (*resourcePtr)->filename);
			else
				strcpy(resInputName, protectedMemoryErrorStr);
		}
		else
		{
			resInputName[0] = 0;
		}

		if (ImGui::InputText(memberInfo->name, resInputName,
					ArrayCount(resInputName), ImGuiInputTextFlags_EnterReturnsTrue))
		{
			const Resource *res = GetResource(resInputName);
			if (!res)
				res = LoadResource((*resourcePtr)->type, resInputName);

			if (res)
				*resourcePtr = res;
			else
				*resourcePtr = nullptr;
			return true;
		}
		return false;
	}

	// Arrays
	if (memberInfo->arrayCount > 0)
	{
		bool somethingChanged = false;
		if (ImGui::TreeNode(memberInfo->name))
		{
			char enumeratedName[512];
			StructMember nonArrayType = *memberInfo;
			nonArrayType.arrayCount = 0;
			nonArrayType.name = enumeratedName;
			for (u32 i = 0; i < memberInfo->arrayCount; ++i)
			{
				sprintf(enumeratedName, "%s, %d", memberInfo->name, i);
				void *itemOffset = ((u8 *)object) + memberInfo->size / memberInfo->arrayCount * i;
				somethingChanged = ImguiMemberAsControl(gameState, itemOffset, &nonArrayType)
					|| somethingChanged;
			}
			ImGui::TreePop();
		}
		return somethingChanged;
	}

	if (memberInfo->pointerLevels)
	{
		bool somethingChanged = false;
		if (ImGui::TreeNode(memberInfo->name))
		{
			StructMember dereferencedType = *memberInfo;
			--dereferencedType.pointerLevels;

			void *ptr = *(void **)object;
			somethingChanged = ImguiMemberAsControl(gameState, ptr, &dereferencedType);

			ImGui::TreePop();
		}
		return somethingChanged;
	}

	static const f32 speed = 0.005f;
	static const u64 step = 1;

	switch (memberInfo->type)
	{
	case TYPE_U8:
		return ImGui::InputScalar(memberInfo->name, ImGuiDataType_U8, object, &step);
	case TYPE_U16:
		return ImGui::InputScalar(memberInfo->name, ImGuiDataType_U16, object, &step);
	case TYPE_U32:
		return ImGui::InputScalar(memberInfo->name, ImGuiDataType_U32, object, &step);
	case TYPE_U64:
		return ImGui::InputScalar(memberInfo->name, ImGuiDataType_U64, object, &step);
	case TYPE_I8:
		return ImGui::InputScalar(memberInfo->name, ImGuiDataType_S8, object, &step);
	case TYPE_I16:
		return ImGui::InputScalar(memberInfo->name, ImGuiDataType_S16, object, &step);
	case TYPE_I32:
		return ImGui::InputScalar(memberInfo->name, ImGuiDataType_S32, object, &step);
	case TYPE_I64:
		return ImGui::InputScalar(memberInfo->name, ImGuiDataType_S64, object, &step);
	case TYPE_F32:
		return ImGui::DragScalar(memberInfo->name, ImGuiDataType_Float, object, speed);
	case TYPE_F64:
		return ImGui::DragScalar(memberInfo->name, ImGuiDataType_Double, object, speed);
	case TYPE_BOOL:
		return ImGui::Checkbox(memberInfo->name, (bool *)object);
	case TYPE_STRUCT:
	{
		const StructInfo *structInfo = (StructInfo *)memberInfo->typeInfo;
		if (memberInfo->typeInfo == &typeInfo_v3)
		{
			if (StructMemberHasTag(memberInfo, "Color"))
			{
				return ImGui::ColorEdit3(memberInfo->name, (f32 *)object);
			}
			return ImGui::DragFloat3(memberInfo->name, (f32 *)object, speed);
		}
		else if (memberInfo->typeInfo == &typeInfo_v4)
		{
			if (StructMemberHasTag(memberInfo, "Color"))
			{
				return ImGui::ColorEdit4(memberInfo->name, (f32 *)object);
			}
			return ImGui::DragFloat4(memberInfo->name, (f32 *)object, speed);
		}
		else
		{
			//ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
			if (ImGui::TreeNode(memberInfo->name))
			{
				bool changed = ImguiStructAsControls(gameState, object, structInfo);
				ImGui::TreePop();
				return changed;
			}
			return false;
		}
	}
	case TYPE_ENUM:
	{
		bool changed = false;
		i64 value;
		switch (memberInfo->size)
		{
			case 1:  value = *(i8  *)object; break;
			case 2:  value = *(i16 *)object; break;
			case 4:  value = *(i32 *)object; break;
			default: value = *(i64 *)object; break;
		}
		const EnumInfo *enumInfo = (const EnumInfo *)memberInfo->typeInfo;

		u32 currentValueIdx = 0;
		for (u32 enumValueIdx = 0; enumValueIdx < enumInfo->valueCount; enumValueIdx++)
		{
			i64 v = enumInfo->values[enumValueIdx].value;
			if (v == value)
			{
				currentValueIdx = enumValueIdx;
				break;
			}
		}

		if (ImGui::BeginCombo(memberInfo->name, enumInfo->values[currentValueIdx].name))
		{
			for (u32 enumValueIdx = 0; enumValueIdx < enumInfo->valueCount; enumValueIdx++)
			{
				i64 v = enumInfo->values[enumValueIdx].value;
				const bool isSelected = (v == value);
				if (ImGui::Selectable(enumInfo->values[enumValueIdx].name, isSelected))
				{
					changed = value != v;
					value = v;
				}

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		return changed;
	}
	}

	return false;
}

bool ImguiStructAsControls(GameState *gameState, void *object, const StructInfo *structInfo)
{
	int step = 1;
	if (structInfo == &typeInfo_EntityHandle)
	{
		EntityHandle *entityHandle = (EntityHandle *)object;
		bool changed = ImGui::InputScalar("ID", ImGuiDataType_S32, &entityHandle->id, &step);
		if (changed)
		{
			entityHandle->generation = gameState->entityGenerations[entityHandle->id];
		}
		return changed;
	}
	else if (strncmp("Array_", structInfo->name, 6) == 0)
	{
		// @Improve: maybe use tags instead of strcmp'ing the struct name.
		void *dataPtrPtr = (u8 *)object + structInfo->members[0].offset;
		void *dataPtr = *(void **)(dataPtrPtr);
		void *sizePtr = (u8 *)object + structInfo->members[1].offset;

		bool somethingChanged = ImGui::InputScalar("Size", ImGuiDataType_U32, sizePtr, &step);

		// Items
		char enumeratedName[512];
		StructMember dataType = structInfo->members[0]; // @Hardcoded
		dataType.name = enumeratedName;
		--dataType.pointerLevels;

		const u32 itemCount = *(u32 *)sizePtr;

		u64 itemSize = dataType.size;
		if (dataType.pointerLevels > 0)
		{
			itemSize = sizeof(void *);
		}
		else if (dataType.type == TYPE_STRUCT)
		{
			itemSize = ((StructInfo *)dataType.typeInfo)->size;
		}

		for (u32 i = 0; i < itemCount; ++i)
		{
			sprintf(enumeratedName, "%s, %d", structInfo->name, i);
			void *itemOffset = ((u8 *)dataPtr) + itemSize * i;
			somethingChanged = ImguiMemberAsControl(gameState, itemOffset, &dataType)
				|| somethingChanged;
		}

		return somethingChanged;
	}
	else
	{
		bool somethingChanged = false;
		u8 *const o = (u8 *)object;
		for (u32 i = 0; i < structInfo->memberCount; ++i)
		{
			const StructMember *memberInfo = &structInfo->members[i];
			somethingChanged = ImguiMemberAsControl(gameState, o + memberInfo->offset, memberInfo) ||
				somethingChanged;
		}
		return somethingChanged;
	}
}

void ImguiShowGameStateWindow(GameState *gameState)
{
	if (!ImGui::Begin("Game state", nullptr, 0))
	{
		ImGui::End();
		return;
	}

	ImguiStructAsControls(gameState, gameState, &typeInfo_GameState);

	ImGui::End();
}

void ImguiShowEditWindow(GameState *gameState)
{
#if EDITOR_PRESENT
	ImGui::SetNextWindowPos(ImVec2(1161, 306), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(253, 229), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowCollapsed(false, ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("Edit", nullptr, 0))
	{
		ImGui::End();
		return;
	}

	ImguiStructAsControls(gameState, &g_debugContext->selectedEntity, &typeInfo_EntityHandle);

	if (ImGui::Button("Add"))
	{
		Transform *newTransform;
		g_debugContext->selectedEntity = AddEntity(gameState, &newTransform);
	}

	if (IsEntityHandleValid(gameState, g_debugContext->selectedEntity))
	{
		ImGui::SameLine();
		if (ImGui::Button("Delete"))
		{
			RemoveEntity(gameState, g_debugContext->selectedEntity);
			g_debugContext->selectedEntity = ENTITY_HANDLE_INVALID;
		}
	}

	if (!IsEntityHandleValid(gameState, g_debugContext->selectedEntity))
	{
		ImGui::End();
		return;
	}

	ImGui::Separator();

	EntityHandle selectedEntityHandle = g_debugContext->selectedEntity;
	Transform *transform = GetEntityTransform(gameState, selectedEntityHandle);
	if (!transform)
	{
		ImGui::Text("Invalid entity selected!");
		ImGui::End();
		return;
	}

	ImGui::Text("Transform");
	ImGui::DragFloat3("Position", transform->translation.v, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f");
	ImGui::DragFloat4("Rotation", transform->rotation.v, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f");
	if (ImGui::Button("Normalize rotation quaternion"))
		transform->rotation = V4Normalize(transform->rotation);

	ImGui::Separator();

	ImGui::Text("Mesh");
	MeshInstance *mesh = GetEntityMesh(gameState, selectedEntityHandle);
	if (mesh)
	{
		ImguiStructAsControls(gameState, mesh, &typeInfo_MeshInstance);
		if (ImGui::Button("Remove mesh"))
		{
			ArrayRemove_MeshInstance(&gameState->meshInstances, mesh);

			u32 idx = (u32)ArrayPointerToIndex_MeshInstance(&gameState->meshInstances, mesh);
			gameState->entityMeshes[mesh->entityHandle.id] = idx;
			gameState->entityMeshes[selectedEntityHandle.id] = ENTITY_ID_INVALID;
		}
	}
	else
	{
		if (ImGui::Button("Add mesh"))
		{
			mesh = ArrayAdd_MeshInstance(&gameState->meshInstances);
			*mesh = {};
			EntityAssignMesh(gameState, selectedEntityHandle, mesh);
		}
	}

	ImGui::Separator();

	ImGui::Text("Skinned mesh");
	SkinnedMeshInstance *skinnedMesh = GetEntitySkinnedMesh(gameState, selectedEntityHandle);
	if (skinnedMesh)
	{
		ImguiStructAsControls(gameState, skinnedMesh, &typeInfo_SkinnedMeshInstance);
		if (ImGui::Button("Remove skinned mesh"))
		{
			ArrayRemove_SkinnedMeshInstance(&gameState->skinnedMeshInstances, skinnedMesh);

			u32 idx = (u32)ArrayPointerToIndex_SkinnedMeshInstance(&gameState->skinnedMeshInstances,
					skinnedMesh);
			gameState->entitySkinnedMeshes[mesh->entityHandle.id] = idx;
			gameState->entitySkinnedMeshes[selectedEntityHandle.id] = ENTITY_ID_INVALID;
		}
	}
	else
	{
		if (ImGui::Button("Add skinned mesh"))
		{
			skinnedMesh = ArrayAdd_SkinnedMeshInstance(&gameState->skinnedMeshInstances);
			*skinnedMesh = {};
			EntityAssignSkinnedMesh(gameState, selectedEntityHandle, skinnedMesh);
		}
	}

	ImGui::Separator();

	ImGui::Text("Collider");
	Collider *collider = GetEntityCollider(gameState, selectedEntityHandle);
	if (collider)
	{
		bool changedType = ImguiMemberAsControl(gameState, &collider->type,
				&typeInfo_Collider.members[0]); // @Hardcoded

		if (changedType && collider->type == COLLIDER_CONVEX_HULL)
		{
			// Set pointer to null, it had garbage anyways
			collider->convexHull.meshRes = nullptr;
		}

		const StructMember *member = &typeInfo_Collider.members[collider->type + 1]; // @Hardcoded
		ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
		ImguiMemberAsControl(gameState, ((u8 *)collider) + member->offset, member);

		if (ImGui::Button("Remove collider"))
		{
			ArrayRemove_Collider(&gameState->colliders, collider);

			u32 idx = (u32)ArrayPointerToIndex_Collider(&gameState->colliders, collider);
			gameState->entityColliders[mesh->entityHandle.id] = idx;
			gameState->entityColliders[selectedEntityHandle.id] = ENTITY_ID_INVALID;
		}
	}
	else
	{
		if (ImGui::Button("Add collider"))
		{
			collider = ArrayAdd_Collider(&gameState->colliders);
			*collider = {};
			EntityAssignCollider(gameState, selectedEntityHandle, collider);
		}
	}

	ImGui::Separator();

	ImGui::Text("Particle system");
	ParticleSystem *particleSystem = GetEntityParticleSystem(gameState, selectedEntityHandle);
	if (particleSystem)
	{
		ImguiStructAsControls(gameState, particleSystem, &typeInfo_ParticleSystem);
		if (ImGui::Button("Reset timer"))
		{
			particleSystem->timer = 0;
		}

		if (ImGui::Button("Remove particle system"))
		{
			DestroyDeviceMesh(particleSystem->deviceBuffer);

			ArrayRemove_ParticleSystem(&gameState->particleSystems, particleSystem);

			u32 idx = (u32)ArrayPointerToIndex_ParticleSystem(&gameState->particleSystems,
					particleSystem);
			gameState->entityParticleSystems[mesh->entityHandle.id] = idx;
			gameState->entityParticleSystems[selectedEntityHandle.id] = ENTITY_ID_INVALID;
		}
	}
	else
	{
		if (ImGui::Button("Add particle system"))
		{
			particleSystem = ArrayAdd_ParticleSystem(&gameState->particleSystems);
			*particleSystem = {};
			particleSystem->deviceBuffer = CreateDeviceMesh(0);
			memset(particleSystem->alive, 0, sizeof(particleSystem->alive));
			EntityAssignParticleSystem(gameState, selectedEntityHandle, particleSystem);
		}
	}

	ImGui::End();

#else
	(void)gameState;
#endif
}

void ImguiShowSceneWindow(GameState *gameState)
{
	if (!ImGui::Begin("Scene"))
	{
		ImGui::End();
		return;
	}

	if (ImGui::Button("Load"))
	{
		// Delete all entities
		u32 count = gameState->transforms.size;
		for (u32 entityId = 0; entityId < count; )
		{
			//EntityHandle handle = { entityId, gameState->entityGenerations[entityId] };
			EntityHandle handle = EntityHandleFromTransformIndex(gameState, entityId); // @Improve
			if (IsEntityHandleValid(gameState, handle))
			{
				RemoveEntity(gameState, handle);
			}
			else
			{
				++entityId;
			}
		}

		u8 *fileBuffer;
		u64 fileSize;
		PlatformReadEntireFile("data/scene.scene", &fileBuffer, &fileSize, FrameAlloc);

		DeserializeEntities(gameState, fileBuffer, fileSize);
	}

	ImGui::SameLine();
	if (ImGui::Button("Save"))
	{
		FileHandle file = PlatformOpenForWrite("data/scene.scene");

		const u64 bufferSize = 8192;
		StringStream stream;
		stream.buffer = (char *)FrameAlloc(bufferSize);
		stream.capacity = bufferSize;
		stream.cursor = 0;

		SerializeEntities(gameState, &stream);

		PlatformWriteToFile(file, stream.buffer, stream.cursor);

		PlatformCloseFile(file);
	}

	ImGui::End();
}
