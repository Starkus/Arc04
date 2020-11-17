void ImguiShowDebugWindow(GameState *gameState)
{
#if DEBUG_BUILD
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

void ImguiShowEditWindow(GameState *gameState)
{
#if USING_IMGUI
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

	ImGui::Separator();

	ImGui::DragFloat3("Position", selectedEntity->pos.v, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f");
	ImGui::DragFloat4("Rotation", selectedEntity->rot.v, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f");
	selectedEntity->rot = V4Normalize(selectedEntity->rot);

	ImGui::Separator();

	static char meshResInputName[128] = "";
	ImGui::InputText("Mesh resource", meshResInputName, IM_ARRAYSIZE(meshResInputName));
	//if (ImGui::Button("Apply"))
	{
		const Resource *res = GetResource(meshResInputName);
		if (res)
		{
			selectedEntity->mesh = res;
		}
	}

	ImGui::Separator();

	{
		const char* items[] = { "COLLIDER_CONVEX_HULL", "COLLIDER_SPHERE", "COLLIDER_CYLINDER", "COLLIDER_CAPSULE" };
		int itemCurrentIdx = (int)selectedEntity->collider.type;
		const char* comboLabel = items[itemCurrentIdx];
		if (ImGui::BeginCombo("Type", comboLabel))
		{
			for (int n = 0; n < IM_ARRAYSIZE(items); n++)
			{
				const bool isSelected = (itemCurrentIdx == n);
				if (ImGui::Selectable(items[n], isSelected))
					itemCurrentIdx = n;

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		if (selectedEntity->collider.type != (ColliderType)itemCurrentIdx)
		{
			selectedEntity->collider.type = (ColliderType)itemCurrentIdx;
			if (selectedEntity->collider.type == COLLIDER_CONVEX_HULL)
			{
				selectedEntity->collider.convexHull.meshRes = 0;
			}
		}
	}
	Collider *collider = &selectedEntity->collider;
	switch (collider->type)
	{
	case COLLIDER_CONVEX_HULL:
	{
		static char colMeshResInputName[128] = "";
		ImGui::InputText("Collision mesh resource", colMeshResInputName, IM_ARRAYSIZE(colMeshResInputName));
		//if (ImGui::Button("Apply2"))
		{
			const Resource *res = GetResource(colMeshResInputName);
			if (res)
			{
				collider->convexHull.meshRes = res;
			}
		}
	} break;
	case COLLIDER_SPHERE:
	{
		ImGui::DragFloat3("Offset", collider->sphere.offset.v, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f");
		ImGui::InputFloat("Radius", &collider->sphere.radius);
	} break;
	case COLLIDER_CYLINDER:
	{
		ImGui::DragFloat3("Offset", collider->cylinder.offset.v, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f");
		ImGui::InputFloat("Radius", &collider->cylinder.radius);
		ImGui::InputFloat("Height", &collider->cylinder.height);
	} break;
	case COLLIDER_CAPSULE:
	{
		ImGui::DragFloat3("Offset", collider->capsule.offset.v, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f");
		ImGui::InputFloat("Radius", &collider->capsule.radius);
		ImGui::InputFloat("Height", &collider->capsule.height);
	} break;
	}

	ImGui::End();
#else
	(void)gameState;
#endif
}

