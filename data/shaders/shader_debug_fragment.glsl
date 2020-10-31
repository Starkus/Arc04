#version 330 core
in vec3 normal;
out vec4 fragColor;

void main()
{
	fragColor = vec4(normal, 0) * 0.5 + vec4(0.5);
}
