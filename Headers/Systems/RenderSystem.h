#pragma once
#include "Header/Handles.h"

namespace FCE
{
	class RenderSystem
	{
		static void Add2DComponent(entt::entity entity, FVR::QuadHandle quadHandle, FVR::TextureHandle textHandle, glm::vec2 Size, glm::vec2 repetition);
	public:
		static void Init();
		static void Add3DComponent(entt::entity, const char* meshPath, const char* texturePath, const char* vertShaderPath, const char* fragShaderPath,glm::vec3 Offset = glm::vec3(0));
		static void Add2DComponentSingleTexture(entt::entity,uint32_t width,uint32_t height, glm::vec2 repetition, const char* texturePath, const char* vertShaderPath, const char* fragShaderPath);
		static void Add2DComponentSpriteSheet(entt::entity, uint32_t width, uint32_t height,glm::vec2 BaseOffset,glm::vec2 sheetSize,glm::vec2 tileSize, glm::vec2 repetition, const char* texturePath, const char* vertShaderPath, const char* fragShaderPath);
		static void Add3DComponent(entt::entity entity, FVR::MeshHandle meshHandle, FVR::TextureHandle textHandle, glm::vec3 Offset = glm::vec3(0));
		static void Update();
	};
}

