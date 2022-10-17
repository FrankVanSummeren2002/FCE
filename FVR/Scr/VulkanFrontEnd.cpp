#include "VulkanFrontEnd.h"
#include "vk_engine.h"
#include "imgui-docking/imgui.h"
#include "imgui-docking/backends/imgui_impl_vulkan.h"
#include "imgui-docking/backends/imgui_impl_glfw.h"
#include "GLFW/glfw3.h"

using namespace FVR;
FVR::VulkanRenderBackEnd RenderEngine;

void FVR::VulkanFrontEnd::Init()
{
	RenderEngine.Init();
}

void FVR::VulkanFrontEnd::EndFrame(glm::mat4 View, glm::mat4 ProjectionMat)
{
	RenderEngine.EndFrame(View,ProjectionMat);
}
void FVR::VulkanFrontEnd::EndFrame2D(glm::vec4 camPos)
{
	RenderEngine.EndFrame2D(camPos);
}

FVR::MeshHandle FVR::VulkanFrontEnd::GetMesh(const char* path)
{
	return RenderEngine.GetMesh(path);
}

QuadHandle FVR::VulkanFrontEnd::GetQuadSingleTexture(uint32_t width, uint32_t height)
{
	return RenderEngine.GetQuad(width, height, glm::vec2(0),glm::vec2(1), glm::vec2(1));
}
QuadHandle FVR::VulkanFrontEnd::GetQuadSpriteSheet(uint32_t width, uint32_t height, glm::vec2 BaseOffset,glm::vec2 TileSheetSize,glm::vec2 TileSize)
{
	return RenderEngine.GetQuad(width, height, BaseOffset,TileSheetSize,TileSize);
}

bool FVR::VulkanFrontEnd::FreeMesh(MeshHandle handle)
{
	return RenderEngine.UnloadMesh(handle);
}

FVR::TextureHandle FVR::VulkanFrontEnd::GetTexture(const char* vertShader, const char* fragShader, const char* textPath,bool Render3D)
{
	MaterialInfo info;
	info.fragmentPath = fragShader;
	info.vertexPath = vertShader;
	info.textureInfo = new TextureInfo();
	info.textureInfo->path = textPath;
	return RenderEngine.GetTexture(info,static_cast<PIPELINE_TYPE>(Render3D));
}

bool FVR::VulkanFrontEnd::FreeQuad(QuadHandle handle)
{
	return RenderEngine.UnloadQuad(handle);
}

bool FVR::VulkanFrontEnd::FreeTexture(TextureHandle handle)
{
	return RenderEngine.UnloadTexture(handle);
}

int FVR::VulkanFrontEnd::BeginFrame(bool DontKeepLastFrameData)
{
	return RenderEngine.BeginFrame(DontKeepLastFrameData);
}

void FVR::VulkanFrontEnd::DrawModel(glm::mat4 modelMat, MeshHandle meshHandle, TextureHandle textHandle)
{
	RenderEngine.DrawModelTextured(modelMat, meshHandle, textHandle);
}

void FVR::VulkanFrontEnd::DrawLine(glm::vec3 Begin, glm::vec3 End, glm::vec3 BeginColor,glm::vec3 EndColor,bool ConvertTo2D)
{
	if (ConvertTo2D)
	{
		Begin = ConvertPosition2D(Begin);
		End = ConvertPosition2D(End);
		Begin.z = 0;
		End.z = 0;
		
	}
	RenderEngine.DrawLine(Begin, End, BeginColor,EndColor);
}

void FVR::VulkanFrontEnd::DrawPoint(glm::vec3 Point, glm::vec3 Color, bool ConvertTo2D)
{
	RenderEngine.DrawPoint(Point, Color);
}

void FVR::VulkanFrontEnd::DrawQuad(glm::vec4 Position,glm::vec2 Scaling, QuadHandle quadHandle, TextureHandle textHandle,float Rotation)
{
	RenderEngine.DrawQuad(Position,Scaling, quadHandle, textHandle, Rotation);
}

void FVR::VulkanFrontEnd::DrawImguiViewPort()
{
	RenderEngine.DrawImguiViewPort();
}

GLFWwindow* FVR::VulkanFrontEnd::GetWindow()
{
	return RenderEngine.GetWindow();
}

void FVR::VulkanFrontEnd::CleanUp()
{
	RenderEngine.CleanUp();
}

glm::vec2 FVR::VulkanFrontEnd::GetScreenSize()
{
	return RenderEngine.WindowSize();
}

glm::vec2 FVR::VulkanFrontEnd::ConvertPosition2D(glm::vec2 pos)
{
	glm::vec2 size = RenderEngine.ScreenSize();
	return pos / size;
}

glm::vec3 FVR::VulkanFrontEnd::ConvertPosition2D(glm::vec3 pos)
{
	glm::vec3 size = glm::vec3(RenderEngine.ScreenSize(),1);
	return pos / size;
}

glm::vec4 FVR::VulkanFrontEnd::ConvertPosition2D(glm::vec4 pos)
{
	glm::vec4 size = glm::vec4(RenderEngine.ScreenSize(),1, 1);
	return pos / size;
}

bool FVR::VulkanFrontEnd::ShouldCloseWindow()
{
	return RenderEngine.ShouldCloseWindow();
}