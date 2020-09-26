#include "SDL/SDL.h"
#include "SDL/SDL_OpenGL.h"

#include "General.h"
#include "Math.h"
#include "Geometry.h"
#include "Primitives.h"

#include "Wavefront.h"

namespace {
#include "OpenGL.h"

const GLchar *vertexShaderSource = "\
#version 330 core\n\
layout (location = 0) in vec3 pos;\n\
layout (location = 1) in vec3 col;\n\
uniform mat4 model;\n\
uniform mat4 view;\n\
uniform mat4 projection;\n\
out vec3 vertexColor;\n\
\n\
void main()\n\
{\n\
	gl_Position = projection * view * model * vec4(pos, 1.0);\n\
	vertexColor = col;\n\
}\n\
";

const GLchar *fragShaderSource = "\
#version 330 core\n\
in vec3 vertexColor;\n\
out vec4 fragColor;\n\
\n\
void main()\n\
{\n\
	fragColor = vec4(vertexColor, 1.0);\n\
}\n\
";

struct DeviceMesh
{
	GLuint vao;
	union
	{
		struct
		{
			GLuint vertexBuffer;
			GLuint indexBuffer;
		};
		GLuint buffers[2];
	};
	u32 indexCount;
};

#if DEBUG_BUILD
struct DebugCube
{
	v3 pos;
	v3 fw;
	v3 up;
	f32 scale;
};
DebugCube debugCubes[2048];
u32 debugCubeCount;
#define DRAW_DEBUG_CUBE(pos, fw, up, scale) if (debugCubeCount < 2048) debugCubes[debugCubeCount++] = { pos, fw, up, scale }
#define DRAW_AA_DEBUG_CUBE(pos, scale) if (debugCubeCount < 2048) debugCubes[debugCubeCount++] = { pos, {0,1,0}, {0,0,1}, scale }
#endif

struct Controller
{
	bool up;
	bool down;
	bool left;
	bool right;
	bool camUp;
	bool camDown;
	bool camLeft;
	bool camRight;
};

struct Entity
{
	v3 pos;
	v3 fw;
};

struct GameState
{
	Controller controller;
	v3 camPos;
	f32 camYaw;
	f32 camPitch;
	u32 entityCount;
	Entity entities[256];
	u32 playerEntity;
};

GLuint LoadShader(const GLchar *shaderSource, GLuint shaderType)
{
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderSource, nullptr);
	glCompileShader(shader);

#if defined(DEBUG_BUILD)
	{
		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status != GL_TRUE)
		{
			char msg[256];
			GLsizei len;
			glGetShaderInfoLog(shader, sizeof(msg), &len, msg);
			SDL_Log("Error compiling shader: %s", msg);
		}
	}
#endif

	return shader;
}

v3 FurthestInDirection(v3 *points, u32 pointCount, v3 dir)
{
	f32 maxDist = -INFINITY;
	v3 result = {};

	v3 *scan = points;
	for (u32 i = 0; i < pointCount; ++i)
	{
		v3 p = *scan++;
		f32 dot = V3Dot(p, dir);
		if (dot > maxDist)
		{
			maxDist = dot;
			result = p;
		}
	}

	return result;
}

inline v3 GJKSupport(v3 *pointsA, u32 pointCountA, v3 *pointsB, u32 pointCountB, v3 dir)
{
	v3 va = FurthestInDirection(pointsA, pointCountA, dir);
	v3 vb = FurthestInDirection(pointsB, pointCountB, -dir);
	return va - vb;
}

