#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec4 vColor;


layout (location = 0) out vec3 outColor;


layout(set = 0, binding = 0) uniform  CameraBuffer
{   
	vec4 view;
} cameraData;

struct ObjectData
{
	vec4 model;
}; 

//all object matrices
layout(std140,set = 1, binding = 0) readonly buffer ObjectBuffer
{   
	ObjectData objects[];
} objectBuffer;


void main() 
{	
	gl_Position = vec4(vPosition, 1.0f) - cameraData.view;
	outColor = vColor.xyz;
}
