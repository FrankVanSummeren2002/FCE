#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vTexCoord;

layout (location = 0) out vec2 texCoord;

layout(set = 0, binding = 0) uniform  CameraBuffer{   
    vec4 view;
} cameraData;

struct ObjectData
{
	vec4 model;
	vec4 TextureData;
}; 

//all object matrices
layout(std140,set = 1, binding = 0) readonly buffer ObjectBuffer{   

	ObjectData objects[];
} objectBuffer;

layout( push_constant ) uniform constants
{
 vec4 Size;
} PushConstants;


void main() 
{	
	vec2 size = PushConstants.Size.xy * objectBuffer.objects[gl_InstanceIndex].TextureData.xy;
	float Rotation = objectBuffer.objects[gl_InstanceIndex].model.w;
	vec4 Position = vec4(objectBuffer.objects[gl_InstanceIndex].model.xy,0,0);

	float cs = cos(Rotation);
	float sn = sin(Rotation);

	vec2 vPosRotated = vPosition.xy * size;
	float x = vPosRotated.x;
	float y = vPosRotated.y;
	vPosRotated.x = x * cs - y * sn;
	vPosRotated.y = x * sn + y * cs;


	gl_Position = vec4(vPosRotated,1.0f, 1.0f) - cameraData.view + Position;
	texCoord = vTexCoord * PushConstants.Size.zw + objectBuffer.objects[gl_InstanceIndex].TextureData.zw;

}
