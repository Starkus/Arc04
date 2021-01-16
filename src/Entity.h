struct EntityHandle
{
	// @Speed: use some bits of ID as generation number.
	u32 id;
	u8 generation;
};
const u32 ENTITY_ID_INVALID = U32_MAX;
const EntityHandle ENTITY_HANDLE_INVALID = { ENTITY_ID_INVALID, 0 };

inline bool operator==(const EntityHandle &a, const EntityHandle &b)
{
	return a.id == b.id && a.generation == b.generation;
}

inline bool operator!=(const EntityHandle &a, const EntityHandle &b)
{
	return a.id != b.id && a.generation != b.generation;
}

DECLARE_ARRAY(Transform);

enum ColliderType
{
	COLLIDER_CONVEX_HULL,
	COLLIDER_CUBE,
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
			f32 scale;
		} convexHull;
		struct
		{
			f32 radius;
			v3 offset;
		} cube;
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
	EntityHandle entityHandle @NoSerialize;
};
DECLARE_ARRAY(Collider);

struct MeshInstance
{
	EntityHandle entityHandle @NoSerialize;
	const Resource *meshRes;
};
DECLARE_ARRAY(MeshInstance);

struct SkinnedMeshInstance
{
	MeshInstance meshInstance @Using;

	i32 animationIdx @NoSerialize;
	f32 animationTime @NoSerialize;

	v3 jointTranslations[128] @NoSerialize;
	v4 jointRotations[128] @NoSerialize;
	v3 jointScales[128] @NoSerialize;
};
DECLARE_ARRAY(SkinnedMeshInstance);

struct Particle
{
	v3 pos;
	v4 color @Color;
	f32 size;
};

struct ParticleBookkeep
{
	f32 lifeTime;
	f32 duration;
	v3 velocity;
};

// @Speed: Separate params so we don't bring them into cache when trying to render.
struct ParticleSystem
{
	EntityHandle entityHandle @Hidden @NoSerialize;
	DeviceMesh deviceBuffer @Hidden @NoSerialize;
	f32 timer @NoSerialize;
	u8 atlasIdx;
	v3 offset;
	f32 spawnRate = 0.1f;
	f32 maxLife = 1.0f;
	f32 initialSize = 0.1f;
	f32 sizeSpread;
	f32 sizeDelta;
	v3 initialVel;
	v3 initialVelSpread;
	v3 acceleration;
	v4 initialColor = { 1, 1, 1, 1 } @Color;
	v4 colorSpread;
	v4 colorDelta;
	bool alive[256] @Hidden @NoSerialize; // @Improve
	ParticleBookkeep bookkeeps[256] @Hidden @NoSerialize;
	Particle particles[256] @Hidden @NoSerialize;
};
DECLARE_ARRAY(ParticleSystem);

#define MAX_ENTITIES 4096
struct EntityManager
{
	u8 entityGenerations[MAX_ENTITIES];
	u32 entityTransforms[MAX_ENTITIES];
	u32 entityMeshes[MAX_ENTITIES];
	u32 entitySkinnedMeshes[MAX_ENTITIES];
	u32 entityParticleSystems[MAX_ENTITIES];
	u32 entityColliders[MAX_ENTITIES];
};
