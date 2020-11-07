#if DEBUG_BUILD
void DrawDebugCube(v3 pos, v3 fw, v3 up, f32 scale, v3 color = {1,0,0})
{
	if (g_debugContext->debugGeometryBuffer.debugCubeCount < 2048)
	{
		g_debugContext->debugGeometryBuffer.debugCubes[g_debugContext->debugGeometryBuffer.debugCubeCount++] =
			{ pos, color, fw, up, scale };
	}
}

void DrawDebugCubeAA(v3 pos, f32 scale, v3 color = {1,0,0})
{
	if (g_debugContext->debugGeometryBuffer.debugCubeCount < 2048)
	{
		g_debugContext->debugGeometryBuffer.debugCubes[g_debugContext->debugGeometryBuffer.debugCubeCount++] =
			{ pos, color, {0,1,0}, {0,0,1}, scale };
	}
}

void DrawDebugTriangles(v3* vertices, int count, v3 color = {1,0,0})
{
	for (int i = 0; i < count; ++i)
	{
		if (g_debugContext->debugGeometryBuffer.triangleVertexCount >= 2048)
			break;

		DebugVertex dv = { vertices[i], color };

		int newIdx = g_debugContext->debugGeometryBuffer.triangleVertexCount++;
		g_debugContext->debugGeometryBuffer.triangleData[newIdx] = dv;
	}
}

void DrawDebugTriangles(DebugVertex* vertices, int count)
{
	for (int i = 0; i < count; ++i)
	{
		if (g_debugContext->debugGeometryBuffer.triangleVertexCount >= 2048)
			break;

		int newIdx = g_debugContext->debugGeometryBuffer.triangleVertexCount++;
		g_debugContext->debugGeometryBuffer.triangleData[newIdx] = vertices[i];
	}
}

void DrawDebugLines(v3* vertices, int count, v3 color = {1,0,0})
{
	for (int i = 0; i < count; ++i)
	{
		if (g_debugContext->debugGeometryBuffer.lineVertexCount >= 2048)
			break;

		DebugVertex dv = { vertices[i], color };

		int newIdx = g_debugContext->debugGeometryBuffer.lineVertexCount++;
		g_debugContext->debugGeometryBuffer.lineData[newIdx] = dv;
	}
}

// Util functions
void DrawDebugWiredBox(v3 min, v3 max, v3 color = {1,0,0})
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
	DrawDebugLines(verts, ArrayCount(verts), color);
}
#endif
