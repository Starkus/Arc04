#version 330 core
layout (location = 0) in uint vertexNum;
layout (location = 1) in vec3 pos;
layout (location = 2) in vec4 inColor;
layout (location = 3) in float size;
uniform mat4 view;
uniform mat4 projection;
uniform int atlasIdx;
out vec2 uv;
out vec4 color;
out vec2 screenPos;

vec2 corners[] = vec2[4](
	vec2(-1, -1),
	vec2(-1,  1),
	vec2( 1, -1),
	vec2( 1,  1)
);

vec2 uvs[] = vec2[4](
	vec2(0, 1),
	vec2(0, 0),
	vec2(1, 1),
	vec2(1, 0)
);

void main()
{
	vec3 camRight = vec3(view[0][0], view[1][0], view[2][0]);
	vec3 camUp = vec3(view[0][1], view[1][1], view[2][1]);
	vec3 v = camRight * corners[vertexNum].x + camUp * corners[vertexNum].y;
	vec4 finalPos = vec4(v * size + pos, 1);
	gl_Position = projection * view * finalPos;

	int atlasX = atlasIdx % 4;
	int atlasY = atlasIdx / 4;
	uv = (uvs[vertexNum] + vec2(atlasX, atlasY)) * 0.25;

	color = inColor;

	screenPos = 0.5 * gl_Position.xy / gl_Position.w - vec2(0.5);
}