bool GJKTest(Entity *entityA, Entity *entityB, v3 *depenetration)
{
	bool intersection = true;
	v3 up = { 0, 0, 1 };

	const v3 aPos = entityA->pos;
	const v3 aFw = entityA->fw;
	const v3 aRight = V3Normalize(V3Cross(aFw, up));
	const v3 aUp = V3Cross(aRight, aFw);
	mat4 aModelMatrix =
	{
		aRight.x,	aRight.y,	aRight.z,	0.0f,
		aFw.x,		aFw.y,		aFw.z,		0.0f,
		aUp.x,		aUp.y,		aUp.z,		0.0f,
		aPos.x,		aPos.y,		aPos.z,		1.0f
	};

	const v3 bPos = entityB->pos;
	const v3 bFw = entityB->fw;
	const v3 bRight = V3Normalize(V3Cross(bFw, up));
	const v3 bUp = V3Cross(bRight, bFw);
	mat4 bModelMatrix =
	{
		bRight.x,	bRight.y,	bRight.z,	0.0f,
		bFw.x,		bFw.y,		bFw.z,		0.0f,
		bUp.x,		bUp.y,		bUp.z,		0.0f,
		bPos.x,		bPos.y,		bPos.z,		1.0f
	};

	v3 vA[24];
	v3 vB[24];

	// Compute all points
	// TODO de-hardcode cube primitive
	{
		for (int i = 0; i < 24; ++i)
		{
			const Vertex *vert = &cubeVertices[i];
			v4 v = { vert->pos.x, vert->pos.y, vert->pos.z, 1.0f };
			v = Mat4TransformV4(bModelMatrix, v);
			vB[i] = { v.x, v.y, v.z };
		}

		for (int i = 0; i < 24; ++i)
		{
			const Vertex *vert = &cubeVertices[i];
			v4 v = { vert->pos.x, vert->pos.y, vert->pos.z, 1.0f };
			v = Mat4TransformV4(aModelMatrix, v);
			vA[i] = { v.x, v.y, v.z };
		}
	}

	v3 foundPoints[4];
	int foundPointsCount = 1;
	v3 testDir = { 0, 1, 0 };

	foundPoints[0] = GJKSupport(vB, 24, vA, 24, testDir);
	testDir = -foundPoints[0];

	while (intersection && foundPointsCount < 4)
	{
		v3 a = GJKSupport(vB, 24, vA, 24, testDir);
		if (V3Dot(testDir, a) < 0)
		{
			intersection = false;
			break;
		}

		switch (foundPointsCount)
		{
			case 1: // Line
			{
				const v3 b = foundPoints[0];

				foundPoints[foundPointsCount++] = a;
				testDir = V3Cross(V3Cross(b - a, -a), b - a);
			} break;
			case 2: // Plane
			{
				const v3 b = foundPoints[1];
				const v3 c = foundPoints[0];

				const v3 ab = b - a;
				const v3 ac = c - a;
				const v3 nor = V3Cross(ac, ab);
				v3 abNor = V3Cross(nor, ab);
				v3 acNor = V3Cross(ac, nor);
				if (V3Dot(acNor, -a) > 0)
				{
					foundPoints[0] = a;
					foundPoints[1] = c;
					testDir = V3Cross(V3Cross(ac, -a), ac);
				}
				else
				{
					if (V3Dot(abNor, -a) > 0)
					{
						foundPoints[0] = a;
						foundPoints[1] = b;
						testDir = V3Cross(V3Cross(ab, -a), ab);
					}
					else
					{
						if (V3Dot(nor, -a) > 0)
						{
							foundPoints[foundPointsCount++] = a;
							testDir = nor;
						}
						else
						{
							foundPoints[0] = b;
							foundPoints[1] = c;
							foundPoints[2] = a;
							++foundPointsCount;
							testDir = -nor;
						}

						// Assert triangle is wound clockwise
						const v3 A = foundPoints[2];
						const v3 B = foundPoints[1];
						const v3 C = foundPoints[0];
						ASSERT(V3Dot(testDir, V3Cross(C - A, B - A)) >= 0);
					}
				}
			} break;
			case 3: // Tetrahedron
			{
				const v3 b = foundPoints[2];
				const v3 c = foundPoints[1];
				const v3 d = foundPoints[0];

				const v3 ab = b - a;
				const v3 ac = c - a;
				const v3 ad = d - a;

				v3 abcNor = V3Cross(ac, ab);
				v3 adbNor = V3Cross(ab, ad);
				v3 acdNor = V3Cross(ad, ac);

				// Assert normals point outside
				ASSERT(V3Dot(d - a, abcNor) < 0);
				ASSERT(V3Dot(c - a, adbNor) < 0);
				ASSERT(V3Dot(b - a, acdNor) < 0);

				if (V3Dot(abcNor, -a) > 0)
				{
					if (V3Dot(adbNor, -a) > 0)
					{
						foundPoints[0] = b;
						foundPoints[1] = a;
						foundPointsCount = 2;
						testDir = V3Cross(V3Cross(ab, -a), ab);
					}
					else
					{
						if (V3Dot(acdNor, -a) > 0)
						{
							foundPoints[0] = c;
							foundPoints[1] = a;
							foundPointsCount = 2;
							testDir = V3Cross(V3Cross(ac, -a), ac);
						}
						else
						{
							if (V3Dot(V3Cross(abcNor, ab), -a) > 0)
							{
								foundPoints[0] = b;
								foundPoints[1] = a;
								foundPointsCount = 2;
								testDir = V3Cross(V3Cross(ab, -a), ab);
							}
							else
							{
								if (V3Dot(V3Cross(ac, abcNor), -a) > 0)
								{
									foundPoints[0] = c;
									foundPoints[1] = a;
									foundPointsCount = 2;
									testDir = V3Cross(V3Cross(ac, -a), ac);
								}
								else
								{
									foundPoints[0] = c;
									foundPoints[1] = b;
									foundPoints[2] = a;
									testDir = abcNor;
								}
							}
						}
					}
				}
				else
				{
					if (V3Dot(acdNor, -a) > 0)
					{
						if (V3Dot(adbNor, -a) > 0)
						{
							foundPoints[0] = d;
							foundPoints[1] = a;
							foundPointsCount = 2;
							testDir = V3Cross(V3Cross(ad, -a), ad);
						}
						else
						{
							if (V3Dot(V3Cross(acdNor, ac), -a) > 0)
							{
								foundPoints[0] = c;
								foundPoints[1] = a;
								foundPointsCount = 2;
								testDir = V3Cross(V3Cross(ac, -a), ac);
							}
							else
							{
								if (V3Dot(V3Cross(ad, acdNor), -a) > 0)
								{
									foundPoints[0] = d;
									foundPoints[1] = a;
									foundPointsCount = 2;
									testDir = V3Cross(V3Cross(ad, -a), ad);
								}
								else
								{
									foundPoints[0] = d;
									foundPoints[1] = c;
									foundPoints[2] = a;
									testDir = acdNor;
								}
							}
						}
					}
					else
					{
						if (V3Dot(adbNor, -a) > 0)
						{
							if (V3Dot(V3Cross(adbNor, ad), -a) > 0)
							{
								foundPoints[0] = d;
								foundPoints[1] = a;
								foundPointsCount = 2;
								testDir = V3Cross(V3Cross(ad, -a), ad);
							}
							else
							{
								if (V3Dot(V3Cross(ab, adbNor), -a) > 0)
								{
									foundPoints[0] = b;
									foundPoints[1] = a;
									foundPointsCount = 2;
									testDir = V3Cross(V3Cross(ab, -a), ab);
								}
								else
								{
									foundPoints[0] = b;
									foundPoints[1] = d;
									foundPoints[2] = a;
									testDir = adbNor;
								}
							}
						}
						else
						{
							// Done!
							foundPoints[foundPointsCount++] = a;
						}
					}
				}
			} break;
		}
	}

	if (!intersection)
		return false;

	// EPA!
	// By now we should have the tetrahedron from GJK
	struct Face
	{
		v3 a;
		v3 b;
		v3 c;
	};

	struct Edge
	{
		v3 a;
		v3 b;
	};

	// TODO dynamic array?
	Face polytope[256];
	int polytopeCount = 0;

	// Make all faces from tetrahedron
	for (int i = 0; i < 4; ++i)
	{
		for (int j = i+1; j < 4; ++j)
		{
			for (int k = j+1; k < 4; ++k)
			{
				polytope[polytopeCount++] =
				{
					foundPoints[i],
					foundPoints[j],
					foundPoints[k]
				};
			}
		}
	}

	// Wind triangles so they face out
	// TODO There must be a way to avoid this
	for (int faceIdx = 0; faceIdx < 4; ++faceIdx)
	{
		Face *face = &polytope[faceIdx];
		v3 normal = V3Cross(face->c - face->a, face->b - face->a);
		if (V3Dot(normal, face->a) < 0)
		{
			// Rewind
			v3 tmp = face->b;
			face->b = face->c;
			face->c = tmp;
		}
	}

	Face closestFeature;
	while (1) // TODO limit iterations for edge cases with colinear triangles?
	{
		// Find closest feature to origin in polytope
		f32 leastDistance = INFINITY;
		for (int faceIdx = 0; faceIdx < polytopeCount; ++faceIdx)
		{
			Face *face = &polytope[faceIdx];
			v3 normal = V3Normalize(V3Cross(face->c - face->a, face->b - face->a));
			f32 distToOrigin = V3Dot(normal, face->a);
			if (distToOrigin >= leastDistance)
				continue;

			// Make sure projected origin is within triangle
			const v3 abNor = V3Cross(face->a - face->b, normal);
			const v3 bcNor = V3Cross(face->b - face->c, normal);
			const v3 acNor = V3Cross(face->c - face->a, normal);

			if (V3Dot(abNor, -face->a) > 0 ||
				V3Dot(bcNor, -face->b) > 0 ||
				V3Dot(acNor, -face->a) > 0)
				continue;

			leastDistance = distToOrigin;
			closestFeature = *face;
		}
		if (leastDistance == INFINITY)
		{
			// Collision is probably on the very edge, we don't need depenetration
			intersection = false;
			break;
		}

		// Expand polytope!
		testDir = V3Cross(closestFeature.c - closestFeature.a, closestFeature.b - closestFeature.a);
		v3 newPoint = GJKSupport(vB, 24, vA, 24, testDir);
		if (V3Dot(testDir, newPoint - closestFeature.a) <= 0)
		{
			break;
		}
		Edge holeEdges[256];
		int holeEdgesCount = 0;
#if DEBUG_BUILD
		int oldPolytopeCount = polytopeCount;
#endif
		for (int faceIdx = 0; faceIdx < polytopeCount; ++faceIdx)
		{
			Face *face = &polytope[faceIdx];
			v3 normal = V3Cross(face->c - face->a, face->b - face->a);
			if (V3Dot(normal, newPoint - face->a) > 0)
			{
				// Add/remove edges to the hole (XOR)
				Edge faceEdges[3] =
				{
					{ face->a, face->b },
					{ face->b, face->c },
					{ face->c, face->a }
				};
				for (int edgeIdx = 0; edgeIdx < 3; ++edgeIdx)
				{
					const Edge &edge = faceEdges[edgeIdx];
					// If it's already on the list, remove it
					bool found = false;
					for (int holeEdgeIdx = 0; holeEdgeIdx < holeEdgesCount; ++holeEdgeIdx)
					{
						const Edge &holeEdge = holeEdges[holeEdgeIdx];
						if ((edge.a == holeEdge.a && edge.b == holeEdge.b) ||
							(edge.a == holeEdge.b && edge.b == holeEdge.a))
						{
							holeEdges[holeEdgeIdx] = holeEdges[--holeEdgesCount];
							found = true;
							break;
						}
					}
					// Otherwise add it
					if (!found)
						holeEdges[holeEdgesCount++] = edge;
				}
				// Remove face from polytope
				polytope[faceIdx] = polytope[--polytopeCount];
				--faceIdx;
			}
		}
#if DEBUG_BUILD
		int deletedFaces = oldPolytopeCount - polytopeCount;
		ASSERT(deletedFaces == 1 || holeEdgesCount < deletedFaces * 3);
#endif
		// Now we should have a hole in the polytope, of which all edges are in holeEdges
		for (int holeEdgeIdx = 0; holeEdgeIdx < holeEdgesCount; ++holeEdgeIdx)
		{
			const Edge &holeEdge = holeEdges[holeEdgeIdx];
			Face newFace = { holeEdge.a, holeEdge.b, newPoint };
			// Rewind if it's facing inwards
			v3 normal = V3Cross(newFace.c - newFace.a, newFace.b - newFace.a);
			if (V3Dot(normal, newFace.a) < 0)
			{
				v3 tmp = newFace.b;
				newFace.b = newFace.c;
				newFace.c = tmp;
			}
			polytope[polytopeCount++] = newFace;
		}
	}

	if (intersection)
	{
		v3 closestFeatureNor = V3Normalize(V3Cross(closestFeature.c - closestFeature.a,
				closestFeature.b - closestFeature.a));
		*depenetration = closestFeatureNor * V3Dot(closestFeatureNor, closestFeature.a);
	}

	return true;
}

