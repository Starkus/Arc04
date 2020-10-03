#include <stdlib.h>
#include "SDL/SDL.h"
#include "Geometry.h"

// Crash upon encountering invalid character in atof and atoi for debugging purposes
#if 1
#define Atof(str) atof(str)
#define Atoi(str) atoi(str)
#else
f64 Atof(const char *str)
{
	for (const char *scan = str; *scan; ++scan)
	{
		if (*scan == '.')
			continue;
		if (*scan == '-')
			continue;
		if (*scan >= '0' && *scan <= '9')
			continue;
		*((int*)0) = 0;
	}
	return atof(str);
}
int Atoi(const char *str)
{
	for (const char *scan = str; *scan; ++scan)
	{
		if (*scan == '-')
			continue;
		if (*scan >= '0' && *scan <= '9')
			continue;
		*((int*)0) = 0;
	}
	return atoi(str);
}
#endif

#define DECLARE_DYNAMIC_ARRAY(_TYPE, _STRUCTNAME) \
struct _STRUCTNAME \
{ \
	_TYPE *data; \
	u32 size; \
	u32 capacity; \
}; \
\
inline void ArrayAddOne(_STRUCTNAME *array) \
{ \
	if (array->size >= array->capacity) \
	{ \
		array->capacity *= 2; \
		array->data = (_TYPE *)realloc(array->data, array->capacity * sizeof(_TYPE)); \
	} \
	array->size++; \
}

DECLARE_DYNAMIC_ARRAY(v2, DynamicArrayV2)
DECLARE_DYNAMIC_ARRAY(v3, DynamicArrayV3)
DECLARE_DYNAMIC_ARRAY(Vertex, DynamicArrayVertex)
DECLARE_DYNAMIC_ARRAY(u16, DynamicArrayU16)

