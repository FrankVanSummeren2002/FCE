// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once
//we will add our main reusable types here
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "glm.hpp"
#include "optional"
#include "vector"
#include "iostream"
#include "Handles.h"
#include "memory"
struct Mesh;
struct Quad;

namespace FVR
{
	struct AllocatedBuffer
	{
		VkBuffer mBuffer;
		VmaAllocation mAllocation;
	};

	struct AllocatedImage
	{
		VkImage mImage;
		VmaAllocation mAllocation;
	};

	struct GPUCameraData
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewproj;
	};

	struct GPUCamera2DData
	{
		glm::vec4 Pos;
	};

	struct GPUSceneData
	{
		glm::vec4 fogColor; // w is for exponent
		glm::vec4 fogDistances; //x for min, y for max, zw unused.
		glm::vec4 ambientColor;
		glm::vec4 sunlightDirection; //w for sun power
		glm::vec4 sunlightColor;
	};

	struct UploadContext
	{
		VkFence mUploadFence;
		VkCommandPool mCommandPool;
		VkCommandBuffer mCommandBuffer;
	};

	struct FrameData
	{
		VkSemaphore mPresentSemaphore, mRenderSemaphore;
		VkFence mRenderFence;

		VkCommandPool mCommandPool;
		VkCommandBuffer mMainCommandBuffer;

		AllocatedBuffer mCameraBuffer;
		VkDescriptorSet mGlobalDescriptor;

		AllocatedBuffer mObjectBuffer;
		VkDescriptorSet mObjectDescriptor;

		AllocatedBuffer mObject2DBuffer;
		VkDescriptorSet mObject2DDescriptor;

		AllocatedBuffer mCamera2DBuffer;
		VkDescriptorSet mGlobal2DDescriptor;

		uint32_t mSwapChainIndex;

		AllocatedBuffer mPointBuffer;
	};

	struct GPUObjectData
	{
		glm::mat4 modelMatrix;
	};
	struct GPUObject2DData
	{
		glm::vec4 Position;
		glm::vec4 Scale;
	};

	struct TextureInfo
	{
		const char* path;
		VkFilter filter = VK_FILTER_NEAREST;
	};

	struct MaterialInfo
	{
		const char* vertexPath = "";
		const char* fragmentPath = "";

		TextureInfo* textureInfo = nullptr;
	};
	struct Texture
	{
		const char* path;
		AllocatedImage mImage;
		VkImageView mImageView;
		VkSampler mImageSampler;
		uint32_t miplevels;
	};

	struct Material
	{
		bool pipelineBuild{false};
		MaterialInfo info;
		VkDescriptorSet textureSet{ VK_NULL_HANDLE }; //texture defaulted to null
		std::shared_ptr<VkPipeline> pipeline;
		std::shared_ptr<VkPipelineLayout> pipelineLayout;
		TextureHandle handle;
		Texture text{};
		uint32_t type = 0;
		~Material()
		{
			if (pipeline != nullptr)
				pipeline.reset();
			if (pipelineLayout != nullptr)
				pipelineLayout.reset();
		}
	};

	struct RenderObject
	{
		Mesh* mesh;

		Material* material;

		glm::mat4 transformMatrix;
	};

	struct Render2DObject
	{
		QuadHandle quadHandle;
		TextureHandle matHandle;

		Quad* quad;
		Material* material;

		std::vector<glm::vec4> position;
		std::vector<glm::vec2> Scaling;
		std::vector<float> Rotation;
	};


	struct MeshPushConstants
	{
		glm::vec4 data;
		glm::mat4 render_matrix;
	};

	struct QuadPushConstant
	{
		glm::vec4 Size;
	};

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> mGraphicsFamily;
		std::optional<uint32_t> mPresentFamily;
		bool IsComplete() { return mGraphicsFamily.has_value() && mPresentFamily.has_value(); };
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
}