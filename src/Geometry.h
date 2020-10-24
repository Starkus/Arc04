struct Vertex
{
	v3 pos;
	v2 uv;
	v3 nor;
};

struct SkinnedVertex
{
	v3 pos;
	v2 uv;
	v3 nor;
	u16 indices[4];
	f32 weights[4];
};

struct Triangle
{
	union
	{
		struct
		{
			v3 a;
			v3 b;
			v3 c;
		};
		v3 corners[3];
	};
	v3 normal;
};

struct QuadTree
{
	v2 lowCorner;
	v2 highCorner;
	int cellsSide;
	u32 *offsets;
	Triangle *triangles;
};