// TODO support .objs that have attributes different than position/uv/normal
OBJLoadResult LoadOBJ(const char *filename)
{
	// TODO allocate in frame allocator?
	DynamicArrayV3 vertices = { (v3 *)malloc(sizeof(v3) * 32), 0, 32 };
	DynamicArrayV2 uvs = { (v2 *)malloc(sizeof(v2) * 32), 0, 32 };
	DynamicArrayV3 normals = { (v3 *)malloc(sizeof(v3) * 32), 0, 32 };

	DynamicArrayVertex resultVertices = { (Vertex *)malloc(sizeof(Vertex) * 32), 0, 32 };
	DynamicArrayU16 resultIndices = { (u16 *)malloc(sizeof(u16) * 32), 0, 32 };

	SDL_RWops *file = SDL_RWFromFile(filename, "r");
	const u64 fileSize = SDL_RWsize(file);
	char *fileBuffer = (char *)malloc(fileSize);
	SDL_RWread(file, fileBuffer, sizeof(char), fileSize);
	SDL_RWclose(file);

	const char *scan = fileBuffer;
	char numberBuffer[256];
	while (*scan)
	{
		switch (*scan)
		{
			case '#':
			case 'o':
				break;
			case 'v':
			{
				++scan;
				switch (*scan)
				{
					case ' ':
					{
						// Get pointer to next vertex in array
						ArrayAddOne(&vertices);
						v3 *vertex = &vertices.data[vertices.size - 1];
						for (int i = 0; i < 3; ++i)
						{
							++scan; // Skip space
							int numberBufferIdx = 0;
							for (; *scan != ' ' && *scan != '\n'; ++scan)
							{
								numberBuffer[numberBufferIdx++] = *scan;
							}
							numberBuffer[numberBufferIdx] = 0;
							vertex->v[i] = (f32)Atof(numberBuffer);
						}
					} break;
					case 't':
					{
						++scan;
						// Get pointer to next uv in array
						ArrayAddOne(&uvs);
						v2 *uv = &uvs.data[uvs.size - 1];
						for (int i = 0; i < 2; ++i)
						{
							++scan; // Skip space
							int numberBufferIdx = 0;
							for (; *scan != ' ' && *scan != '\n'; ++scan)
							{
								numberBuffer[numberBufferIdx++] = *scan;
							}
							numberBuffer[numberBufferIdx] = 0;
							uv->v[i] = (f32)Atof(numberBuffer);
						}
					} break;
					case 'n':
					{
						++scan;
						// Get pointer to next normal in array
						ArrayAddOne(&normals);
						v3 *normal = &normals.data[normals.size - 1];
						for (int i = 0; i < 3; ++i)
						{
							++scan; // Skip space
							int numberBufferIdx = 0;
							for (; *scan != ' ' && *scan != '\n'; ++scan)
							{
								numberBuffer[numberBufferIdx++] = *scan;
							}
							numberBuffer[numberBufferIdx] = 0;
							normal->v[i] = (f32)Atof(numberBuffer);
						}
					} break;
				}
			} break;
			case 'f':
			{
				++scan;
				// Read triangle
				for (int i = 0; i < 3; ++i)
				{
					int indices[3];
					for (int j = 0; j < 3; ++j)
					{
						++scan;
						int numberBufferIdx = 0;
						for (; *scan != '/' && *scan != ' ' && *scan != '\n'; ++scan)
						{
							numberBuffer[numberBufferIdx++] = *scan;
						}
						numberBuffer[numberBufferIdx] = 0;
						indices[j] = Atoi(numberBuffer) - 1;
					}

					ArrayAddOne(&resultVertices);
					Vertex *vertex = &((Vertex *)resultVertices.data)[resultVertices.size - 1];
					vertex->pos = ((v3 *)vertices.data)[indices[0]];
					vertex->uv = ((v2 *)uvs.data)[indices[1]];
					vertex->color = ((v3 *)normals.data)[indices[2]]; // FIXME saving color into normal
					vertex->color = vertex->color * 0.5f + v3{ 0.5f, 0.5f, 0.5f };

					// FIXME dumb linear indices, add actual duplicate removal
					// TODO check index doesn't go out of u16 bounds
					ArrayAddOne(&resultIndices);
					((u16 *)resultIndices.data)[resultIndices.size - 1] = (u16)resultIndices.size - 1;
				}
			} break;
		}

		// Go to next line
		for (; *scan != 0 && *scan++ != '\n'; );
	}

	// Free temp buffers
	free(vertices.data);
	free(uvs.data);
	free(normals.data);

	OBJLoadResult result;
	result.vertices = (Vertex *)resultVertices.data;
	result.vertexCount = resultVertices.size;
	result.indices = (u16 *)resultIndices.data;
	result.indexCount = resultIndices.size;

	return result;
}

void LoadOBJAsPoints(const char *filename, v3 **points, u32 *pointCount)
{
	// TODO allocate in frame allocator?
	DynamicArrayV3 vertices = { (v3 *)malloc(sizeof(v3) * 32), 0, 32 };

	SDL_RWops *file = SDL_RWFromFile(filename, "r");
	const u64 fileSize = SDL_RWsize(file);
	char *fileBuffer = (char *)malloc(fileSize);
	SDL_RWread(file, fileBuffer, sizeof(char), fileSize);
	SDL_RWclose(file);

	const char *scan = fileBuffer;
	char numberBuffer[256];
	while (*scan)
	{
		switch (*scan)
		{
			case '#':
			case 'o':
			case 'f':
				break;
			case 'v':
			{
				++scan;
				switch (*scan)
				{
					case 't':
					case 'n':
						break;
					case ' ':
					{
						// Get pointer to next vertex in array
						ArrayAddOne(&vertices);
						v3 *vertex = &vertices.data[vertices.size - 1];
						for (int i = 0; i < 3; ++i)
						{
							++scan; // Skip space
							int numberBufferIdx = 0;
							for (; *scan != ' ' && *scan != '\n'; ++scan)
							{
								numberBuffer[numberBufferIdx++] = *scan;
							}
							numberBuffer[numberBufferIdx] = 0;
							vertex->v[i] = (f32)Atof(numberBuffer);
						}
					} break;
				}
			} break;
		}

		// Go to next line
		for (; *scan != 0 && *scan++ != '\n'; );
	}

	*points = vertices.data;
	*pointCount = vertices.size;
}
