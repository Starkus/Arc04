#if DEBUG_BUILD
void DrawDebugCube(GameState *gameState, v3 pos, v3 fw, v3 up, f32 scale)
{
	if (gameState->debugGeometryBuffer.debugCubeCount < 2048)
		gameState->debugGeometryBuffer.debugCubes[gameState->debugGeometryBuffer.debugCubeCount++] = { pos, fw, up, scale };
}

void DrawDebugCubeAA(GameState *gameState, v3 pos, f32 scale)
{
	if (gameState->debugGeometryBuffer.debugCubeCount < 2048)
		gameState->debugGeometryBuffer.debugCubes[gameState->debugGeometryBuffer.debugCubeCount++] = { pos, {0,1,0}, {0,0,1}, scale };
}

void DrawDebugTriangles(GameState *gameState, Vertex* vertices, int count)
{
	for (int i = 0; i < count; ++i)
	{
		if (gameState->debugGeometryBuffer.triangleVertexCount >= 2048)
			break;

		int newIdx = gameState->debugGeometryBuffer.triangleVertexCount++;
		gameState->debugGeometryBuffer.triangleData[newIdx] = vertices[i];
	}
}
void DrawDebugLines(GameState *gameState, Vertex* vertices, int count)
{
	for (int i = 0; i < count; ++i)
	{
		if (gameState->debugGeometryBuffer.lineVertexCount >= 2048)
			break;

		int newIdx = gameState->debugGeometryBuffer.lineVertexCount++;
		gameState->debugGeometryBuffer.lineData[newIdx] = vertices[i];
	}
}
#endif
