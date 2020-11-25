#version 330 core
in vec2 uv;
out vec4 fragColor;

uniform sampler2D texColor;
uniform sampler2D texDepth;

void main()
{
	fragColor = texture(texColor, uv);
}
