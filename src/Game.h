enum ColliderType
{
	COLLIDER_CONVEX_HULL,
	COLLIDER_SPHERE,
	COLLIDER_CYLINDER,
	COLLIDER_CAPSULE
};

struct Collider
{
	ColliderType type;
	union
	{
		struct
		{
			const Resource *meshRes;
		} convexHull;
		struct
		{
			f32 radius;
			v3 offset;
		} sphere;
		struct
		{
			f32 radius;
			f32 height;
			v3 offset;
		} cylinder, capsule;
	};
};

struct EntityHandle
{
	u32 id;
	u8 generation;
};
EntityHandle ENTITY_HANDLE_INVALID = { U32_MAX, 0 };

struct SkinnedMeshInstance
{
	EntityHandle entityHandle;
	const Resource *meshRes;
	int animationIdx;
	f32 animationTime;
};

struct Particle
{
	v3 pos;
	v3 color;
	f32 size;
};

struct ParticleBookkeep
{
	f32 lifeTime;
	f32 duration;
	v3 velocity;
};

struct ParticleSystem
{
	EntityHandle entityHandle;
	DeviceMesh deviceBuffer;
	f32 timer;
	bool alive[256]; // @Improve
	ParticleBookkeep bookkeeps[256];
	Particle particles[256];
};

struct Entity
{
	v3 pos;
	v4 rot;

	// @Todo: decide how to handle optional things.
	const Resource *mesh;
	SkinnedMeshInstance *skinnedMeshInstance;
	ParticleSystem *particleSystem;

	Collider collider;
};

struct LevelGeometry
{
	const Resource *renderMesh;
	const Resource *geometryGrid;
};

enum PlayerStateFlags
{
	PLAYERSTATEFLAG_AIRBORNE = 0x8000
};

enum PlayerState
{
	// Grounded
	PLAYERSTATE_IDLE = 0x001,
	PLAYERSTATE_RUNNING = 0x002,
	PLAYERSTATE_LAND = 0x003,
	// Airborne
	PLAYERSTATE_JUMP = PLAYERSTATEFLAG_AIRBORNE | 0x001,
	PLAYERSTATE_FALL = PLAYERSTATEFLAG_AIRBORNE | 0x002
};

enum PlayerAnim
{
	// Note! this has to match the order of the animations in the meta
	PLAYERANIM_IDLE,
	PLAYERANIM_RUN,
	PLAYERANIM_JUMP,
	PLAYERANIM_LAND,
	PLAYERANIM_FALL,
	PLAYERANIM_PULSEATTACK
};

struct Player
{
	EntityHandle entityHandle;
	v3 vel;
	PlayerState state;
};

#if DEBUG_BUILD
struct DebugCube
{
	v3 pos;
	v3 color;
	v3 fw;
	v3 up;
	f32 scale;
};

struct DebugVertex
{
	v3 pos;
	v3 color;
};

struct DebugGeometryBuffer
{
	DeviceMesh deviceMesh;
	DeviceMesh cubeMesh;
	DeviceMesh cubePositionsBuffer;

	DebugVertex *triangleData;
	u32 triangleVertexCount;
	DebugVertex *lineData;
	u32 lineVertexCount;

	DebugCube debugCubes[2048];
	u32 debugCubeCount;
};

struct DebugContext
{
	DeviceProgram debugDrawProgram, debugCubesProgram;
	DebugGeometryBuffer debugGeometryBuffer;

	bool wireframeDebugDraws;
	bool drawAABBs;
	bool drawSupports;
	bool verboseCollisionLogging;

	// GJK EPA
	bool drawGJKPolytope;
	bool freezeGJKGeom;
	int gjkDrawStep;
	int gjkStepCount;
	DebugVertex *GJKSteps[64];
	int GJKStepCounts[64];
	v3 GJKNewPoint[64];

	static const int epaMaxSteps = 32;
	bool drawEPAPolytope;
	bool freezePolytopeGeom;
	int polytopeDrawStep;
	int epaStepCount;
	DebugVertex *polytopeSteps[epaMaxSteps];
	int polytopeStepCounts[epaMaxSteps];
	v3 epaNewPoint[epaMaxSteps];

	// Editor
	int selectedEntityIdx;
	int hoveredEntityIdx;
	DeviceProgram editorSelectedProgram;
};
#endif

struct GameState
{
	f32 timeMultiplier;

	v3 camPos;
	f32 camYaw;
	f32 camPitch;
	u32 entityCount;

	Entity entities[256];
	Entity *entityPointers[256];
	u8 entityGenerations[256];

	LevelGeometry levelGeometry;
	Player player;

	SkinnedMeshInstance skinnedMeshInstances[64];
	u32 skinnedMeshCount;

	ParticleSystem particleSystems[64];
	u32 particleSystemCount;

	// @Cleanup: move to some Render Device Context or something?
	DeviceProgram program, skinnedMeshProgram, particleSystemProgram;
	DeviceMesh particleMesh;
};

struct Button
{
	bool endedDown;
	bool changed;
};

struct Controller
{
	v2 leftStick;
	v2 rightStick;
	union
	{
		struct
		{
			Button up;
			Button down;
			Button left;
			Button right;
			Button jump;
			Button camUp;
			Button camDown;
			Button camLeft;
			Button camRight;

#if DEBUG_BUILD
			Button mouseLeft;
			Button mouseMiddle;
			Button mouseRight;

			Button debugUp;
			Button debugDown;
			Button debugLeft;
			Button debugRight;
#endif
		};
		Button b[16];
	};
	v2 mousePos;
};

struct PlatformContext
{
	PlatformCode *platformCode;
	Memory *memory;
#if USING_IMGUI
	ImGuiContext *imguiContext;
#endif
};

#define INIT_GAME_MODULE(name) void name(PlatformContext platformContext)
typedef INIT_GAME_MODULE(InitGameModule_t);
INIT_GAME_MODULE(InitGameModuleStub) { (void) platformContext; }

#define GAME_RESOURCE_POST_LOAD(name) bool name(Resource *resource, u8 *fileBuffer, \
		bool initialize)
typedef GAME_RESOURCE_POST_LOAD(GameResourcePostLoad_t);
GAME_RESOURCE_POST_LOAD(GameResourcePostLoadStub) { (void)resource, fileBuffer, initialize; return false; }

#define START_GAME(name) void name()
typedef START_GAME(StartGame_t);
START_GAME(StartGameStub) { }

#define UPDATE_AND_RENDER_GAME(name) void name(Controller *controller, f32 deltaTime)
typedef UPDATE_AND_RENDER_GAME(UpdateAndRenderGame_t);
UPDATE_AND_RENDER_GAME(UpdateAndRenderGameStub) { (void) controller, deltaTime; }
