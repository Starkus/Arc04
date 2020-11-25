#version 330 core
layout (location = 0) in uint vertexNum;
out vec2 uv;

vec2 corners[] = vec2[4](
	vec2(-1, -1),
	vec2(-1,  1),
	vec2( 1, -1),
	vec2( 1,  1)
);

vec2 uvs[] = vec2[4](
	vec2(0, 0),
	vec2(0, 1),
	vec2(1, 0),
	vec2(1, 1)
);

void main()
{
	gl_Position = vec4(corners[vertexNum], 0, 1);
	uv = uvs[vertexNum];
}
