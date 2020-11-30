#version 330 core
in vec2 uv;
in vec3 normal;
out vec4 fragColor;
uniform vec4 color;

void main()
{
	fragColor = color;
}
