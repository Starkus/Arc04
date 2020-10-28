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
		gameState->debugGeometryBuffer.triangleData[gameState->debugGeometryBuffer.triangleVertexCount + i] = vertices[i];
	}
	gameState->debugGeometryBuffer.triangleVertexCount += count;
}
void DrawDebugLines(GameState *gameState, Vertex* vertices, int count)
{
	for (int i = 0; i < count; ++i)
	{
		gameState->debugGeometryBuffer.lineData[gameState->debugGeometryBuffer.lineVertexCount + i] = vertices[i];
	}
	gameState->debugGeometryBuffer.lineVertexCount += count;
}
#endif
