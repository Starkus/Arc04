#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 nor;
layout (location = 3) in uvec4 indices;
layout (location = 4) in vec4 weights;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 jointTranslations[128];
uniform vec4 jointRotations[128];
uniform vec3 jointScales[128];
uniform sampler2D texSampler;
out vec2 uv;
out vec3 normal;

mat4 Mat4Compose(vec3 t, vec4 r, vec3 s)
{
	mat4 m;

	m[0][0] = (1 - 2*r.y*r.y - 2*r.z*r.z) * s.x;
	m[0][1] = (2*r.x*r.y + 2*r.z*r.w) * s.x;
	m[0][2] = (2*r.x*r.z - 2*r.y*r.w) * s.x;
	m[0][3] = 0;

	m[1][0] = (2*r.x*r.y - 2*r.z*r.w) * s.y;
	m[1][1] = (1 - 2*r.x*r.x - 2*r.z*r.z) * s.y;
	m[1][2] = (2*r.y*r.z + 2*r.x*r.w) * s.y;
	m[1][3] = 0;

	m[2][0] = (2*r.x*r.z + 2*r.y*r.w) * s.z;
	m[2][1] = (2*r.y*r.z - 2*r.x*r.w) * s.z;
	m[2][2] = (1 - 2*r.x*r.x - 2*r.y*r.y) * s.z;
	m[2][3] = 0;

	m[3][0] = t.x;
	m[3][1] = t.y;
	m[3][2] = t.z;
	m[3][3] = 1;

	return m;
}

void main()
{
	mat4 j0 = Mat4Compose(jointTranslations[indices.x], jointRotations[indices.x], jointScales[indices.x]);
	mat4 j1 = Mat4Compose(jointTranslations[indices.y], jointRotations[indices.y], jointScales[indices.y]);
	mat4 j2 = Mat4Compose(jointTranslations[indices.z], jointRotations[indices.z], jointScales[indices.z]);
	mat4 j3 = Mat4Compose(jointTranslations[indices.w], jointRotations[indices.w], jointScales[indices.w]);

	vec4 oldPos = vec4(pos, 1.0);
	vec4 newPos = (j0 * oldPos) * weights.x;
	newPos += (j1 * oldPos) * weights.y;
	newPos += (j2 * oldPos) * weights.z;
	newPos += (j3 * oldPos) * weights.w;

	vec4 oldNor = vec4(nor, 0.0);
	vec4 newNor = (j0[indices.x] * oldNor) * weights.x;
	newNor += (j1 * oldNor) * weights.y;
	newNor += (j2 * oldNor) * weights.z;
	newNor += (j3 * oldNor) * weights.w;

	gl_Position = projection * view * model * vec4(newPos.xyz, 1.0);
	normal = (model * vec4(newNor.xyz, 0)).xyz;
	uv = vec2(inUv.x, -inUv.y);
}
