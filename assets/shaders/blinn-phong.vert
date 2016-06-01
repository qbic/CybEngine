#version 420 core

uniform mat4 u_projMatrix;
uniform mat4 u_modelViewMatrix;

in vec3 a_position;
in vec3 a_normal;
in vec2 a_texCoord0;

out VertexInfo
{
	vec3 position;
	vec3 normal;
	vec2 texCoord0;
} outVertex;

void main()
{
	gl_Position = u_projMatrix * u_modelViewMatrix * vec4(a_position, 1.0);
	outVertex.position = a_position;
	outVertex.normal = a_normal;
	outVertex.texCoord0 = a_texCoord0;
}