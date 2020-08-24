#include "General.h"

struct Vertex;

struct OBJLoadResult
{
	Vertex *vertices;
	u32 vertexCount;
	u16 *indices;
	u32 indexCount;
};

OBJLoadResult LoadOBJ(const char *filename);
