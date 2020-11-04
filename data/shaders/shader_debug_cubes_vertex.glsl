#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 instPos;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 fw;
layout (location = 4) in vec3 up;
layout (location = 5) in float scale;
uniform mat4 view;
uniform mat4 projection;
out vec3 color;

void main()
{
	vec3 right = normalize(cross(fw, up));
	vec3 up2 = normalize(cross(fw, right));
	mat4 inst = mat4(
	  right.x, fw.x, up2.x, instPos.x,
	  right.y, fw.y, up2.y, instPos.y,
	  right.z, fw.z, up2.z, instPos.z,
	  0, 0, 0, 1
	);
	vec4 newPos = (vec4(pos * scale, 1) * inst);
	gl_Position = projection * view * newPos;
	color = inColor;
}
