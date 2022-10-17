#pragma once

#include <vk_types.h>
#include <vk_engine.h>

namespace FVR
{
	namespace vkUtil 
	{

		bool LoadImageFromFile(FVR::VulkanRenderBackEnd& engine, const char* file, AllocatedImage& outImage, uint32_t& MaxMipLevel);

	}
}