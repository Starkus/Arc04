#if DEBUG_BUILD
void DrawDebugCube(GameState *gameState, v3 pos, v3 fw, v3 up, f32 scale, v3 color = {1,0,0})
{
	if (gameState->debugGeometryBuffer.debugCubeCount < 2048)
	{
		gameState->debugGeometryBuffer.debugCubes[gameState->debugGeometryBuffer.debugCubeCount++] =
			{ pos, color, fw, up, scale };
	}
}

void DrawDebugCubeAA(GameState *gameState, v3 pos, f32 scale, v3 color = {1,0,0})
{
	if (gameState->debugGeometryBuffer.debugCubeCount < 2048)
	{
		gameState->debugGeometryBuffer.debugCubes[gameState->debugGeometryBuffer.debugCubeCount++] =
			{ pos, color, {0,1,0}, {0,0,1}, scale };
	}
}

void DrawDebugTriangles(GameState *gameState, v3* vertices, int count, v3 color = {1,0,0})
{
	for (int i = 0; i < count; ++i)
	{
		if (gameState->debugGeometryBuffer.triangleVertexCount >= 2048)
			break;

		DebugVertex dv = { vertices[i], color };

		int newIdx = gameState->debugGeometryBuffer.triangleVertexCount++;
		gameState->debugGeometryBuffer.triangleData[newIdx] = dv;
	}
}
void DrawDebugLines(GameState *gameState, v3* vertices, int count, v3 color = {1,0,0})
{
	for (int i = 0; i < count; ++i)
	{
		if (gameState->debugGeometryBuffer.lineVertexCount >= 2048)
			break;

		DebugVertex dv = { vertices[i], color };

		int newIdx = gameState->debugGeometryBuffer.lineVertexCount++;
		gameState->debugGeometryBuffer.lineData[newIdx] = dv;
	}
}

// Util functions
void DrawDebugWiredBox(GameState *gameState, v3 min, v3 max, v3 color = {1,0,0})
{
	v3 verts[] =
	{
		{ min.x, min.y, min.z }, { max.x, min.y, min.z },
		{ max.x, min.y, min.z }, { max.x, max.y, min.z },
		{ max.x, max.y, min.z }, { min.x, max.y, min.z },
		{ min.x, max.y, min.z }, { min.x, min.y, min.z },

		{ min.x, min.y, min.z }, { min.x, min.y, max.z },
		{ max.x, min.y, min.z }, { max.x, min.y, max.z },
		{ max.x, max.y, min.z }, { max.x, max.y, max.z },
		{ min.x, max.y, min.z }, { min.x, max.y, max.z },

		{ min.x, min.y, max.z }, { max.x, min.y, max.z },
		{ max.x, min.y, max.z }, { max.x, max.y, max.z },
		{ max.x, max.y, max.z }, { min.x, max.y, max.z },
		{ min.x, max.y, max.z }, { min.x, min.y, max.z },
	};
	DrawDebugLines(gameState, verts, ArrayCount(verts), color);
}
#endif
