#version 330 core
in vec2 uv;
in vec4 color;
in vec2 screenPos;
out vec4 fragColor;

uniform sampler2D tex;
uniform sampler2D depthBuffer;

void main()
{
	fragColor = color;
	fragColor.a *= texture(tex, uv).r;

	float bufferDepth = texture2D(depthBuffer, screenPos).r;
	float fragDepth = gl_FragCoord.z;

	fragColor.a *= clamp((bufferDepth - fragDepth) * 5000.0, 0, 1);
}
