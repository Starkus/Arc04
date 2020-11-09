#version 330 core
in vec2 uv;
in vec3 normal;
out vec4 fragColor;

uniform sampler2D texAlbedo;
uniform sampler2D texNormal;

void main()
{
	vec3 albedoMap = texture(texAlbedo, uv).rgb;
	vec3 normalMap = texture(texNormal, uv).rgb * 2 - vec3(1);
	vec3 tangent = cross(vec3(0,0,1), normal);
	vec3 bitangent = cross(normal, tangent);
	mat4 ntb = mat4(
		tangent.x, bitangent.x, normal.x, 0,
		tangent.y, bitangent.y, normal.y, 0,
		tangent.z, bitangent.z, normal.z, 0,
		0, 0, 0, 1
	);
	vec3 nor = (vec4(normalMap, 0) * ntb).rgb;
	vec3 lightDir = normalize(vec3(1, 0.7, 1.3));
	float light = dot(nor, lightDir);
	fragColor = vec4(albedoMap * (light * 0.5 + 0.5), 0);
}
