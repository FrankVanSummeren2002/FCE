#pragma once
#include "vk_types.h"

#include "iostream"
#include "vector"
#include <functional>
#include <deque>
#include "vk_mesh.h"
#include "glm.hpp"

namespace FVR
{
	struct DeletionQueue
	{
		std::deque<std::function<void()>> mDeletors;

		void push_function(std::function<void()>&& function) 
		{
			mDeletors.push_back(function);
		}

		void flush() 
		{
			// reverse iterate the deletion queue to execute all the functions
			for (auto it = mDeletors.rbegin(); it != mDeletors.rend(); it++) {
				(*it)(); //call the function
			}

			mDeletors.clear();
		}
	};

	class PipelineBuilder
	{
	public:

		std::vector<VkPipelineShaderStageCreateInfo> mShaderStages;
		VkPipelineVertexInputStateCreateInfo mVertexInputInfo{};
		VkPipelineInputAssemblyStateCreateInfo mInputAssembly{};
		VkViewport mViewport{};
		VkRect2D mScissor{};
		VkPipelineRasterizationStateCreateInfo mRasterizer{};
		VkPipelineColorBlendAttachmentState mColorBlendAttachment{};
		VkPipelineMultisampleStateCreateInfo mMultisampling{};
		VkPipelineLayout mPipelineLayout{};
		VkPipelineDepthStencilStateCreateInfo mDepthStencil{};

		VkPipeline BuildPipeline(VkDevice device, VkRenderPass pass);
	};


}