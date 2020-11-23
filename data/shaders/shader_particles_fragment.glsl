#version 330 core
in vec2 uv;
in vec4 color;
out vec4 fragColor;

uniform sampler2D tex;

void main()
{
	fragColor = color;
	fragColor.a *= texture(tex, uv).r;
}