void StartGame()
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	// OpenGL versions
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	// OpenGL no backwards comp and debug flag
	i32 contextFlags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
	DEBUG_ONLY(contextFlags |= SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, contextFlags);

	SDL_Window *window = SDL_CreateWindow("GAME", 16, 16, 960, 540, SDL_WINDOW_OPENGL);
	if (window == nullptr)
		SDL_Log("No window!");

	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	if (glContext == nullptr)
		SDL_Log("No context!");

	LoadOpenGLProcs();

	DeviceMesh playerMesh, cubeMesh, planeMesh;

	GLuint program;
	{
		// Player
		{
			OBJLoadResult obj = LoadOBJ("data/monkey.obj");
			playerMesh.indexCount = obj.indexCount;

			glGenVertexArrays(1, &playerMesh.vao);
			glBindVertexArray(playerMesh.vao);

			glGenBuffers(2, playerMesh.buffers);
			glBindBuffer(GL_ARRAY_BUFFER, playerMesh.vertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(obj.vertices[0]) * obj.vertexCount, obj.vertices, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, playerMesh.indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(obj.indices[0]) * obj.indexCount, obj.indices, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, color));
			glEnableVertexAttribArray(1);

			free(obj.vertices);
			free(obj.indices);
		}

		// Cube
		{
			cubeMesh.indexCount = 6 * 2 * 3;

			glGenVertexArrays(1, &cubeMesh.vao);
			glBindVertexArray(cubeMesh.vao);

			glGenBuffers(2, cubeMesh.buffers);
			glBindBuffer(GL_ARRAY_BUFFER, cubeMesh.vertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeMesh.indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, color));
			glEnableVertexAttribArray(1);
		}

		// Plane
		{
			planeMesh.indexCount = 2 * 3;

			glGenVertexArrays(1, &planeMesh.vao);
			glBindVertexArray(planeMesh.vao);

			glGenBuffers(2, planeMesh.buffers);
			glBindBuffer(GL_ARRAY_BUFFER, planeMesh.vertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeMesh.indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, color));
			glEnableVertexAttribArray(1);
		}

		// Shaders
		GLuint vertexShader = LoadShader(vertexShaderSource, GL_VERTEX_SHADER);
		GLuint fragmentShader = LoadShader(fragShaderSource, GL_FRAGMENT_SHADER);

		program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);
