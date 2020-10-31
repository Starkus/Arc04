#version 330 core
in vec3 normal;
out vec4 fragColor;

void main()
{
	vec3 lightDir = normalize(vec3(1, 0.7, 1.3));
	float light = dot(normal, lightDir);
	fragColor = vec4(light * 0.6 + 0.4);
	//fragColor = vec4(normal, 0) * 0.5 + vec4(0.5);
}
