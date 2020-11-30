#version 330 core
in vec2 uv;
in vec3 normal;
out vec4 fragColor;
uniform vec4 color;

void main()
{
	vec3 lightDir = normalize(vec3(1, 0.7, 1.3));
	float light = dot(normal, lightDir) * 0.5 + 0.5;
	fragColor = color * light;
}
