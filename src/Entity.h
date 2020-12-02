struct EntityHandle
{
	// @Speed: use some bits of ID as generation number.
	u32 id;
	u8 generation;
};
EntityHandle ENTITY_HANDLE_INVALID = { U32_MAX, 0 };

inline bool operator==(const EntityHandle &a, const EntityHandle &b)
{
	return a.id == b.id && a.generation == b.generation;
}

inline bool operator!=(const EntityHandle &a, const EntityHandle &b)
{
	return a.id != b.id && a.generation != b.generation;
}

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
	EntityHandle entityHandle;
};
DECLARE_ARRAY(Collider);

struct SkinnedMeshInstance
{
	EntityHandle entityHandle;
	const Resource *meshRes;
	i32 animationIdx;
	f32 animationTime;

	v3 jointTranslations[128];
	v4 jointRotations[128];
	v3 jointScales[128];
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
	EntityHandle entityHandle @Hidden;
	DeviceMesh deviceBuffer @Hidden;
	f32 timer;
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
	bool alive[256] @Hidden; // @Improve
	ParticleBookkeep bookkeeps[256] @Hidden;
	Particle particles[256] @Hidden;
};
DECLARE_ARRAY(ParticleSystem);

struct Entity
{
	Transform transform @Using;

	// @Todo: decide how to handle optional things.
	const Resource *mesh;
	SkinnedMeshInstance *skinnedMeshInstance;
	ParticleSystem *particleSystem;
	//Collider *collider;
};
DECLARE_ARRAY(Entity);

#define MAX_ENTITIES 256
struct EntityManager
{
	Array_Entity entities;
	Entity *entityPointers[MAX_ENTITIES];
	u8 entityGenerations[MAX_ENTITIES];

	Collider *entityColliders[MAX_ENTITIES];
};
