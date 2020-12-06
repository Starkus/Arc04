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
	PLAYERANIM_INVALID = -1,
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
	f32 speed;
};

enum JumperState
{
	JUMPERSTATE_IDLE,
	JUMPERSTATE_CHASING
};

struct Jumper
{
	EntityHandle entityHandle;
	JumperState state;
	EntityHandle target = ENTITY_HANDLE_INVALID;
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

// @Improve: this is garbage
enum PickingResult
{
	PICKING_NOTHING = 0,
	PICKING_ENTITY = 1,
	PICKING_GIZMO_ROT_X = 2,
	PICKING_GIZMO_ROT_Y = 3,
	PICKING_GIZMO_ROT_Z = 4,
	PICKING_GIZMO_X = 5,
	PICKING_GIZMO_Y = 6,
	PICKING_GIZMO_Z = 7,
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
	EntityHandle selectedEntity = ENTITY_HANDLE_INVALID;
	EntityHandle hoveredEntity;
	DeviceProgram editorSelectedProgram;
	DeviceProgram editorGizmoProgram;
};
#endif

struct GameState
{
	f32 timeMultiplier;

	v3 camPos;
	f32 camYaw;
	f32 camPitch;

	EntityManager entityManager @Using;

	LevelGeometry levelGeometry;
	Player player;

	Jumper jumper;

	Array_Transform transforms;
	Array_MeshInstance meshInstances;
	Array_SkinnedMeshInstance skinnedMeshInstances;
	Array_ParticleSystem particleSystems;
	Array_Collider colliders;

	// @Cleanup: move to some Render Device Context or something?
	mat4 invViewMatrix, viewMatrix, projMatrix;
	DeviceProgram program, skinnedMeshProgram, particleSystemProgram;
	DeviceMesh particleMesh;
	// Frame buffer
	DeviceFrameBuffer frameBuffer;
	DeviceTexture frameBufferColorTex, frameBufferDepthTex;
	DeviceProgram frameBufferProgram;
};
