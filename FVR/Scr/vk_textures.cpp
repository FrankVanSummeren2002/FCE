#include "vk_textures.h"
#include <iostream>
#include <vk_initializers.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool FVR::vkUtil::LoadImageFromFile(FVR::VulkanRenderBackEnd& engine, const char* file, AllocatedImage& outImage,uint32_t& MaxMipLevel)
{
	int texWidth, texHeight, texChannels;
	texWidth = 10;
	texHeight = 10;

	stbi_uc* pixels = stbi_load(file, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		std::cout << "Failed to load texture file " << file << std::endl;
		return false;
	}
	
	MaxMipLevel = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	void* pixel_ptr = pixels;
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	//the format R8G8B8A8 matches exactly with the pixels loaded from stb_image lib
	VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB;

	//allocate temporary buffer for holding texture data to upload
	AllocatedBuffer stagingBuffer = engine.CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	//copy data to buffer
	void* data;
	vmaMapMemory(engine.mAllocator, stagingBuffer.mAllocation, &data);

	memcpy(data, pixel_ptr, static_cast<size_t>(imageSize));

	vmaUnmapMemory(engine.mAllocator, stagingBuffer.mAllocation);
	//we no longer need the loaded data, so we can free the pixels as they are now in the staging buffer
	stbi_image_free(pixels);


	VkExtent3D imageExtent;
	imageExtent.width = static_cast<uint32_t>(texWidth);
	imageExtent.height = static_cast<uint32_t>(texHeight);
	imageExtent.depth = 1;

	VkImageCreateInfo dimg_info = vkInit::ImageCreateInfo(image_format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, imageExtent,MaxMipLevel,VK_SAMPLE_COUNT_1_BIT);

	VmaAllocationCreateInfo dimg_allocinfo = {};
	dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	//allocate and create the image
	vmaCreateImage(engine.mAllocator, &dimg_info, &dimg_allocinfo, &outImage.mImage, &outImage.mAllocation, nullptr);

	engine.ImmediateSubmit([&](VkCommandBuffer cmd) 
	{
		VkImageSubresourceRange range;
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.levelCount = MaxMipLevel;
		range.baseArrayLayer = 0;
		range.layerCount = 1;

		VkImageMemoryBarrier imageBarrier_toTransfer = {};
		imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

		imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier_toTransfer.image = outImage.mImage;
		imageBarrier_toTransfer.subresourceRange = range;

		imageBarrier_toTransfer.srcAccessMask = 0;
		imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		//barrier the image into the transfer-receive layout
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);
	});


	//make the image readable for the shader
	engine.ImmediateSubmit([&](VkCommandBuffer cmd)
		{
			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;

			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = imageExtent;

			//copy the buffer into the image
			vkCmdCopyBufferToImage(cmd, stagingBuffer.mBuffer, outImage.mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
		});

	//generate the mipmaps
	engine.ImmediateSubmit([&](VkCommandBuffer cmd)
	{
		//check if mipmaps are allowed
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(engine.mChosenGPU, VK_FORMAT_R8G8B8A8_SRGB, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		{
			std::cout << ("texture image format does not support linear blitting!") << std::endl;
			MaxMipLevel = 1;
		}
		//generate mip levels
		else
		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = outImage.mImage;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.subresourceRange.levelCount = 1;

			int32_t mipWidth = texWidth;
			int32_t mipHeight = texHeight;

			for (uint32_t i = 1; i < MaxMipLevel; i++)
			{
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				vkCmdPipelineBarrier(cmd,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				VkImageBlit blit{};
				blit.srcOffsets[0] = { 0, 0, 0 };
				blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
				blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = 0;
				blit.srcSubresource.layerCount = 1;
				blit.dstOffsets[0] = { 0, 0, 0 };
				blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
				blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.dstSubresource.mipLevel = i;
				blit.dstSubresource.baseArrayLayer = 0;
				blit.dstSubresource.layerCount = 1;

				vkCmdBlitImage(cmd, outImage.mImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					outImage.mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &blit,
					VK_FILTER_LINEAR);

				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(cmd,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				if (mipWidth > 1) mipWidth /= 2;
				if (mipHeight > 1) mipHeight /= 2;
			}

			barrier.subresourceRange.baseMipLevel = MaxMipLevel - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);
		}
	});


	//engine.mDeletionQueue.push_function([=]() {

	//	vmaDestroyImage(engine.mAllocator, outImage.mImage, outImage.mAllocation);
	//	});

	vmaDestroyBuffer(engine.mAllocator, stagingBuffer.mBuffer, stagingBuffer.mAllocation);

	std::cout << "Texture loaded successfully " << file << std::endl;

	
	return true;
}
