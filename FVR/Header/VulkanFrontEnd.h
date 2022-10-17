#pragma once
#include "glm.hpp"
#include "vector"
#include "Handles.h"
struct GLFWwindow;
namespace FVR
{
	class VulkanFrontEnd
	{
	public:
	
		//Returns wheter the project should close
		//a boolean for deleting the last frames data
		//you usually want this to be set to true unless you have paused the updates for your game
		//but still want to display the last frame
		int BeginFrame(bool DontKeepLastFrameData = true);
		void EndFrame(glm::mat4 View,glm::mat4 ProjectionMat);
		void EndFrame2D(glm::vec4 camPos);
		void Init();
		void CleanUp();

		MeshHandle GetMesh(const char* path);
		QuadHandle GetQuadSingleTexture(uint32_t width, uint32_t height);
		QuadHandle GetQuadSpriteSheet(uint32_t width, uint32_t height, glm::vec2 BaseOffset, glm::vec2 TileSheetSize,glm::vec2 TileSize);
		TextureHandle GetTexture(const char* vertShader,const char* fragShader,const char* textPath, bool Render3D);

		bool FreeQuad(QuadHandle);
		bool FreeMesh(MeshHandle);
		bool FreeTexture(TextureHandle);

		void DrawModel(glm::mat4 modelMat,MeshHandle meshHandle,TextureHandle textHandle);
		void DrawLine(glm::vec3 Begin, glm::vec3 End, glm::vec3 BeginColor,glm::vec3 EndColor, bool ConvertTo2D = false);
		void DrawPoint(glm::vec3 Point, glm::vec3 Color,bool ConvertTo2D = false);
		void DrawQuad(glm::vec4 Position,glm::vec2 Scaling,QuadHandle quadHandle,TextureHandle textHandle,float Rotation);
		void DrawImguiViewPort();

		GLFWwindow* GetWindow();
		glm::vec2 GetScreenSize();
		glm::vec2 ConvertPosition2D(glm::vec2 pos);
		glm::vec3 ConvertPosition2D(glm::vec3 pos);
		glm::vec4 ConvertPosition2D(glm::vec4 pos);

		bool ShouldCloseWindow();
	};
}

