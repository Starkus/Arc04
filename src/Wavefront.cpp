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

// TODO support .objs that have attributes different than position/uv/normal
OBJLoadResult LoadOBJ(const char *filename)
{
	OBJLoadResult result;
	// TODO replace by dynamic arrays
	// TODO allocate in frame allocator?
	result.vertices = (Vertex *)malloc(sizeof(Vertex) * 4096);
	result.vertexCount = 0;
	result.indices = (u16 *)malloc(sizeof(u16) * 4096);
	result.indexCount = 0;

	v3 *vertexBuffer = (v3 *)malloc(sizeof(v3) * 2048);
	v2 *uvBuffer = (v2 *)malloc(sizeof(v2) * 2048);
	v3 *normalBuffer = (v3 *)malloc(sizeof(v3) * 2048);

	u32 vertexCount = 0, uvCount = 0, normalCount = 0;

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
						// Read vertex
						v3 *vertex = &vertexBuffer[vertexCount++];
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
						// Read UV
						v2 *uv = &uvBuffer[uvCount++];
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
						// Read normal
						v3 *normal = &normalBuffer[normalCount++];
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

					Vertex *vertex = &result.vertices[result.vertexCount++];
					vertex->pos = vertexBuffer[indices[0]];
					vertex->uv = uvBuffer[indices[1]];
					vertex->color = normalBuffer[indices[2]]; // FIXME saving color into normal

					// FIXME dumb linear indices, add actual duplicate removal
					// TODO check index doesn't go out of u16 bounds
					result.indices[result.indexCount] = (u16)result.indexCount++;
				}
			} break;
		}

		// Go to next line
		for (; *scan != 0 && *scan++ != '\n'; );
	}

	// Free temp buffers
	free(vertexBuffer);
	free(uvBuffer);
	free(normalBuffer);

	return result;
}
