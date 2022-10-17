// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
struct GLFWwindow;

namespace FVR
{
	namespace vkInit
	{
		VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);

		VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo();

		VkPipelineInputAssemblyStateCreateInfo InputAssemblyCreateInfo(VkPrimitiveTopology topology);

		VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo(VkPolygonMode polygonMode);

		VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);

		VkPipelineMultisampleStateCreateInfo MultisamplingStateCreateInfo(VkSampleCountFlagBits	msaa);

		VkPipelineColorBlendAttachmentState ColorBlendAttachmentState(VkBool32 blendable);
		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo();

		VkRenderPassBeginInfo RenderPassBeginInfo(VkRenderPass renderPass, VkExtent2D windowExtent, VkFramebuffer framebuffer);

		VkDescriptorSetLayoutBinding DescriptorsetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);

		VkWriteDescriptorSet WriteDescriptorBuffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding);

		VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags = 0);

		VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

		VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, uint32_t mipLevels, VkSampleCountFlagBits MSAA);

		VkImageViewCreateInfo ImageCreateViews(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags,uint32_t mipLevel);

		VkPipelineDepthStencilStateCreateInfo DepthStencilCreateInfo(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp);

		VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
		VkSubmitInfo SubmitInfo(VkCommandBuffer* cmd);

		VkSamplerCreateInfo SamplerCreateInfo(VkFilter filters, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,uint32_t mipmap = 1);
		VkWriteDescriptorSet WriteDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding);

		VkSwapchainKHR CreateSwapChain(VkSurfaceKHR Surface, VkPhysicalDevice PhysicalDevice, VkDevice Device, GLFWwindow* Window, VkFormat& SwapChainImageFormat, VkExtent2D& extend, std::vector<VkImage>& Images,int& MinImageCount);
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels,VkDevice device);
		VkInstance CreateInstance(const char* AppName, const char* EngineName, bool EnableValidationLayers, std::vector<const char*> validationLayers,const VkAllocationCallbacks* pAlloc);
		VkDebugUtilsMessengerEXT CreateDebugMessenger(VkInstance instance);
		VkPhysicalDevice PickPhysicalDevice(VkInstance Instance, VkSurfaceKHR surface, VkSampleCountFlagBits &MsaaSamples);
		VkDevice CreateLogicalDevice(VkPhysicalDevice physical, VkSurfaceKHR Surface, bool EnableValidation, std::vector<const char*> validationLayers, VkQueue& GraphicsQueue, uint32_t& GraphicsFamily);
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice PhysicalDevice);
	}
}


