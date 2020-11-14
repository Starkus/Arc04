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
			const Resource *pointCloud;
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

struct SkinnedMeshInstance
{
	int entityHandle;
	const Resource *meshRes;
	int animationIdx;
	f32 animationTime;
};

struct Entity
{
	v3 pos;
	v3 fw;

	// @Todo: decide how to handle optional things.
	const Resource *mesh;
	SkinnedMeshInstance *skinnedMeshInstance;

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
	Entity *entity;
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
	LevelGeometry levelGeometry;
	Player player;

	SkinnedMeshInstance skinnedMeshInstances[64];
	u32 skinnedMeshCount;
	//int animationIdx;
	//f32 animationTime;

	// @Cleanup: move to some Render Device Context or something?
	DeviceProgram program, skinnedMeshProgram;
	DeviceMesh anvilMesh, cubeMesh, sphereMesh, cylinderMesh, capsuleMesh;
	SkinnedMesh skinnedMesh;
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
			Button debugUp;
			Button debugDown;
			Button debugLeft;
			Button debugRight;
#endif
		};
		Button b[13];
	};
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
