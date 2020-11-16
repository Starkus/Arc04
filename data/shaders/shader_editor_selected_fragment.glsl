#version 330 core
in float outTime;
out vec4 fragColor;

void main()
{
	float v = sin(outTime * 5) * 0.3 + 0.7;
	fragColor = vec4(v);
}
