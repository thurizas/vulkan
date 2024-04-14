#version 450 		// Use GLSL 4.5

layout(location=0) in vec3 pos;
layout(location=1) in vec3 col;

layout(binding=0) uniform UboVP {
	mat4 proj;
	mat4 view;
} uboVP;

layout(push_constant) uniform PushModel
{
	mat4 model;
} pushModel;

layout(location = 0) out vec3 fragCol;	// Output colour for vertex (location is required)



void main() 
{
	gl_Position = uboVP.proj*uboVP.view*pushModel.model *vec4(pos, 1.0);
	fragCol = col;
}