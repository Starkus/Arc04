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

struct Entity
{
	v3 pos;
	v3 fw;
	const Resource *mesh;
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
	v3 fw;
	v3 up;
	f32 scale;
};

struct DebugGeometryBuffer
{
	DeviceMesh deviceMesh;

	Vertex *triangleData;
	u32 triangleVertexCount;
	Vertex *lineData;
	u32 lineVertexCount;

	DebugCube debugCubes[2048];
	u32 debugCubeCount;
};
#endif

struct GameState
{
	v3 camPos;
	f32 camYaw;
	f32 camPitch;
	u32 entityCount;
	Entity entities[256];
	LevelGeometry levelGeometry;
	Player player;

	int animationIdx;
	f32 animationTime;

	// @Cleanup: move to some Render Device Context or something?
	DeviceProgram program, skinnedMeshProgram;
	DeviceMesh anvilMesh, cubeMesh, sphereMesh, cylinderMesh, capsuleMesh;
	SkinnedMesh skinnedMesh;

	// Debug
	DeviceProgram debugDrawProgram;
	DebugGeometryBuffer debugGeometryBuffer;
	int currentPolytopeStep;
};
