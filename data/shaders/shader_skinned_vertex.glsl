#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 nor;
layout (location = 3) in uvec4 indices;
layout (location = 4) in vec4 weights;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 joints[128];
uniform sampler2D texSampler;
out vec2 uv;
out vec3 normal;

void main()
{
	vec4 oldPos = vec4(pos, 1.0);
	vec4 newPos = (joints[indices.x] * oldPos) * weights.x;
	newPos += (joints[indices.y] * oldPos) * weights.y;
	newPos += (joints[indices.z] * oldPos) * weights.z;
	newPos += (joints[indices.w] * oldPos) * weights.w;

	vec4 oldNor = vec4(nor, 0.0);
	vec4 newNor = (joints[indices.x] * oldNor) * weights.x;
	newNor += (joints[indices.y] * oldNor) * weights.y;
	newNor += (joints[indices.z] * oldNor) * weights.z;
	newNor += (joints[indices.w] * oldNor) * weights.w;

	gl_Position = projection * view * model * vec4(newPos.xyz, 1.0);
	normal = (model * vec4(newNor.xyz, 0)).xyz;
	uv = vec2(inUv.x, -inUv.y);
}
