#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 inColor;
uniform mat4 view;
uniform mat4 projection;
out vec3 color;

void main()
{
	gl_Position = projection * view * vec4(pos, 1);
	color = inColor;
}