#if defined(DEBUG_BUILD)
		{
			GLint status;
			glGetProgramiv(program, GL_LINK_STATUS, &status);
			if (status != GL_TRUE)
			{
				char msg[256];
				GLsizei len;
				glGetProgramInfoLog(program, sizeof(msg), &len, msg);
				SDL_Log("Error linking shader program: %s", msg);
			}
		}
#endif

		glUseProgram(program);

		glEnable(GL_DEPTH_TEST);

		const f32 fov = HALFPI;
		const f32 near = 0.01f;
		const f32 far = 2000.0f;
		const f32 aspectRatio = (16.0f / 9.0f);

		const f32 top = Tan(HALFPI - fov / 2.0f);
		const f32 right = top / aspectRatio;

		mat4 proj =
		{
			right, 0.0f, 0.0f, 0.0f,
			0.0f, top, 0.0f, 0.0f,
			0.0f, 0.0f, -(far + near) / (far - near), -1.0f,
			0.0f, 0.0f, -(2.0f * far * near) / (far - near), 0.0f
		};
		GLuint projUniform = glGetUniformLocation(program, "projection");
		glUniformMatrix4fv(projUniform, 1, false, proj.m);
	}

	GameState gameState = {};
	gameState.playerEntity = gameState.entityCount++;
	gameState.entities[gameState.playerEntity].fw = { 0.0f, 1.0f, 0.0f };
	gameState.camPitch = -PI * 0.35f;

	// Test entities
	Entity *testEntity = &gameState.entities[gameState.entityCount++];
	testEntity->pos = { -5.0f, 2.0f, 0.0f };
	testEntity->fw = { 0.0f, 1.0f, 0.0f };

	testEntity = &gameState.entities[gameState.entityCount++];
	testEntity->pos = { 4.0f, 3.0f, 0.0f };
	testEntity->fw = { 0.0f, 1.0f, 0.0f };

	testEntity = &gameState.entities[gameState.entityCount++];
	testEntity->pos = { 2.0f, -3.0f, 0.0f };
	testEntity->fw = { 0.707f, 0.707f, 0.0f };

	testEntity = &gameState.entities[gameState.entityCount++];
	testEntity->pos = { -7.0f, -3.0f, 0.0f };
	testEntity->fw = { 0.707f, 0.0f, 0.707f };

	u64 lastPerfCounter = SDL_GetPerformanceCounter();
	bool running = true;
	while (running)
	{
		const u64 newPerfCounter = SDL_GetPerformanceCounter();
		const f64 time = (f64)newPerfCounter / (f64)SDL_GetPerformanceFrequency();
		const f32 deltaTime = (f32)(newPerfCounter - lastPerfCounter) / (f32)SDL_GetPerformanceFrequency();

#if DEBUG_BUILD
		debugCubeCount = 0;
#endif

		// Check events
		SDL_Event evt;
		while (SDL_PollEvent(&evt) != 0)
		{
			switch (evt.type)
			{
				case SDL_QUIT:
				{
					running = false;
				} break;
				case SDL_KEYUP:
				case SDL_KEYDOWN:
				{
					const bool isDown = evt.key.state == SDL_PRESSED;
					switch (evt.key.keysym.sym)
					{
						case SDLK_q:
							running = false;
							break;
						case SDLK_w:
							gameState.controller.up = isDown;
							break;
						case SDLK_a:
							gameState.controller.left = isDown;
							break;
						case SDLK_s:
							gameState.controller.down = isDown;
							break;
						case SDLK_d:
							gameState.controller.right = isDown;
							break;

						case SDLK_UP:
							gameState.controller.camUp = isDown;
							break;
						case SDLK_DOWN:
							gameState.controller.camDown = isDown;
							break;
						case SDLK_LEFT:
							gameState.controller.camLeft = isDown;
							break;
						case SDLK_RIGHT:
							gameState.controller.camRight = isDown;
							break;
					}
				} break;
			}
		}

		// Update
		{
			if (gameState.controller.camUp)
				gameState.camPitch += 1.0f * deltaTime;
			else if (gameState.controller.camDown)
				gameState.camPitch -= 1.0f * deltaTime;

			if (gameState.controller.camLeft)
				gameState.camYaw -= 1.0f * deltaTime;
			else if (gameState.controller.camRight)
				gameState.camYaw += 1.0f * deltaTime;

			// Move player
			{
				v3 worldInputDir = {};
				if (gameState.controller.up)
					worldInputDir += { Sin(gameState.camYaw), Cos(gameState.camYaw) };
				else if (gameState.controller.down)
					worldInputDir += { -Sin(gameState.camYaw), -Cos(gameState.camYaw) };

				if (gameState.controller.left)
					worldInputDir += { -Cos(gameState.camYaw), Sin(gameState.camYaw) };
				else if (gameState.controller.right)
					worldInputDir += { Cos(gameState.camYaw), -Sin(gameState.camYaw) };

				f32 sqlen = V3SqrLen(worldInputDir);
				if (sqlen > 1)
					worldInputDir /= Sqrt(sqlen);

				Entity *player = &gameState.entities[gameState.playerEntity];

				if (V3SqrLen(worldInputDir) > 0)
				{
					player->fw = worldInputDir;
				}

				const f32 playerSpeed = 3.0f;
				v3 newPlayerPos = player->pos + worldInputDir * playerSpeed * deltaTime;

				// Collision
				Entity newPlayerEntity = *player;
				newPlayerEntity.pos = newPlayerPos;
				bool intersection = false;
				for (u32 entityIndex = 0; entityIndex < gameState.entityCount; ++ entityIndex)
				{
					if (entityIndex != gameState.playerEntity)
					{
						v3 depenetration;
						if (GJKTest(&newPlayerEntity, &gameState.entities[entityIndex], &depenetration))
						{
							intersection = true;
							newPlayerEntity.pos += depenetration;
							break;
						}
					}
				}

				player->pos = newPlayerEntity.pos;
				gameState.camPos = newPlayerEntity.pos;
			}

#if DEBUG_BUILD
			// Draw a cube for each entity
			for (u32 entityIdx = 0; entityIdx < gameState.entityCount; ++entityIdx)
			{
				Entity *entity = &gameState.entities[entityIdx];
				v3 up = { 0, 0, 1 };
				DRAW_DEBUG_CUBE(entity->pos, entity->fw, up, 1.0f);
			}
#endif
		}

		// Draw
		{
			// View matrix
			{
				mat4 view =
				{
					1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, -5.0f, 1.0f
				};
				const f32 pitch = gameState.camPitch;
				mat4 pitchMatrix =
				{
					1.0f,	0.0f,			0.0f,		0.0f,
					0.0f,	Cos(pitch),		Sin(pitch),	0.0f,
					0.0f,	-Sin(pitch),	Cos(pitch),	0.0f,
					0.0f,	0.0f,			0.0f,		1.0f
				};
				view = Mat4Multiply(pitchMatrix, view);
				const f32 yaw = gameState.camYaw;
				mat4 yawMatrix =
				{
					Cos(yaw),	Sin(yaw),	0.0f,	0.0f,
					-Sin(yaw),	Cos(yaw),	0.0f,	0.0f,
					0.0f,		0.0f,		1.0f,	0.0f,
					0.0f,		0.0f,		0.0f,	1.0f
				};
				view = Mat4Multiply(yawMatrix, view);
				const v3 pos = gameState.camPos;
				mat4 camPosMatrix =
				{
					1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					-pos.x, -pos.y, -pos.z, 1.0f
				};
				view = Mat4Multiply(camPosMatrix, view);
				GLuint viewUniform = glGetUniformLocation(program, "view");
				glUniformMatrix4fv(viewUniform, 1, false, view.m);
			}

			glClearColor(0.95f, 0.88f, 0.05f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Player
			{
				Entity *player = &gameState.entities[gameState.playerEntity];
				const v3 pos = player->pos;
				const v3 fw = player->fw;
				const v3 right = V3Cross(player->fw, {0,0,1});
				const v3 up = V3Cross(right, fw);
				// TODO double check axis system (left/right hand and so on)
				// TODO pull out function?
				const mat4 model =
				{
					right.x,	right.y,	right.z,	0.0f,
					fw.x,		fw.y,		fw.z,		0.0f,
					up.x,		up.y,		up.z,		0.0f,
					pos.x,		pos.y,		pos.z,		1.0f
				};
				const GLuint modelUniform = glGetUniformLocation(program, "model");
				glUniformMatrix4fv(modelUniform, 1, false, model.m);

				glBindVertexArray(playerMesh.vao);
				glDrawElements(GL_TRIANGLES, playerMesh.indexCount, GL_UNSIGNED_SHORT, NULL);
			}

#if DEBUG_BUILD
			// Debug draws
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

				for (u32 i = 0; i < debugCubeCount; ++i)
				{
					DebugCube *cube = &debugCubes[i];

					v3 pos = cube->pos;
					v3 fw = cube->fw * cube->scale;
					v3 right = V3Normalize(V3Cross(cube->fw, cube->up));
					v3 up = V3Cross(right, cube->fw) * cube->scale;
					right *= cube->scale;
					mat4 model =
					{
						right.x,	right.y,	right.z,	0.0f,
						fw.x,		fw.y,		fw.z,		0.0f,
						up.x,		up.y,		up.z,		0.0f,
						pos.x,		pos.y,		pos.z,		1.0f
					};
					const GLuint modelUniform = glGetUniformLocation(program, "model");
					glUniformMatrix4fv(modelUniform, 1, false, model.m);
					glBindVertexArray(cubeMesh.vao);
					glDrawElements(GL_TRIANGLES, cubeMesh.indexCount, GL_UNSIGNED_SHORT, NULL);
				}

				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
#endif

			// Plane
			{
				mat4 model =
				{
					1.0f,	0.0f,	0.0f,	0.0f,
					0.0f,	1.0f,	0.0f,	0.0f,
					0.0f,	0.0f,	1.0f,	0.0f,
					0.0f,	0.0f,	-1.0f,	1.0f
				};
				const GLuint modelUniform = glGetUniformLocation(program, "model");
				glUniformMatrix4fv(modelUniform, 1, false, model.m);

				glBindVertexArray(planeMesh.vao);
				glDrawElements(GL_TRIANGLES, planeMesh.indexCount, GL_UNSIGNED_SHORT, NULL);
			}

			SDL_GL_SwapWindow(window);
		}

		lastPerfCounter = newPerfCounter;
	}

	// Cleanup
	{
		glDeleteBuffers(1, &playerMesh.vertexBuffer);
		glDeleteBuffers(1, &planeMesh.vertexBuffer);
		glDeleteBuffers(1, &cubeMesh.vertexBuffer);
		glDeleteVertexArrays(1, &playerMesh.vao);
		glDeleteVertexArrays(1, &planeMesh.vao);
		glDeleteVertexArrays(1, &cubeMesh.vao);

		SDL_GL_DeleteContext(glContext);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	return;
}
}; // end anonymous namespace

int main(int argc, char **argv)
{
	(void) argc, argv;
	StartGame();
	return 0;
}
