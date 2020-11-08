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

