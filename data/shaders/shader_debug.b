       P      #version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 nor;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 normal;

void main()
{
	gl_Position = projection * view * model * vec4(pos, 1.0);
	normal = (model * vec4(nor, 0.0)).xyz;
}
 #version 330 core
in vec3 normal;
out vec4 fragColor;

void main()
{
	fragColor = vec4(normal, 0) * 0.5 + vec4(0.5);
}
 