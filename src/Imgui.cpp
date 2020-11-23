const char *protectedMemoryErrorStr = "Can't read memory!";

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

bool ImguiStructAsControls(void *object, const StructInfo *structInfo);

// Returns true if value changed
bool ImguiMemberAsControl(void *object, const StructMember *memberInfo)
{
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
			if (res)
				*resourcePtr = res;
			else
				*resourcePtr = nullptr;
			return true;
		}
		return false;
	}

	if (memberInfo->pointerLevels)
	{
		bool somethingChanged = false;
		if (ImGui::TreeNode(memberInfo->name))
		{
			StructMember dereferencedType = *memberInfo;
			--dereferencedType.pointerLevels;

			void *ptr = *(void **)object;
			somethingChanged = ImguiMemberAsControl(ptr, &dereferencedType);

			ImGui::TreePop();
		}
		return somethingChanged;
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
				somethingChanged = ImguiMemberAsControl(itemOffset, &nonArrayType)
					|| somethingChanged;
			}
			ImGui::TreePop();
		}
		return somethingChanged;
	}

	const f32 speed = 0.005f;

	switch (memberInfo->type)
	{
	case TYPE_U8:
		return ImGui::DragScalar(memberInfo->name, ImGuiDataType_U8, object, speed);
	case TYPE_U16:
		return ImGui::DragScalar(memberInfo->name, ImGuiDataType_U16, object, speed);
	case TYPE_U32:
		return ImGui::DragScalar(memberInfo->name, ImGuiDataType_U32, object, speed);
	case TYPE_U64:
		return ImGui::DragScalar(memberInfo->name, ImGuiDataType_U64, object, speed);
	case TYPE_I8:
		return ImGui::DragScalar(memberInfo->name, ImGuiDataType_S8, object, speed);
	case TYPE_I16:
		return ImGui::DragScalar(memberInfo->name, ImGuiDataType_S16, object, speed);
	case TYPE_I32:
		return ImGui::DragScalar(memberInfo->name, ImGuiDataType_S32, object, speed);
	case TYPE_I64:
		return ImGui::DragScalar(memberInfo->name, ImGuiDataType_S64, object, speed);
	case TYPE_F32:
		return ImGui::DragScalar(memberInfo->name, ImGuiDataType_Float, object, speed);
	case TYPE_F64:
		return ImGui::DragScalar(memberInfo->name, ImGuiDataType_Double, object, speed);
	case TYPE_BOOL:
		return ImGui::Checkbox(memberInfo->name, (bool *)object);
	case TYPE_STRUCT:
	{
		if (memberInfo->typeInfo == &typeInfo_v3)
		{
			return ImGui::DragFloat3(memberInfo->name, (f32 *)object, speed);
		}
		else if (memberInfo->typeInfo == &typeInfo_v4)
		{
			return ImGui::DragFloat4(memberInfo->name, (f32 *)object, speed);
		}
		else
		{
			//ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
			if (ImGui::TreeNode(memberInfo->name))
			{
				bool changed = ImguiStructAsControls(object, (const StructInfo *)memberInfo->typeInfo);
				ImGui::TreePop();
				return changed;
			}
			return false;
		}
	}
	case TYPE_ENUM:
	{
		bool changed = false;
		i64 *value = (i64 *)object;
		const EnumInfo *enumInfo = (const EnumInfo *)memberInfo->typeInfo;

		u32 currentValueIdx = 0;
		for (u32 enumValueIdx = 0; enumValueIdx < enumInfo->valueCount; enumValueIdx++)
		{
			i64 v = enumInfo->values[enumValueIdx].value;
			if (v == *value)
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
				const bool isSelected = (v == *value);
				if (ImGui::Selectable(enumInfo->values[enumValueIdx].name, isSelected))
				{
					changed = *value != v;
					*value = v;
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

bool ImguiStructAsControls(void *object, const StructInfo *structInfo)
{
	bool somethingChanged = false;
	u8 *const o = (u8 *)object;
	for (u32 i = 0; i < structInfo->memberCount; ++i)
	{
		const StructMember *memberInfo = &structInfo->members[i];
		somethingChanged = ImguiMemberAsControl(o + memberInfo->offset, memberInfo) ||
			somethingChanged;
	}
	return somethingChanged;
}

void ImguiShowGameStateWindow(GameState *gameState)
{
	if (!ImGui::Begin("Game state", nullptr, 0))
	{
		ImGui::End();
		return;
	}

	ImguiStructAsControls(gameState, &typeInfo_GameState);

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

	ImGui::InputInt("Entity", &g_debugContext->selectedEntityIdx);
	if (g_debugContext->selectedEntityIdx < 0)
		g_debugContext->selectedEntityIdx = 0;
	else if (g_debugContext->selectedEntityIdx >= (int)gameState->entityCount)
		g_debugContext->selectedEntityIdx = gameState->entityCount - 1;
	Entity *selectedEntity = &gameState->entities[g_debugContext->selectedEntityIdx];

	if (ImGui::Button("Add"))
	{
		Entity *newEntity;
		AddEntity(gameState, &newEntity);
		g_debugContext->selectedEntityIdx = gameState->entityCount - 1;
	}

	ImGui::SameLine();
	if (ImGui::Button("Delete"))
	{
		for (u32 id = 0; id < 256; ++id)
		{
			if (gameState->entityPointers[id] == selectedEntity)
			{
				EntityHandle handle = { id, gameState->entityGenerations[id] };
				RemoveEntity(gameState, handle);
				break;
			}
		}
	}

	ImGui::Separator();

	ImGui::Text("Transform");
	ImGui::DragFloat3("Position", selectedEntity->pos.v, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f");
	ImGui::DragFloat4("Rotation", selectedEntity->rot.v, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f");
	if (ImGui::Button("Normalize rotation quaternion"))
		selectedEntity->rot = V4Normalize(selectedEntity->rot);

	ImGui::Separator();

	ImGui::Text("Mesh");
	static char meshResInputName[128] = "";
	if (ImGui::InputText("Mesh resource", meshResInputName, ArrayCount(meshResInputName), ImGuiInputTextFlags_EnterReturnsTrue) |
		ImGui::Button("Change mesh"))
	{
		const Resource *res = GetResource(meshResInputName);
		selectedEntity->mesh = res;
	}

	ImGui::Separator();

	ImGui::Text("Skinned mesh");
	SkinnedMeshInstance *skinnedMesh = selectedEntity->skinnedMeshInstance;
	if (skinnedMesh)
	{
		ImguiStructAsControls(skinnedMesh, &typeInfo_SkinnedMeshInstance);
		if (ImGui::Button("Remove skinned mesh"))
		{
			SkinnedMeshInstance *last = &gameState->skinnedMeshInstances[--gameState->skinnedMeshCount];
			Entity *entityOfLast = GetEntity(gameState, last->entityHandle);

			*skinnedMesh = *last;
			entityOfLast->skinnedMeshInstance = skinnedMesh;

			selectedEntity->skinnedMeshInstance = nullptr;
		}
	}
	else
	{
		if (ImGui::Button("Add skinned mesh"))
		{
			skinnedMesh = &gameState->skinnedMeshInstances[gameState->skinnedMeshCount++];
			*skinnedMesh = {};
			skinnedMesh->entityHandle = FindEntityHandle(gameState, selectedEntity);
			selectedEntity->skinnedMeshInstance = skinnedMesh;
		}
	}

	ImGui::Separator();

	ImGui::Text("Collider");
	{
		Collider *collider = &selectedEntity->collider;
		bool changedType = ImguiMemberAsControl(&collider->type,
				&typeInfo_Collider.members[0]); // @Hardcoded

		if (changedType && collider->type == COLLIDER_CONVEX_HULL)
		{
			// Set pointer to null, it had garbage anyways
			collider->convexHull.meshRes = nullptr;
		}

		const StructMember *member = &typeInfo_Collider.members[collider->type + 1];
		ImguiMemberAsControl(((u8 *)&selectedEntity->collider) + member->offset, member);
	}

	ImGui::Separator();

	ImGui::Text("Particle system");
	ParticleSystem *particleSystem = selectedEntity->particleSystem;
	if (particleSystem)
	{
		ImguiStructAsControls(particleSystem, &typeInfo_ParticleSystem);
		if (ImGui::Button("Remove particle system"))
		{
			DestroyDeviceMesh(particleSystem->deviceBuffer);

			ParticleSystem *last = &gameState->particleSystems[--gameState->particleSystemCount];
			Entity *entityOfLast = GetEntity(gameState, last->entityHandle);
			ASSERT(entityOfLast);

			*particleSystem = *last;
			entityOfLast->particleSystem = particleSystem;

			selectedEntity->particleSystem = nullptr;
		}
	}
	else
	{
		if (ImGui::Button("Add particle system"))
		{
			ParticleSystem *newParticleSystem = &gameState->particleSystems[gameState->particleSystemCount++];
			*newParticleSystem = {};
			newParticleSystem->entityHandle = FindEntityHandle(gameState, selectedEntity);
			newParticleSystem->deviceBuffer = CreateDeviceMesh(0);
			memset(newParticleSystem->alive, 0, sizeof(newParticleSystem->alive));
			selectedEntity->particleSystem = newParticleSystem;
		}
	}

	ImGui::End();
#else
	(void)gameState;
#endif
}

