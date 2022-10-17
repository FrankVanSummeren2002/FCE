
#include "vk_engine.h"
#include "vk_types.h"
#include "vk_initializers.h"
#include "vk_textures.h"

#include "imgui-docking/imgui.h"
#include "imgui-docking/backends/imgui_impl_vulkan.h"
#include "imgui-docking/backends/imgui_impl_glfw.h"

#include <fstream>
#include <filesystem>
#include "array"

#include <gtx/transform.hpp>

#include "vk_DescriptorBuilder.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"



#define VK_CHECK(x)																		\
	do																					\
	{																					\
		VkResult err = x;																\
		if (err)																		\
		{																				\
			std::cout << std::endl << "Detected Vulkan error: " << err << std::endl;	\
			abort();																	\
		}																				\
	} while (0)

const int MAX_OBJECTS = 10000;

#ifdef NDEBUG
const bool EnableValidationLayers = false;
#else
const bool EnableValidationLayers = true;
#endif

static void framebufferResizeCallback(GLFWwindow * window, int width, int height) 
{
	auto app = reinterpret_cast<FVR::VulkanRenderBackEnd*>(glfwGetWindowUserPointer(window));
	app->mNeedsResizing = true;
}

FVR::AllocatedBuffer FVR::VulkanRenderBackEnd::CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
	//allocate vertex buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;

	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;


	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;

	AllocatedBuffer newBuffer;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(mAllocator, &bufferInfo, &vmaallocInfo,
		&newBuffer.mBuffer,
		&newBuffer.mAllocation,
		nullptr));

	return newBuffer;
}

void FVR::VulkanRenderBackEnd::InitImguiRenderPass()
{
	VkAttachmentDescription attachment = {};
	attachment.format = mSwapChainImageFormat;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment = {};
	color_attachment.attachment = 0;
	color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0; // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 1;
	info.pAttachments = &attachment;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dependency;

	VK_CHECK(vkCreateRenderPass(mDevice, &info, nullptr, &mImGuiRenderPass));
	mSwapChainDeletionQueue.push_function([=]() 
		{
			vkDestroyRenderPass(mDevice, mImGuiRenderPass, nullptr);
		});
}
void FVR::VulkanRenderBackEnd::InitImguiCommandPool()
{
	VkCommandPoolCreateInfo ImguiCommandPool = vkInit::CommandPoolCreateInfo(mGraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	VK_CHECK(vkCreateCommandPool(mDevice, &ImguiCommandPool, nullptr, &mImGuiCommandPool));
	mDeletionQueue.push_function([=]()
		{
			vkDestroyCommandPool(mDevice, mImGuiCommandPool, nullptr);
		});

}
void FVR::VulkanRenderBackEnd::InitImguiCommandBuffers()
{
	mImGuiCommandBuffers.resize(mSwapChainImageViews.size());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = mImGuiCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)mImGuiCommandBuffers.size();

	VK_CHECK(vkAllocateCommandBuffers(mDevice, &allocInfo, mImGuiCommandBuffers.data()));
	

}
void FVR::VulkanRenderBackEnd::InitImguiFrameBuffer()
{
	mImGuiFramebuffers.resize(mSwapChainImageViews.size());

	VkImageView attachment[1];
	VkFramebufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.renderPass = mImGuiRenderPass;
	info.attachmentCount = 1;
	info.pAttachments = attachment;
	info.width = mWindowExtent.width;
	info.height = mWindowExtent.height;
	info.layers = 1;
	for (uint32_t i = 0; i < mSwapChainImageViews.size(); i++)
	{
		attachment[0] = mSwapChainImageViews[i];
		VK_CHECK(vkCreateFramebuffer(mDevice, &info, nullptr, &mImGuiFramebuffers[i]));
		mSwapChainDeletionQueue.push_function([=]() 
			{
				vkDestroyFramebuffer(mDevice, mImGuiFramebuffers[i], nullptr);
			});
		
	}
	
}
void FVR::VulkanRenderBackEnd::InitImguiSampler()
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;

	VK_CHECK(vkCreateSampler(mDevice, &samplerInfo, nullptr, &mViewportSampler));

	mSwapChainDeletionQueue.push_function([=]() 
		{
			vkDestroySampler(mDevice, mViewportSampler, nullptr);
		});
}
void FVR::VulkanRenderBackEnd::InitImguiDescriptor()
{
	mDset.resize(mViewportImageViews.size());
	for (uint32_t i = 0; i < mViewportImageViews.size(); i++)
		mDset[i] = ImGui_ImplVulkan_AddTexture(mViewportSampler, mViewportImageViews[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
void FVR::VulkanRenderBackEnd::InitImgui()
{
	//1: create descriptor pool for IMGUI
	// the size of the pool is very oversize, but it's copied from imgui demo itself.
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;


	VK_CHECK(vkCreateDescriptorPool(mDevice, &pool_info, nullptr, &mImguiPool));

	//this initializes imgui for glfw
	ImGui_ImplGlfw_InitForVulkan(mWindow, true);

	//this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = mInstance;
	init_info.PhysicalDevice = mChosenGPU;
	init_info.Device = mDevice;
	init_info.Queue = mGraphicsQueue;
	init_info.DescriptorPool = mImguiPool;
	init_info.MinImageCount = mMinImageCount;
	init_info.ImageCount = mMinImageCount;

	if (EnableValidationLayers)
	{
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		ImGui_ImplVulkan_Init(&init_info, mImGuiRenderPass);
	}
	else
	{
		init_info.MSAASamples = mMaxMsaa;
		ImGui_ImplVulkan_Init(&init_info, mRenderPass);
	}
	/*
	mImguiWindow = new ImGui_ImplVulkanH_Window();
	mImguiWindow->Surface = mSurface;
	mImguiWindow->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(mChosenGPU, mSurface, &mSwapChainImageFormat, 1, VK_COLORSPACE_SRGB_NONLINEAR_KHR);
	mImguiWindow->PresentMode = VK_PRESENT_MODE_FIFO_KHR;
	*/
	//execute a gpu command to upload imgui font textures
	ImmediateSubmit([&](VkCommandBuffer cmd)
		{
			ImGui_ImplVulkan_CreateFontsTexture(cmd);
		});

	//clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	//add the destroy the imgui created structures
	mDeletionQueue.push_function([=]()
		{
			vkDestroyDescriptorPool(mDevice, mImguiPool, nullptr);
			ImGui_ImplVulkan_Shutdown();
		});

	ImGuiIO& io = ImGui::GetIO();
}

void FVR::VulkanRenderBackEnd::InitQuadIndexBuffer()
{
	std::vector<uint16_t> indices = { 0,1,2,2,3,0 };
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
	//allocate staging buffer
	VkBufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.pNext = nullptr;

	stagingBufferInfo.size = bufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	//let the VMA library know that this data should be on CPU RAM
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	AllocatedBuffer stagingBuffer;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(mAllocator, &stagingBufferInfo, &vmaallocInfo,
		&stagingBuffer.mBuffer,
		&stagingBuffer.mAllocation,
		nullptr));

	//copy vertex data
	void* data;
	vmaMapMemory(mAllocator, stagingBuffer.mAllocation, &data);

	memcpy(data, indices.data(), (size_t)bufferSize);

	vmaUnmapMemory(mAllocator, stagingBuffer.mAllocation);


	//allocate vertex buffer
	VkBufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.pNext = nullptr;
	//this is the total size, in bytes, of the buffer we are allocating
	vertexBufferInfo.size = bufferSize;
	//this buffer is going to be used as a Vertex Buffer
	vertexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	//let the VMA library know that this data should be GPU native
	vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(mAllocator, &vertexBufferInfo, &vmaallocInfo,
		&mQuadIndexBuffer.mBuffer,
		&mQuadIndexBuffer.mAllocation,
		nullptr));

	ImmediateSubmit([=](VkCommandBuffer cmd)
		{
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer.mBuffer, mQuadIndexBuffer.mBuffer, 1, &copy);
		});

	mDeletionQueue.push_function([=]()
		{
			vmaDestroyBuffer(mAllocator, mQuadIndexBuffer.mBuffer, mQuadIndexBuffer.mAllocation);
		});

	vmaDestroyBuffer(mAllocator, stagingBuffer.mBuffer, stagingBuffer.mAllocation);
}

void FVR::VulkanRenderBackEnd::InitDebugPipeline()
{
	PipelineBuilder pipelineBuilder;

	VkShaderModule FragShader;
	if (!LoadShaderModule("Shaders/3D/DebugLine.frag.spv", &FragShader))
		std::cout << "Error when building fragment shader for the debug Lines " << std::endl;

	VkShaderModule VertexShader;
	if (!LoadShaderModule("Shaders/3D/DebugLine.vert.spv", &VertexShader))
		std::cout << "Error when building the  vertex shader debug Lines "  << std::endl;

	VkShaderModule PointFragShader;
	if (!LoadShaderModule("Shaders/3D/DebugPoint.frag.spv", &PointFragShader))
		std::cout << "Error when building fragment shader for the debug Points " << std::endl;

	VkShaderModule PointVertexShader;
	if (!LoadShaderModule("Shaders/3D/DebugPoint.vert.spv", &PointVertexShader))
		std::cout << "Error when building the  vertex shader debug Points " << std::endl;

	VkShaderModule FragShader2D;
	if (!LoadShaderModule("Shaders/2D/DebugLine2D.frag.spv", &FragShader2D))
		std::cout << "Error when building fragment shader for the debug Lines " << std::endl;

	VkShaderModule VertexShader2D;
	if (!LoadShaderModule("Shaders/2D/DebugLine2D.vert.spv", &VertexShader2D))
		std::cout << "Error when building the  vertex shader debug Lines " << std::endl;

	VkShaderModule PointFragShader2D;
	if (!LoadShaderModule("Shaders/2D/DebugPoint2D.frag.spv", &PointFragShader2D))
		std::cout << "Error when building fragment shader for the debug Points " << std::endl;

	VkShaderModule PointVertexShader2D;
	if (!LoadShaderModule("Shaders/2D/DebugPoint2D.vert.spv", &PointVertexShader2D))
		std::cout << "Error when building the  vertex shader debug Points " << std::endl;

	pipelineBuilder.mDepthStencil = vkInit::DepthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

	//vertex input controls how to read vertices from vertex buffers. We aren't using it yet
	pipelineBuilder.mVertexInputInfo = vkInit::VertexInputStateCreateInfo();

	//input assembly is the configuration for drawing triangle lists, strips, or individual points.
	//we are just going to draw triangle list
	pipelineBuilder.mInputAssembly = vkInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

	//build viewport and scissor from the swapchain extents
	pipelineBuilder.mViewport.x = 0.0f;
	pipelineBuilder.mViewport.y = 0.0f;
	pipelineBuilder.mViewport.width = (float)mWindowExtent.width;
	pipelineBuilder.mViewport.height = (float)mWindowExtent.height;
	pipelineBuilder.mViewport.minDepth = 0.0f;
	pipelineBuilder.mViewport.maxDepth = 1.0f;

	pipelineBuilder.mScissor.offset = { 0, 0 };
	pipelineBuilder.mScissor.extent = mWindowExtent;

	//configure the rasterizer to draw filled triangles
	pipelineBuilder.mRasterizer = vkInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);

	//we don't use multisampling, so just run the default one
	VkSampleCountFlagBits Msaa;
	if (EnableValidationLayers)
		Msaa = VK_SAMPLE_COUNT_1_BIT;
	else
		Msaa = mMaxMsaa;
	pipelineBuilder.mMultisampling = vkInit::MultisamplingStateCreateInfo(Msaa);

	//a single blend attachment with no blending and writing to RGBA
	pipelineBuilder.mColorBlendAttachment = vkInit::ColorBlendAttachmentState(VK_FALSE);

	VertexInputDescription vertexDescription = DebugVertex::GetVertexDescription();

	//connect the pipeline builder vertex input info to the one we get from Vertex
	pipelineBuilder.mVertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
	pipelineBuilder.mVertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)vertexDescription.attributes.size();

	pipelineBuilder.mVertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
	pipelineBuilder.mVertexInputInfo.vertexBindingDescriptionCount = (uint32_t)vertexDescription.bindings.size();


	VkPipelineLayoutCreateInfo pipeline_layout_info = vkInit::PipelineLayoutCreateInfo();
	
	//hook the global set layout
	VkDescriptorSetLayout setLayouts[] = { mGlobalSetLayout, mObjectSetLayout };

	pipeline_layout_info.setLayoutCount = 2;
	pipeline_layout_info.pSetLayouts = setLayouts;


	VK_CHECK(vkCreatePipelineLayout(mDevice, &pipeline_layout_info, nullptr, &mLinePipelineLayout));
	VK_CHECK(vkCreatePipelineLayout(mDevice, &pipeline_layout_info, nullptr, &mPointPipelineLayout));
	VK_CHECK(vkCreatePipelineLayout(mDevice, &pipeline_layout_info, nullptr, &mPointPipelineLayout2D));
	VK_CHECK(vkCreatePipelineLayout(mDevice, &pipeline_layout_info, nullptr, &mLinePipelineLayout2D));

	pipelineBuilder.mShaderStages.push_back(
		vkInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, VertexShader));

	pipelineBuilder.mShaderStages.push_back(
		vkInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, FragShader));

	//use the triangle layout we created
	pipelineBuilder.mPipelineLayout = mLinePipelineLayout;

	//finally build the pipeline
	VkRenderPass renderpass;
	if (EnableValidationLayers)
		renderpass = mViewportRenderPass;
	else
		renderpass = mRenderPass;

	mLinePipeline = pipelineBuilder.BuildPipeline(mDevice, renderpass);

	pipelineBuilder.mShaderStages.clear();
	pipelineBuilder.mShaderStages.push_back(
		vkInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, VertexShader2D));
	pipelineBuilder.mShaderStages.push_back(
		vkInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, FragShader2D));
	mLinePipeline2D = pipelineBuilder.BuildPipeline(mDevice, renderpass);

	pipelineBuilder.mInputAssembly = vkInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
	pipelineBuilder.mPipelineLayout = mPointPipelineLayout;

	pipelineBuilder.mShaderStages.clear();
	pipelineBuilder.mShaderStages.push_back(
		vkInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, PointVertexShader));
	pipelineBuilder.mShaderStages.push_back(
		vkInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, PointFragShader));
	mPointPipeline = pipelineBuilder.BuildPipeline(mDevice, renderpass);

	pipelineBuilder.mShaderStages.clear();
	pipelineBuilder.mShaderStages.push_back(
		vkInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, PointVertexShader2D));
	pipelineBuilder.mShaderStages.push_back(
		vkInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, PointFragShader2D));
	mPointPipeline2D = pipelineBuilder.BuildPipeline(mDevice, renderpass);


	vkDestroyShaderModule(mDevice, FragShader, nullptr);
	vkDestroyShaderModule(mDevice, VertexShader, nullptr);
	vkDestroyShaderModule(mDevice, PointFragShader, nullptr);
	vkDestroyShaderModule(mDevice, PointVertexShader, nullptr);
	vkDestroyShaderModule(mDevice, FragShader2D, nullptr);
	vkDestroyShaderModule(mDevice, VertexShader2D, nullptr);
	vkDestroyShaderModule(mDevice, PointFragShader2D, nullptr);
	vkDestroyShaderModule(mDevice, PointVertexShader2D, nullptr);

	mSwapChainDeletionQueue.push_function([=]() 
		{
			vkDestroyPipeline(mDevice, mLinePipeline, nullptr);
			vkDestroyPipelineLayout(mDevice, mLinePipelineLayout, nullptr);
			vkDestroyPipeline(mDevice, mPointPipeline, nullptr);
			vkDestroyPipelineLayout(mDevice, mPointPipelineLayout, nullptr);
			vkDestroyPipeline(mDevice, mLinePipeline2D, nullptr);
			vkDestroyPipelineLayout(mDevice, mLinePipelineLayout2D, nullptr);
			vkDestroyPipeline(mDevice, mPointPipeline2D, nullptr);
			vkDestroyPipelineLayout(mDevice, mPointPipelineLayout2D, nullptr);
		});
}

FVR::Material* FVR::VulkanRenderBackEnd::CreateMaterial(MaterialInfo matInfo,TextureHandle& textHandle,PIPELINE_TYPE type)
{
	Material* mat = new Material();
	mat->type = type;
	bool pipelineBuild = false;
	for (auto& it : mMaterials)
	{
		if (it.second->info.fragmentPath == matInfo.fragmentPath && it.second->info.vertexPath == matInfo.vertexPath)
		{
			pipelineBuild = true;
			if (it.second->text.path == matInfo.textureInfo->path)
			{
				textHandle = it.second->handle;
				delete mat;
				return it.second;
			}
			mat->pipeline = it.second->pipeline;
			mat->pipelineLayout = it.second->pipelineLayout;
		}
	}

	if (!pipelineBuild)
	{
		if (mat->pipeline == nullptr)
			mat->pipeline = std::make_shared<VkPipeline>();
		if (mat->pipelineLayout == nullptr)
			mat->pipelineLayout = std::make_shared<VkPipelineLayout>();
		BuildPipeline(matInfo.vertexPath,matInfo.fragmentPath ,*mat->pipeline.get(), *mat->pipelineLayout.get(),static_cast<PIPELINE_TYPE>(mat->type));
	}
	
	//todo also create the texture stuff
	mat->text = LoadImage(matInfo.textureInfo->path);
	mat->text.mImageSampler = CreateImageSampler(mat, matInfo);
	mat->info = matInfo;
	MaterialIndex++;
	textHandle = MaterialIndex;
	mat->handle = MaterialIndex;
	mat->pipelineBuild = true;
	mMaterials[textHandle] = mat;
	return mMaterials[textHandle];
}

void FVR::VulkanRenderBackEnd::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VkCommandBuffer cmd = mUploadContext.mCommandBuffer;

	//begin the command buffer recording. We will use this command buffer exactly once before resetting, so we tell vulkan that
	VkCommandBufferBeginInfo cmdBeginInfo = vkInit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	//execute the function
	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = vkInit::SubmitInfo(&cmd);


	//submit command buffer to the queue and execute it.
	// _uploadFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit(mGraphicsQueue, 1, &submit, mUploadContext.mUploadFence));

	vkWaitForFences(mDevice, 1, &mUploadContext.mUploadFence, true, 9999999999);
	vkResetFences(mDevice, 1, &mUploadContext.mUploadFence);

	// reset the command buffers inside the command pool
	vkResetCommandPool(mDevice, mUploadContext.mCommandPool, 0);
}
void FVR::VulkanRenderBackEnd::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function, VkCommandPool cmdPool)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = cmdPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmd;
	vkAllocateCommandBuffers(mDevice, &allocInfo, &cmd);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

	//execute the function
	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = vkInit::SubmitInfo(&cmd);


	//submit command buffer to the queue and execute it.
	// _uploadFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit(mGraphicsQueue, 1, &submit, VK_NULL_HANDLE));
	vkQueueWaitIdle(mGraphicsQueue);

	vkFreeCommandBuffers(mDevice, cmdPool, 1, &cmd);
}

FVR::FrameData& FVR::VulkanRenderBackEnd::GetCurrentFrame()
{
	return mFrames[mFrameNumber % FRAME_OVERLAP];
}

void FVR::VulkanRenderBackEnd::ReusePreviousLinePoints()
{
	int Frame = GetCurrentFrame().mSwapChainIndex;
	int LastFrame = Frame - 1;
	if (LastFrame < 0)
		LastFrame = (int)mSwapChainImages.size() - 1;

	mDebugLines[Frame].mPoints.insert(mDebugLines[Frame].mPoints.end(), mDebugLines[LastFrame].mPoints.begin(), mDebugLines[LastFrame].mPoints.end());
	mDebugPoints[Frame].insert(mDebugPoints[Frame].end(), mDebugPoints[LastFrame].begin(), mDebugPoints[LastFrame].end());
}

void FVR::VulkanRenderBackEnd::InitSwapChain()
{
	mSwapChain = vkInit::CreateSwapChain(mSurface,mChosenGPU, mDevice, mWindow, mSwapChainImageFormat, mWindowExtent, mSwapChainImages,mMinImageCount);
	//store swapchain and its related images
	
	mSwapChainImageViews.resize(mSwapChainImages.size());

	for (size_t i = 0; i < mSwapChainImages.size(); i++)
	{
		mSwapChainImageViews[i] = vkInit::CreateImageView(mSwapChainImages[i], mSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1,mDevice);
	}

	

	mSwapChainDeletionQueue.push_function([=]()
		{
		vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);
		});

	VkExtent3D depthImageExtent = 
	{
		mWindowExtent.width,
		mWindowExtent.height,
		1
	};

	//hardcoding the depth format to 32 bit float
	mDepthFormat = VK_FORMAT_D32_SFLOAT;

	//the depth image will be an image with the format we selected and Depth Attachment usage flag
	{

		CreateDepthImage(mDepthImageView, mDepthImage, mDepthFormat, mMaxMsaa);
	}

	{
		CreateDepthImage(mViewportDepthImageView,mViewportDepthImage,mDepthFormat,VK_SAMPLE_COUNT_1_BIT);
		
	}
	
}
void FVR::VulkanRenderBackEnd::InitVulkan()
{
	//store the instance
	mInstance = vkInit::CreateInstance("FVR","FCE",EnableValidationLayers,mValidationLayers,mInstanceAllocCallBack);
	//store the debug messenger
	if(EnableValidationLayers)
	mDebugMessenger = vkInit::CreateDebugMessenger(mInstance);

	// get the surface of the window we opened with SDL
	if (glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface) != VK_SUCCESS)
		throw std::runtime_error("failed to create window surface!");

	mChosenGPU = vkInit::PickPhysicalDevice(mInstance, mSurface, mMaxMsaa);
	if (mChosenGPU == VK_NULL_HANDLE)
		throw std::runtime_error("failed to find a suitable GPU!");

	//dont see a refrence to this somewhere
	/*
	VkPhysicalDeviceShaderDrawParametersFeatures shader_draw_parameters_features = {};
	shader_draw_parameters_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
	shader_draw_parameters_features.pNext = nullptr;
	shader_draw_parameters_features.shaderDrawParameters = VK_TRUE;
	vkb::Device vkbDevice = deviceBuilder.add_pNext(&shader_draw_parameters_features).build().value();
	*/
	// Get the VkDevice handle used in the rest of a Vulkan application
	mDevice = vkInit::CreateLogicalDevice(mChosenGPU,mSurface, EnableValidationLayers,mValidationLayers,mGraphicsQueue,mGraphicsQueueFamily);
	
	//initialize the memory allocator
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = mChosenGPU;
	allocatorInfo.device = mDevice;
	allocatorInfo.instance = mInstance;
	vmaCreateAllocator(&allocatorInfo, &mAllocator);

	mDeletionQueue.push_function([&]()
		{
		vmaDestroyAllocator(mAllocator);
		});

	vkGetPhysicalDeviceProperties(mChosenGPU, &mGpuProperties);
	std::cout << "The GPU has a minimum buffer alignment of " << mGpuProperties.limits.minUniformBufferOffsetAlignment << std::endl;
}
void FVR::VulkanRenderBackEnd::InitWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	mWindow = glfwCreateWindow(mWindowExtent.width, mWindowExtent.height,"FCE", nullptr, nullptr);
	mScreenSize = glm::vec2(mWindowExtent.width, mWindowExtent.height);
	glfwSetWindowUserPointer(mWindow, this);
	glfwSetFramebufferSizeCallback(mWindow, framebufferResizeCallback);
}
void FVR::VulkanRenderBackEnd::InitCommands()
{
	//create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers

	VkCommandPoolCreateInfo commandPoolInfo = vkInit::CommandPoolCreateInfo(mGraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++) 
	{
		VK_CHECK(vkCreateCommandPool(mDevice, &commandPoolInfo, nullptr, &mFrames[i].mCommandPool));

		//allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = vkInit::CommandBufferAllocateInfo(mFrames[i].mCommandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(mDevice, &cmdAllocInfo, &mFrames[i].mMainCommandBuffer));
	}
	mDeletionQueue.push_function([=]()
		{
			for (int i = 0; i < FRAME_OVERLAP; i++)
			{
				vkDestroyCommandPool(mDevice, mFrames[i].mCommandPool, nullptr);
			}
		});

	VkCommandPoolCreateInfo uploadCommandPoolInfo = vkInit::CommandPoolCreateInfo(mGraphicsQueueFamily);
	//create pool for upload context
	VK_CHECK(vkCreateCommandPool(mDevice, &uploadCommandPoolInfo, nullptr, &mUploadContext.mCommandPool));

	//allocate the default command buffer that we will use for the instant commands
	VkCommandBufferAllocateInfo cmdAllocInfo = vkInit::CommandBufferAllocateInfo(mUploadContext.mCommandPool, 1);

	VK_CHECK(vkAllocateCommandBuffers(mDevice, &cmdAllocInfo, &mUploadContext.mCommandBuffer));

	mViewportCommandBuffers.resize(FRAME_OVERLAP + 1);

	VkCommandPoolCreateInfo ViewportCommandPool = vkInit::CommandPoolCreateInfo(mGraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	VK_CHECK(vkCreateCommandPool(mDevice, &ViewportCommandPool, nullptr, &mViewportCommandPool));

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = mViewportCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)mViewportCommandBuffers.size();
	VK_CHECK(vkAllocateCommandBuffers(mDevice, &allocInfo, mViewportCommandBuffers.data()));


	mDeletionQueue.push_function([=]() {
		vkDestroyCommandPool(mDevice, mUploadContext.mCommandPool, nullptr);
		vkDestroyCommandPool(mDevice, mViewportCommandPool, nullptr);
		});




}
void FVR::VulkanRenderBackEnd::InitDefaultRenderpass()
{
	// the renderpass will use this color attachment.
	VkAttachmentDescription color_attachment = {};
	//the attachment will have the format needed by the swapchain
	color_attachment.format = mSwapChainImageFormat;
	//1 sample, we won't be doing MSAA
	color_attachment.samples = mMaxMsaa;
	// we Clear when this attachment is loaded
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// we keep the attachment stored when the renderpass ends
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	//we don't care about stencil
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//we don't know or care about the starting layout of the attachment
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	//after the renderpass ends, the image has to be on a layout ready for display
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_attachment_ref = {};
	//attachment number will index into the pAttachments array in the parent renderpass itself
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


	VkAttachmentDescription depth_attachment = {};
	// Depth attachment
	depth_attachment.flags = 0;
	depth_attachment.format = mDepthFormat;
	depth_attachment.samples = mMaxMsaa;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref = {};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format = mSwapChainImageFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//we are going to create 1 subpass, which is the minimum you can do
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;
	subpass.pDepthStencilAttachment = &depth_attachment_ref;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	VkAttachmentDescription attachments[3] = { color_attachment,depth_attachment,colorAttachmentResolve };

	//settign up the dependency to make sure the frames wait on each other
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency depth_dependency = {};
	depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	depth_dependency.dstSubpass = 0;
	depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depth_dependency.srcAccessMask = 0;
	depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency dependencies[2] = { dependency, depth_dependency };

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	//connect the color attachment to the info
	render_pass_info.attachmentCount = 3;
	render_pass_info.pAttachments = &attachments[0];
	//connect the subpass to the info
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 2;
	render_pass_info.pDependencies = &dependencies[0];


	VK_CHECK(vkCreateRenderPass(mDevice, &render_pass_info, nullptr, &mRenderPass));

	mSwapChainDeletionQueue.push_function([=]()
		{
		vkDestroyRenderPass(mDevice, mRenderPass, nullptr);
		});
}
void FVR::VulkanRenderBackEnd::InitFramebuffers()
{
	VkFramebufferCreateInfo fb_info = {};
	fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_info.pNext = nullptr;

	fb_info.renderPass = mRenderPass;
	fb_info.attachmentCount = 1;
	fb_info.width = mWindowExtent.width;
	fb_info.height = mWindowExtent.height;
	fb_info.layers = 1;

	//grab how many images we have in the swapchain
	const uint32_t swapchain_imagecount = (uint32_t)mSwapChainImages.size();
	mFramebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

	//create framebuffers for each of the swapchain image views
	for (uint32_t i = 0; i < swapchain_imagecount; i++) 
	{
		VkImageView attachments[3];
		attachments[0] = mMsaaImageView;
		attachments[1] = mDepthImageView;
		attachments[2] = mSwapChainImageViews[i];

		fb_info.pAttachments = attachments;
		fb_info.attachmentCount = 3;
		VK_CHECK(vkCreateFramebuffer(mDevice, &fb_info, nullptr, &mFramebuffers[i]));

		mSwapChainDeletionQueue.push_function([=]() 
			{
			vkDestroyFramebuffer(mDevice, mFramebuffers[i], nullptr);
			vkDestroyImageView(mDevice, mSwapChainImageViews[i], nullptr);
			});
	}
}
void FVR::VulkanRenderBackEnd::InitSyncStructures()
{
	VkFenceCreateInfo fenceCreateInfo = vkInit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

	VkSemaphoreCreateInfo semaphoreCreateInfo = vkInit::SemaphoreCreateInfo();

	for (int i = 0; i < FRAME_OVERLAP; i++) 
	{

		VK_CHECK(vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &mFrames[i].mRenderFence));

		//enqueue the destruction of the fence
		mDeletionQueue.push_function([=]()
			{
			vkDestroyFence(mDevice, mFrames[i].mRenderFence, nullptr);
			});


		VK_CHECK(vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &mFrames[i].mPresentSemaphore));
		VK_CHECK(vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &mFrames[i].mRenderSemaphore));

		//enqueue the destruction of semaphores
		mDeletionQueue.push_function([=]()
			{
			vkDestroySemaphore(mDevice, mFrames[i].mPresentSemaphore, nullptr);
			vkDestroySemaphore(mDevice, mFrames[i].mRenderSemaphore, nullptr);
			});
	}

	VkFenceCreateInfo uploadFenceCreateInfo = vkInit::FenceCreateInfo();

	VK_CHECK(vkCreateFence(mDevice, &uploadFenceCreateInfo, nullptr, &mUploadContext.mUploadFence));
	mDeletionQueue.push_function([=]() 
		{
		vkDestroyFence(mDevice, mUploadContext.mUploadFence, nullptr);
		});
	
}
void FVR::VulkanRenderBackEnd::InitDescriptors()
{
	mDescAllocator = new FVR::DescriptorAllocator();
	mDescAllocator->Init(mDevice);
	const size_t sceneParamBufferSize = FRAME_OVERLAP * PadUniformBufferSize(sizeof(GPUSceneData));

	mSceneParameterBuffer = CreateBuffer(sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	std::vector<VkDescriptorPoolSize> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = 0;
	pool_info.maxSets = 10;
	pool_info.poolSizeCount = (uint32_t)sizes.size();
	pool_info.pPoolSizes = sizes.data();

	vkCreateDescriptorPool(mDevice, &pool_info, nullptr, &mDescriptorPool);

	//binding for camera data at 0
	VkDescriptorSetLayoutBinding cameraBind = vkInit::DescriptorsetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);

	//binding for scene data at 1
	VkDescriptorSetLayoutBinding sceneBind = vkInit::DescriptorsetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);

	VkDescriptorSetLayoutBinding bindings[] = { cameraBind,sceneBind };

	VkDescriptorSetLayoutCreateInfo setinfo = {};
	setinfo.bindingCount = 2;
	setinfo.flags = 0;
	setinfo.pNext = nullptr;
	setinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setinfo.pBindings = bindings;


	vkCreateDescriptorSetLayout(mDevice, &setinfo, nullptr, &mGlobalSetLayout);

	VkDescriptorSetLayoutBinding objectBind = vkInit::DescriptorsetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);

	VkDescriptorSetLayoutCreateInfo set2info = {};
	set2info.bindingCount = 1;
	set2info.flags = 0;
	set2info.pNext = nullptr;
	set2info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set2info.pBindings = &objectBind;

	vkCreateDescriptorSetLayout(mDevice, &set2info, nullptr, &mObjectSetLayout);
	
	VkDescriptorSetLayoutBinding textureBind = vkInit::DescriptorsetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

	VkDescriptorSetLayoutCreateInfo set3info = {};
	set3info.bindingCount = 1;
	set3info.flags = 0;
	set3info.pNext = nullptr;
	set3info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set3info.pBindings = &textureBind;

	vkCreateDescriptorSetLayout(mDevice, &set3info, nullptr, &mSingleTextureSetLayout);

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		mFrames[i].mObjectBuffer = CreateBuffer(sizeof(GPUObjectData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		mFrames[i].mCameraBuffer = CreateBuffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		mFrames[i].mObject2DBuffer = CreateBuffer(sizeof(GPUObject2DData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		mFrames[i].mCamera2DBuffer = CreateBuffer(sizeof(GPUCamera2DData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		//allocate one descriptor set for each frame
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.pNext = nullptr;
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		//using the pool we just set
		allocInfo.descriptorPool = mDescriptorPool;
		//only 1 descriptor
		allocInfo.descriptorSetCount = 1;
		//using the global data layout
		allocInfo.pSetLayouts = &mGlobalSetLayout;

		vkAllocateDescriptorSets(mDevice, &allocInfo, &mFrames[i].mGlobalDescriptor);
		vkAllocateDescriptorSets(mDevice, &allocInfo, &mFrames[i].mGlobal2DDescriptor);

		VkDescriptorSetAllocateInfo objectSetAlloc = {};
		objectSetAlloc.pNext = nullptr;
		objectSetAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		objectSetAlloc.descriptorPool = mDescriptorPool;
		objectSetAlloc.descriptorSetCount = 1;
		objectSetAlloc.pSetLayouts = &mObjectSetLayout;

		vkAllocateDescriptorSets(mDevice, &objectSetAlloc, &mFrames[i].mObjectDescriptor);
		vkAllocateDescriptorSets(mDevice, &objectSetAlloc, &mFrames[i].mObject2DDescriptor);

		VkDescriptorBufferInfo cameraInfo;
		cameraInfo.buffer = mFrames[i].mCameraBuffer.mBuffer;
		cameraInfo.offset = 0;
		cameraInfo.range = sizeof(GPUCameraData);

		VkDescriptorBufferInfo sceneInfo;
		sceneInfo.buffer = mSceneParameterBuffer.mBuffer;
		sceneInfo.offset = 0;
		sceneInfo.range = sizeof(GPUSceneData);

		VkDescriptorBufferInfo objectBufferInfo;
		objectBufferInfo.buffer = mFrames[i].mObjectBuffer.mBuffer;
		objectBufferInfo.offset = 0;
		objectBufferInfo.range = sizeof(GPUObjectData) * MAX_OBJECTS;

		VkDescriptorBufferInfo camera2DInfo;
		camera2DInfo.buffer = mFrames[i].mCameraBuffer.mBuffer;
		camera2DInfo.offset = 0;
		camera2DInfo.range = sizeof(GPUCamera2DData);

		VkDescriptorBufferInfo objectBuffer2DInfo;
		objectBuffer2DInfo.buffer = mFrames[i].mObject2DBuffer.mBuffer;
		objectBuffer2DInfo.offset = 0;
		objectBuffer2DInfo.range = sizeof(GPUObject2DData) * MAX_OBJECTS;

		VkWriteDescriptorSet cameraWrite = vkInit::WriteDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, mFrames[i].mGlobalDescriptor, &cameraInfo, 0);

		VkWriteDescriptorSet sceneWrite = vkInit::WriteDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, mFrames[i].mGlobalDescriptor, &sceneInfo, 1);

		VkWriteDescriptorSet objectWrite = vkInit::WriteDescriptorBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, mFrames[i].mObjectDescriptor, &objectBufferInfo, 0);

		VkWriteDescriptorSet camera2DWrite = vkInit::WriteDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, mFrames[i].mGlobal2DDescriptor, &camera2DInfo, 0);

		VkWriteDescriptorSet object2DWrite = vkInit::WriteDescriptorBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, mFrames[i].mObject2DDescriptor, &objectBuffer2DInfo, 0);

		VkWriteDescriptorSet setWrites[] = {cameraWrite,sceneWrite,objectWrite, camera2DWrite,object2DWrite };

		vkUpdateDescriptorSets(mDevice, 5, setWrites, 0, nullptr);
	}

	// add buffers to deletion queues

		mDeletionQueue.push_function([&]()
			{
				for (int i = 0; i < FRAME_OVERLAP; i++)
				{
				vmaDestroyBuffer(mAllocator, mFrames[i].mObjectBuffer.mBuffer, mFrames[i].mObjectBuffer.mAllocation);
				vmaDestroyBuffer(mAllocator, mFrames[i].mCameraBuffer.mBuffer, mFrames[i].mCameraBuffer.mAllocation);
				vmaDestroyBuffer(mAllocator, mFrames[i].mObject2DBuffer.mBuffer, mFrames[i].mObject2DBuffer.mAllocation);
				vmaDestroyBuffer(mAllocator, mFrames[i].mCamera2DBuffer.mBuffer, mFrames[i].mCamera2DBuffer.mAllocation);
				}
			});
	


	// add descriptor set layout to deletion queues
	mDeletionQueue.push_function([&]() 
		{
		vmaDestroyBuffer(mAllocator, mSceneParameterBuffer.mBuffer, mSceneParameterBuffer.mAllocation);
		vkDestroyDescriptorSetLayout(mDevice, mGlobalSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(mDevice, mObjectSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(mDevice, mSingleTextureSetLayout, nullptr);
		vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
		mDescAllocator->Cleanup();
		});
}

bool FVR::VulkanRenderBackEnd::LoadShaderModule(const char* filePath, VkShaderModule* outShaderModule)
{
	//open the file. With cursor at the end
	std::cout << std::filesystem::current_path() << std::endl;
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);
	
	if (!file.is_open()) 
	{
		return false;
	}

	//find what the size of the file is by looking up the location of the cursor
//because the cursor is at the end, it gives the size directly in bytes
	size_t fileSize = (size_t)file.tellg();

	//spirv expects the buffer to be on uint32, so make sure to reserve an int vector big enough for the entire file
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	//put file cursor at beginning
	file.seekg(0);

	//load the entire file into the buffer
	file.read((char*)buffer.data(), fileSize);

	//now that the file is loaded into the buffer, we can close it
	file.close();

	//create a new shader module, using the buffer we loaded
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;

	//codeSize has to be in bytes, so multiply the ints in the buffer by size of int to know the real size of the buffer
	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();

	//check that the creation goes well.
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(mDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		return false;
	}
	*outShaderModule = shaderModule;
	return true;
}

void FVR::VulkanRenderBackEnd::UploadMesh(Mesh& mesh)
{
	const size_t bufferSize = mesh.mVertices.size() * sizeof(Vertex);
	//allocate staging buffer
	VkBufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.pNext = nullptr;

	stagingBufferInfo.size = bufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	//let the VMA library know that this data should be on CPU RAM
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	AllocatedBuffer stagingBuffer;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(mAllocator, &stagingBufferInfo, &vmaallocInfo,
		&stagingBuffer.mBuffer,
		&stagingBuffer.mAllocation,
		nullptr));

	//copy vertex data
	void* data;
	vmaMapMemory(mAllocator, stagingBuffer.mAllocation, &data);

	memcpy(data, mesh.mVertices.data(), mesh.mVertices.size() * sizeof(Vertex));

	vmaUnmapMemory(mAllocator, stagingBuffer.mAllocation);


	//allocate vertex buffer
	VkBufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.pNext = nullptr;
	//this is the total size, in bytes, of the buffer we are allocating
	vertexBufferInfo.size = bufferSize;
	//this buffer is going to be used as a Vertex Buffer
	vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	//let the VMA library know that this data should be GPU native
	vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(mAllocator, &vertexBufferInfo, &vmaallocInfo,
		&mesh.mVertexBuffer.mBuffer,
		&mesh.mVertexBuffer.mAllocation,
		nullptr));

	ImmediateSubmit([=](VkCommandBuffer cmd) 
		{
		VkBufferCopy copy;
		copy.dstOffset = 0;
		copy.srcOffset = 0;
		copy.size = bufferSize;
		vkCmdCopyBuffer(cmd, stagingBuffer.mBuffer, mesh.mVertexBuffer.mBuffer, 1, &copy);
		});

	mDeletionQueue.push_function([=]() 
		{
		vmaDestroyBuffer(mAllocator, mesh.mVertexBuffer.mBuffer, mesh.mVertexBuffer.mAllocation);
		});

	vmaDestroyBuffer(mAllocator, stagingBuffer.mBuffer, stagingBuffer.mAllocation);

}
void FVR::VulkanRenderBackEnd::UploadQuad()
{
	const size_t bufferSize = 4 * sizeof(Vertex2D);
	//allocate staging buffer
	VkBufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.pNext = nullptr;

	stagingBufferInfo.size = bufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	//let the VMA library know that this data should be on CPU RAM
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	AllocatedBuffer stagingBuffer;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(mAllocator, &stagingBufferInfo, &vmaallocInfo,
		&stagingBuffer.mBuffer,
		&stagingBuffer.mAllocation,
		nullptr));

	std::vector<Vertex2D> quad;
	quad.push_back(Vertex2D(glm::vec3(-0.5f, -0.5f, 0.f), glm::vec2(1.0f, 0.0f)));
	quad.push_back(Vertex2D(glm::vec3(0.5f, -0.5f, 0.f), glm::vec2(0.0f, 0.0f)));
	quad.push_back(Vertex2D(glm::vec3(0.5f, 0.5f, 0.f), glm::vec2(0.0f, 1.0f)));
	quad.push_back(Vertex2D(glm::vec3(-0.5f, 0.5f, 0.f), glm::vec2(1.0f, 1.0f)));

	//copy vertex data
	void* data;
	vmaMapMemory(mAllocator, stagingBuffer.mAllocation, &data);

	memcpy(data, quad.data(), quad.size() * sizeof(Vertex2D));

	vmaUnmapMemory(mAllocator, stagingBuffer.mAllocation);


	//allocate vertex buffer
	VkBufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.pNext = nullptr;
	//this is the total size, in bytes, of the buffer we are allocating
	vertexBufferInfo.size = bufferSize;
	//this buffer is going to be used as a Vertex Buffer
	vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	//let the VMA library know that this data should be GPU native
	vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(mAllocator, &vertexBufferInfo, &vmaallocInfo,
		&mQuadVertexBuffer.mBuffer,
		&mQuadVertexBuffer.mAllocation,
		nullptr));

	ImmediateSubmit([=](VkCommandBuffer cmd)
		{
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer.mBuffer, mQuadVertexBuffer.mBuffer, 1, &copy);
		});
	vmaDestroyBuffer(mAllocator, stagingBuffer.mBuffer, stagingBuffer.mAllocation);

	mDeletionQueue.push_function([=]() 
		{
			vmaDestroyBuffer(mAllocator, mQuadVertexBuffer.mBuffer, mQuadVertexBuffer.mAllocation);
		});
}
void FVR::VulkanRenderBackEnd::UploadLine(DebugVertex* start, uint32_t count, AllocatedBuffer& buffer)
{
	const size_t bufferSize = count * sizeof(DebugVertex);
	//allocate staging buffer
	VkBufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.pNext = nullptr;

	stagingBufferInfo.size = bufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	//let the VMA library know that this data should be on CPU RAM
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	AllocatedBuffer stagingBuffer;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(mAllocator, &stagingBufferInfo, &vmaallocInfo,
		&stagingBuffer.mBuffer,
		&stagingBuffer.mAllocation,
		nullptr));

	//copy vertex data
	void* data;
	vmaMapMemory(mAllocator, stagingBuffer.mAllocation, &data);

	memcpy(data, start, count * sizeof(DebugVertex));

	vmaUnmapMemory(mAllocator, stagingBuffer.mAllocation);

	//allocate vertex buffer
	VkBufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.pNext = nullptr;
	//this is the total size, in bytes, of the buffer we are allocating
	vertexBufferInfo.size = bufferSize;
	//this buffer is going to be used as a Vertex Buffer
	vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	//let the VMA library know that this data should be GPU native
	vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(mAllocator, &vertexBufferInfo, &vmaallocInfo,
		&buffer.mBuffer,
		&buffer.mAllocation,
		nullptr));

	ImmediateSubmit([=](VkCommandBuffer cmd)
		{
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = bufferSize;
			vkCmdCopyBuffer(cmd, stagingBuffer.mBuffer, buffer.mBuffer, 1, &copy);
		});

	vmaDestroyBuffer(mAllocator, stagingBuffer.mBuffer, stagingBuffer.mAllocation);
}

void FVR::VulkanRenderBackEnd::ClearDebugLines(int frame)
{
		if (mDebugLines[frame].mPoints.size() > 0)
	vmaDestroyBuffer(mAllocator, mDebugLines[frame].mVertexBuffer.mBuffer, mDebugLines[frame].mVertexBuffer.mAllocation);

	if (mDebugLines[frame].mPoints.size() > 0)
	mDebugLines[frame].mPoints.clear();

	if(mDebugPoints[frame].size() > 0)
	mDebugPoints[frame].clear();
}

void FVR::VulkanRenderBackEnd::Init()
{
	InitWindow();
	InitVulkan();
	InitSwapChain();
	InitCommands();
	InitViewportImage();
	InitViewportImageViews();
	//InitViewportDescriptor();
	InitDefaultRenderpass();
	InitViewportRenderpass();
	//InitViewportPipeline();
	InitMsaa();
	InitFramebuffers();
	InitViewportFrameBuffers();
	InitSyncStructures();
	InitDescriptors();
	InitImguiRenderPass();
	InitImguiCommandPool();
	InitImguiCommandBuffers();
	InitImguiFrameBuffer();
	InitImgui();
	InitImguiSampler();
	InitImguiDescriptor();
	InitDebugPipeline();
	InitQuadIndexBuffer();
	UploadQuad();
	mDebugLines.resize(mSwapChainImages.size());
	mDebugPoints.resize(mSwapChainImages.size());
	mPoint.mPos = glm::vec3(0, 0, 0);
	UploadLine(&mPoint, 1, mPointBuffer);

	mIsInitialised = true;
}

void FVR::VulkanRenderBackEnd::Draw(glm::mat4 view, glm::mat4 ProjectionMatrix,RenderObject* first,int count,VkCommandBuffer cmd)
{
	//make a model view matrix for rendering the object
	//camera view
	//camera projection

	GPUCameraData camData;
	camData.proj = ProjectionMatrix;
	camData.view = view;
	camData.viewproj = ProjectionMatrix * view;

	void* data;
	vmaMapMemory(mAllocator, GetCurrentFrame().mCameraBuffer.mAllocation, &data);

	memcpy(data, &camData, sizeof(GPUCameraData));

	vmaUnmapMemory(mAllocator, GetCurrentFrame().mCameraBuffer.mAllocation);

	void* objectData;
	vmaMapMemory(mAllocator, GetCurrentFrame().mObjectBuffer.mAllocation, &objectData);

	GPUObjectData* objectSSBO = (GPUObjectData*)objectData;

	for (int i = 0; i < count; i++)
	{
		RenderObject& object = first[i];
		objectSSBO[i].modelMatrix = object.transformMatrix;
	}

	vmaUnmapMemory(mAllocator, GetCurrentFrame().mObjectBuffer.mAllocation);


	float framed = (mFrameNumber / 120.f);

	mSceneParameters.ambientColor = { sin(framed),0,cos(framed),1 };

	char* sceneData;
	vmaMapMemory(mAllocator, mSceneParameterBuffer.mAllocation, (void**)&sceneData);

	int frameIndex = mFrameNumber % FRAME_OVERLAP;

	sceneData += PadUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;

	memcpy(sceneData, &mSceneParameters, sizeof(GPUSceneData));

	vmaUnmapMemory(mAllocator, mSceneParameterBuffer.mAllocation);

	Mesh* lastMesh = nullptr;
	Material* lastMaterial = nullptr;
	for (int i = 0; i < count; i++)
	{
		RenderObject& object = first[i];

		//only bind the pipeline if it doesn't match with the already bound one
		if (object.material != lastMaterial)
		{

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *object.material->pipeline.get());
			lastMaterial = object.material;

			//offset for our scene buffer
			uint32_t uniform_offset = (uint32_t)PadUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *object.material->pipelineLayout.get(), 0, 1, &GetCurrentFrame().mGlobalDescriptor, 1, &uniform_offset);

			//object data descriptor
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *object.material->pipelineLayout.get(), 1, 1, &GetCurrentFrame().mObjectDescriptor, 0, nullptr);

			if (object.material->textureSet != VK_NULL_HANDLE)
			{
				//texture descriptor
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *object.material->pipelineLayout.get(), 2, 1, &object.material->textureSet, 0, nullptr);
			}
		}

			MeshPushConstants constants;
			constants.render_matrix = object.transformMatrix;

			//upload the mesh to the GPU via push constants
			vkCmdPushConstants(cmd, *object.material->pipelineLayout.get(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

			//only bind the mesh if it's a different one from last bind
			if (object.mesh != lastMesh) 
			{
				//bind the mesh vertex buffer with offset 0
				VkDeviceSize offset = 0;
				vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->mVertexBuffer.mBuffer, &offset);
				lastMesh = object.mesh;
			}
			//we can now draw
			vkCmdDraw(cmd, (uint32_t)object.mesh->mVertices.size(), 1, 0, i);
	}
}
void FVR::VulkanRenderBackEnd::DebugDraw(glm::mat4 view, glm::mat4 Proj, DebugLine* FirstLine, int LineCount, DebugPoint* FirstPoint, int PointCount, VkCommandBuffer cmd, int offset)
{
	//use all of the mapping from the drawing function
	void* objectData;
	vmaMapMemory(mAllocator, GetCurrentFrame().mObjectBuffer.mAllocation, &objectData);

	GPUObjectData* objectSSBO = (GPUObjectData*)objectData;

	for (int i = 0; i < LineCount; i++)
	{
		DebugLine& object = FirstLine[i];
		objectSSBO[offset + i].modelMatrix = glm::mat4(1);
	}

	for (int i = 0; i < PointCount; i++)
	{
		DebugPoint& object = FirstPoint[i];
		objectSSBO[LineCount + offset + i].modelMatrix = object.Position;
	}

	vmaUnmapMemory(mAllocator, GetCurrentFrame().mObjectBuffer.mAllocation);

	int frameIndex = mFrameNumber % FRAME_OVERLAP;

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mLinePipeline);
	uint32_t uniform_offset = (uint32_t)PadUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mLinePipelineLayout, 0, 1, &GetCurrentFrame().mGlobalDescriptor, 1, &uniform_offset);
	//object data descriptor
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mLinePipelineLayout, 1, 1, &GetCurrentFrame().mObjectDescriptor, 0, nullptr);

	
	for (int i = 0; i < LineCount; i++)
	{
		DebugLine& object = FirstLine[i];
		if (!object.mPoints.empty())
		{
			//bind the mesh vertex buffer with offset 0
			VkDeviceSize Offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &object.mVertexBuffer.mBuffer, &Offset);
			vkCmdDraw(cmd, (uint32_t)object.mPoints.size(), 1, 0, offset + i);
		}
	}
	
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPointPipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPointPipelineLayout, 0, 1, &GetCurrentFrame().mGlobalDescriptor, 1, &uniform_offset);
	//object data descriptor
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPointPipelineLayout, 1, 1, &GetCurrentFrame().mObjectDescriptor, 0, nullptr);


	for (int i = 0; i < PointCount; i++)
	{
		DebugPoint& object = FirstPoint[i];

	//	bind the mesh vertex buffer with offset 0
		VkDeviceSize Offset = 0;
		vkCmdDraw(cmd, 1, 1, 0, LineCount + offset + i);
	}
}
void FVR::VulkanRenderBackEnd::Draw2D(glm::vec4 camPos, Render2DObject* first, int& count, VkCommandBuffer cmd)
{
	GPUCamera2DData camData;
	camData.Pos = camPos;
	camData.Pos /= glm::vec4((float)mScaleBase.width, (float)mScaleBase.height,1,1);
	camData.Pos.x;
	camData.Pos.z = 0;
	void* data;
	vmaMapMemory(mAllocator, GetCurrentFrame().mCameraBuffer.mAllocation, &data);

	memcpy(data, &camData, sizeof(GPUCamera2DData));

	vmaUnmapMemory(mAllocator, GetCurrentFrame().mCameraBuffer.mAllocation);
	//TODO WHY?
	glm::vec2 scaling = glm::vec2((float)mWindowExtent.width / 1700.f,(float)mWindowExtent.height / 900.f);

	void* objectData;
	vmaMapMemory(mAllocator, GetCurrentFrame().mObjectBuffer.mAllocation, &objectData);

	GPUObject2DData* objectSSBO = (GPUObject2DData*)objectData;
	{
		int currentVal = 0;
		for (int i = 0; i < count; i++)
		{
			Render2DObject& object = first[i];
			for (int it = 0; it < object.position.size(); it++)
			{
				objectSSBO[currentVal].Position = object.position[it] * glm::vec4(scaling, 0, 0) + glm::vec4(0, 0, 0, object.Rotation[it]);
				objectSSBO[currentVal].Scale = glm::vec4(object.Scaling[it], object.quad->TextureMetrics.mInitialOffset);
				currentVal++;
			}

		}
	}
	int ActualCount = 0;
	vmaUnmapMemory(mAllocator, GetCurrentFrame().mObjectBuffer.mAllocation);

	float framed = (mFrameNumber / 120.f);

	mSceneParameters.ambientColor = { sin(framed),0,cos(framed),1 };

	char* sceneData;
	vmaMapMemory(mAllocator, mSceneParameterBuffer.mAllocation, (void**)&sceneData);

	int frameIndex = mFrameNumber % FRAME_OVERLAP;

	sceneData += PadUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;

	memcpy(sceneData, &mSceneParameters, sizeof(GPUSceneData));

	vmaUnmapMemory(mAllocator, mSceneParameterBuffer.mAllocation);

	vkCmdBindIndexBuffer(cmd, mQuadIndexBuffer.mBuffer, 0, VK_INDEX_TYPE_UINT16);
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmd, 0, 1, &mQuadVertexBuffer.mBuffer, &offset);
	Render2DObject* lastQuad = nullptr;
	Material* lastMaterial = nullptr;
	
	for (int i = 0; i < count; i++)
	{
		Render2DObject& object = first[i];

		//only bind the pipeline if it doesn't match with the already bound one
		if (object.material != lastMaterial)
		{

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *object.material->pipeline.get());
			lastMaterial = object.material;

			//offset for our scene buffer
			uint32_t uniform_offset = (uint32_t)PadUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *object.material->pipelineLayout.get(), 0, 1, &GetCurrentFrame().mGlobalDescriptor, 1, &uniform_offset);

			//object data descriptor
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *object.material->pipelineLayout.get(), 1, 1, &GetCurrentFrame().mObjectDescriptor, 0, nullptr);

			if (object.material->textureSet != VK_NULL_HANDLE)
			{
				//texture descriptor
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *object.material->pipelineLayout.get(), 2, 1, &object.material->textureSet, 0, nullptr);
			}
		}

		//only bind the mesh if it's a different one from last bind
		bool rebind = false;
		if (!lastQuad)
			rebind = true;
		else if (object.quad != lastQuad->quad || object.Scaling != lastQuad->Scaling)
			rebind = true;

		if(rebind)
		{
		
			QuadPushConstant constants;
			constants.Size = glm::vec4(object.quad->mWidth ,object.quad->mHeight,object.quad->TextureMetrics.mTextScaling);
			vkCmdPushConstants(cmd, *object.material->pipelineLayout.get(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(QuadPushConstant), &constants);
			
			lastQuad = &object;
		}
		//we can now draw
		vkCmdDrawIndexed(cmd, 6, object.position.size(), 0, 0, ActualCount);
		ActualCount += object.position.size();
	}
	count = ActualCount;
}
void FVR::VulkanRenderBackEnd::DebugDraw2D(glm::vec4 CamPos, DebugLine* FirstLine, int LineCount, DebugPoint* FirstPoint, int PointCount, VkCommandBuffer cmd, int offset)
{
	//use all of the mapping from the drawing function
	void* objectData;
	vmaMapMemory(mAllocator, GetCurrentFrame().mObjectBuffer.mAllocation, &objectData);

	GPUObject2DData* objectSSBO = (GPUObject2DData*)objectData;

	for (int i = 0; i < LineCount; i++)
	{
		DebugLine& object = FirstLine[i];
		objectSSBO[offset + i].Position = glm::vec4(0);
	}

	for (int i = 0; i < PointCount; i++)
	{
		DebugPoint& object = FirstPoint[i];
		objectSSBO[LineCount + offset + i].Position = object.Position2D;
		objectSSBO[LineCount + offset + i].Scale = object.mPoint.mColor;
	}

	vmaUnmapMemory(mAllocator, GetCurrentFrame().mObjectBuffer.mAllocation);

	int frameIndex = mFrameNumber % FRAME_OVERLAP;

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mLinePipeline2D);
	uint32_t uniform_offset = (uint32_t)PadUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mLinePipelineLayout2D, 0, 1, &GetCurrentFrame().mGlobalDescriptor, 1, &uniform_offset);
	//object data descriptor
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mLinePipelineLayout2D, 1, 1, &GetCurrentFrame().mObjectDescriptor, 0, nullptr);


	for (int i = 0; i < LineCount; i++)
	{
		DebugLine& object = FirstLine[i];
		if (!object.mPoints.empty())
		{
			//bind the mesh vertex buffer with offset 0
			VkDeviceSize Offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &object.mVertexBuffer.mBuffer, &Offset);
			vkCmdDraw(cmd, (uint32_t)object.mPoints.size(), 1, 0, offset + i);
		}
	}

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPointPipeline2D);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPointPipelineLayout2D, 0, 1, &GetCurrentFrame().mGlobalDescriptor, 1, &uniform_offset);
	//object data descriptor
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPointPipelineLayout2D, 1, 1, &GetCurrentFrame().mObjectDescriptor, 0, nullptr);
	{
		VkDeviceSize Offset = 0;
		vkCmdBindVertexBuffers(cmd, 0, 1, &mPointBuffer.mBuffer, &Offset);
	}
	for (int i = 0; i < PointCount; i++)
	{
		DebugPoint& object = FirstPoint[i];
		//TODO points should have their own cmdbuffer;
		//	bind the mesh vertex buffer with offset 0
		VkDeviceSize Offset = 0;
		vkCmdDraw(cmd, 1, 1, 0, LineCount + offset + i);
	}
}

void FVR::VulkanRenderBackEnd::PushFrame(VkSemaphore* semaphores,int count)
{
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = count;
	presentInfo.pWaitSemaphores = semaphores;

	VkSwapchainKHR swapChains[] = { mSwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &GetCurrentFrame().mSwapChainIndex;
	presentInfo.pResults = nullptr; // Optional

	VkResult result = vkQueuePresentKHR(mGraphicsQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mNeedsResizing)
	{
		mNeedsResizing = false;
		RecreateSwapChain();
	}
}
void FVR::VulkanRenderBackEnd::Submit(std::vector<VkCommandBuffer> cmd)
{
	//std::array<VkCommandBuffer, 2> submitCommandBuffers =
	//{ mViewportCommandBuffers[swapchainindex], mImGuiCommandBuffers[swapchainindex] };

	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	submit.pWaitDstStageMask = &waitStage;

	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &GetCurrentFrame().mPresentSemaphore;

	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &GetCurrentFrame().mRenderSemaphore;

	submit.commandBufferCount = (uint32_t)cmd.size();
	submit.pCommandBuffers = cmd.data();
	VK_CHECK(vkQueueSubmit(mGraphicsQueue, 1, &submit, GetCurrentFrame().mRenderFence));
}

void FVR::VulkanRenderBackEnd::InitMsaa()
{
	VkFormat format = mSwapChainImageFormat;
	VkExtent3D ImageExtent =
	{
		mWindowExtent.width,
		mWindowExtent.height,
		1
	};
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY; 
	allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkImageCreateInfo info = vkInit::ImageCreateInfo(format, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, ImageExtent, 1, mMaxMsaa);

	vmaCreateImage(mAllocator, &info, &allocInfo, &mMsaaImage.mImage, &mMsaaImage.mAllocation, nullptr);
	mMsaaImageView = vkInit::CreateImageView(mMsaaImage.mImage, format,VK_IMAGE_ASPECT_COLOR_BIT , 1, mDevice);

	mSwapChainDeletionQueue.push_function([=]()
	{
			vkDestroyImageView(mDevice, mMsaaImageView, nullptr);
			vmaDestroyImage(mAllocator, mMsaaImage.mImage, mMsaaImage.mAllocation);
	});
}

void FVR::VulkanRenderBackEnd::InitViewportImage()
{
	mViewportImages.resize(mSwapChainImages.size());
	mDstImageMemory.resize(mSwapChainImages.size());

	for (uint32_t i = 0; i < mSwapChainImages.size(); i++)
	{
		// Create the linear tiled destination image to copy to and to read the memory from
		VkImageCreateInfo imageCreateCI{};
		imageCreateCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateCI.imageType = VK_IMAGE_TYPE_2D;
		// Note that vkCmdBlitImage (if supported) will also do format conversions if the swapchain color format would differ
		imageCreateCI.format = VK_FORMAT_B8G8R8A8_SRGB;
		imageCreateCI.extent.width = mWindowExtent.width;
		imageCreateCI.extent.height = mWindowExtent.height;
		imageCreateCI.extent.depth = 1;
		imageCreateCI.arrayLayers = 1;
		imageCreateCI.mipLevels = 1;
		imageCreateCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateCI.tiling = VK_IMAGE_TILING_LINEAR;
		imageCreateCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		// Create the image
		// VkImage dstImage;
		vkCreateImage(mDevice, &imageCreateCI, nullptr, &mViewportImages[i]);
		// Create memory to back up the image
		VkMemoryRequirements memRequirements;
		VkMemoryAllocateInfo memAllocInfo{};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		// VkDeviceMemory dstImageMemory;
		vkGetImageMemoryRequirements(mDevice, mViewportImages[i], &memRequirements);
		memAllocInfo.allocationSize = memRequirements.size;
		// Memory must be host visible to copy from
		memAllocInfo.memoryTypeIndex = vkInit::FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mChosenGPU);
		vkAllocateMemory(mDevice, &memAllocInfo, nullptr, &mDstImageMemory[i]);
		vkBindImageMemory(mDevice, mViewportImages[i], mDstImageMemory[i], 0);

		ImmediateSubmit([&](VkCommandBuffer cmd)
			{
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageMemoryBarrier.image = mViewportImages[i];
				imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

				vkCmdPipelineBarrier(
					cmd,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &imageMemoryBarrier);

			}, mViewportCommandPool);
	}

	mSwapChainDeletionQueue.push_function([=]()
		{
			for (int i = 0; i < mViewportImages.size(); i++)
			{
				vkDestroyImage(mDevice, mViewportImages[i], nullptr);
			}
			for (int i = 0; i < mDstImageMemory.size(); i++)
			{
				vkFreeMemory(mDevice, mDstImageMemory[i], nullptr);
			}
		});

}
void FVR::VulkanRenderBackEnd::InitViewportImageViews()
{
	mViewportImageViews.resize(mViewportImages.size());

	for (uint32_t i = 0; i < mViewportImages.size(); i++)
	{
		mViewportImageViews[i] = vkInit::CreateImageView(mViewportImages[i], VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1, mDevice);
	}
}
void FVR::VulkanRenderBackEnd::InitViewportFrameBuffers()
{
	mViewportFramebuffers.resize(mViewportImageViews.size());

	for (size_t i = 0; i < mViewportImageViews.size(); i++)
	{
		std::array<VkImageView, 2> attachments =
		{
				mViewportImageViews[i],
				mViewportDepthImageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = mViewportRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = mWindowExtent.width;
		framebufferInfo.height = mWindowExtent.height;
		framebufferInfo.layers = 1;

		VK_CHECK(vkCreateFramebuffer(mDevice, &framebufferInfo, nullptr, &mViewportFramebuffers[i]));

		mSwapChainDeletionQueue.push_function([=]() 
			{
				vkDestroyImageView(mDevice, mViewportImageViews[i], nullptr);
				vkDestroyFramebuffer(mDevice, mViewportFramebuffers[i], nullptr);
			});
	}
}
void FVR::VulkanRenderBackEnd::InitViewportRenderpass()
{
	// the renderpass will use this color attachment.
	VkAttachmentDescription color_attachment = {};
	//the attachment will have the format needed by the swapchain
	color_attachment.format = mSwapChainImageFormat;
	//1 sample, we won't be doing MSAA
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// we Clear when this attachment is loaded
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// we keep the attachment stored when the renderpass ends
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	//we don't care about stencil
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//we don't know or care about the starting layout of the attachment
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	//after the renderpass ends, the image has to be on a layout ready for display
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference color_attachment_ref = {};
	//attachment number will index into the pAttachments array in the parent renderpass itself
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		
	VkAttachmentDescription depth_attachment = {};
	// Depth attachment
	depth_attachment.flags = 0;
	depth_attachment.format = mDepthFormat;
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref = {};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	//we are going to create 1 subpass, which is the minimum you can do
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;
	subpass.pDepthStencilAttachment = &depth_attachment_ref;
	

	VkAttachmentDescription attachments[2] = { color_attachment,depth_attachment };

	//settign up the dependency to make sure the frames wait on each other
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency depth_dependency = {};
	depth_dependency.srcSubpass = 0;
	depth_dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
	depth_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	depth_dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	depth_dependency.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	depth_dependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

	VkSubpassDependency dependencies[2] = { dependency, depth_dependency };

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	//connect the color attachment to the info
	render_pass_info.attachmentCount = 2;
	render_pass_info.pAttachments = &attachments[0];
	//connect the subpass to the info
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 2;
	render_pass_info.pDependencies = &dependencies[0];


	VK_CHECK(vkCreateRenderPass(mDevice, &render_pass_info, nullptr, &mViewportRenderPass));

	mSwapChainDeletionQueue.push_function([=]()
		{
			vkDestroyRenderPass(mDevice, mViewportRenderPass, nullptr);
		});
}

void FVR::VulkanRenderBackEnd::CleanUp()
{
	vkDeviceWaitIdle(mDevice);
	if (mIsInitialised)
	{
		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			vkWaitForFences(mDevice, 1, &mFrames[i].mRenderFence, true, 1000000000);
		}
		for (int i = 0; i < mDebugLines.size(); i++)
		{
			ClearDebugLines(i);
		}

		for (auto it : mMaterials)
		{
			if (it.second->pipeline.use_count() == 1)
				vkDestroyPipeline(mDevice, *it.second->pipeline.get(), nullptr);
			if (it.second->pipelineLayout.use_count() == 1)
				vkDestroyPipelineLayout(mDevice, *it.second->pipelineLayout.get(), nullptr);

			vmaDestroyImage(mAllocator, it.second->text.mImage.mImage, it.second->text.mImage.mAllocation);
			vkDestroySampler(mDevice, it.second->text.mImageSampler, nullptr);
			vkDestroyImageView(mDevice, it.second->text.mImageView, nullptr);

			delete it.second;
		}
		mMaterials.clear();

		vmaDestroyBuffer(mAllocator, mPointBuffer.mBuffer, mPointBuffer.mAllocation);
		mSwapChainDeletionQueue.flush();
		mDeletionQueue.flush();

		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
			func(mInstance, mDebugMessenger, nullptr);
		
		vkDestroyDevice(mDevice, nullptr);
		vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
		vkDestroyInstance(mInstance, nullptr);

		glfwDestroyWindow(mWindow);
		glfwTerminate();
	}
}

FVR::Texture FVR::VulkanRenderBackEnd::LoadImage(const char* path)
{
	Texture text;
	text.miplevels = 0;
	text.path = path;
	vkUtil::LoadImageFromFile(*this, path, text.mImage,text.miplevels);
	
	VkImageViewCreateInfo imageinfo = vkInit::ImageCreateViews(VK_FORMAT_R8G8B8A8_SRGB, text.mImage.mImage, VK_IMAGE_ASPECT_COLOR_BIT,text.miplevels);
	vkCreateImageView(mDevice, &imageinfo, nullptr, &text.mImageView);

	return text;
}

FVR::MeshHandle FVR::VulkanRenderBackEnd::GetMesh(const char* path)
{
	//if loaded instantly return meshindex
	for (auto& it : mMeshes)
	{
		if (it.second.mFilePath == path)
			return it.first;
	}
	Mesh mesh;
	mesh.mFilePath = path;
	mesh.LoadFromOBJ(path);
	UploadMesh(mesh);
	MeshIndex++;
	mMeshes[MeshIndex] = mesh;
	return MeshIndex;
}
FVR::TextureHandle FVR::VulkanRenderBackEnd::GetTexture(MaterialInfo info,PIPELINE_TYPE type)
{
	TextureHandle handle;
	CreateMaterial(info, handle,type);
	return handle;
}
FVR::QuadHandle FVR::VulkanRenderBackEnd::GetQuad(uint32_t width, uint32_t height,glm::vec2 BaseOffset,glm::vec2 textureMapSize,glm::vec2 TextureSize)
{
	TextureMetrics metrics;
	metrics.mInitialOffset = BaseOffset / textureMapSize;
	metrics.mTextScaling = TextureSize / textureMapSize;
	//if loaded instantly return meshindex
	for (auto& it : mQuads)
	{
		if (it.second.mBaseWidth == width && it.second.mBaseHeight == height && metrics == it.second.TextureMetrics)
			return it.first;
	}
	Quad quad;
	quad.mBaseHeight = (float)height;
	quad.mBaseWidth = (float)width;
	quad.TextureMetrics = metrics;

	quad.mWidth = (width / (float)mScaleBase.width) * (mScaleBase.width / mScaleBase.height);
	quad.mHeight = (height / (float)mScaleBase.height);

	QuadIndex++;
	mQuads[QuadIndex] = quad;
	return QuadIndex;
}

bool FVR::VulkanRenderBackEnd::UnloadMesh(MeshHandle handle)
{
	auto it = mMeshes.find(handle);
	if (it != mMeshes.end())
	{
		it->second.mVertexBuffer;
		vmaDestroyBuffer(mAllocator, it->second.mVertexBuffer.mBuffer, it->second.mVertexBuffer.mAllocation);
		mMeshes.erase(handle);
		return true;
	}
	else
		return false;
}
bool FVR::VulkanRenderBackEnd::UnloadTexture(TextureHandle handle)
{
	auto it = mMaterials.find(handle);
	if (it != mMaterials.end())
	{
		if(it->second->pipeline.use_count() == 1)
		vkDestroyPipeline(mDevice, *it->second->pipeline, nullptr);
		if(it->second->pipelineLayout.use_count() == 1)
		vkDestroyPipelineLayout(mDevice, *it->second->pipelineLayout, nullptr);

		it->second->pipeline.reset();
		it->second->pipelineLayout.reset();
		delete it->second;
		mMaterials.erase(it->second->handle);
		return true;
	}
	return false;
}
bool FVR::VulkanRenderBackEnd::UnloadQuad(QuadHandle handle)
{
	auto it = mQuads.find(handle);
	if (it != mQuads.end())
	{
		mQuads.erase(handle);
		return true;
	}
	else
		return false;
}

int FVR::VulkanRenderBackEnd::BeginFrame(bool DeleteLastFrameData)
{
	//wait for the fences to be ready
	if (mImguiUsed)
		mImguiUsed = false;
	else
		mImguiSize = glm::vec2(0, 0);

	VK_CHECK(vkWaitForFences(mDevice, 1, &GetCurrentFrame().mRenderFence, true, 1000000000));
	
	VkResult result =vkAcquireNextImageKHR(mDevice, mSwapChain, 1000000000, GetCurrentFrame().mPresentSemaphore, nullptr, &GetCurrentFrame().mSwapChainIndex);
	uint32_t swapchainIndex = GetCurrentFrame().mSwapChainIndex;
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		mResized = true;
		std::cout << "begin" << std::endl;
		return glfwWindowShouldClose(mWindow);
	}
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	//set the clear value
	VkClearValue clearValue;
	float flash = abs(sin(mFrameNumber / 120.f));
	clearValue.color = { { 0.0f, 0.0f, flash, 1.0f } };

	//clear depth at 1
	VkClearValue depthClear;
	depthClear.depthStencil.depth = 1.f;

	VkClearValue clearValues[] = { clearValue, depthClear };

	if (EnableValidationLayers)
	{

		VK_CHECK(vkResetCommandBuffer(mViewportCommandBuffers[swapchainIndex], 0));
		VK_CHECK(vkResetCommandBuffer(mImGuiCommandBuffers[swapchainIndex], 0));

		ClearDebugLines(GetCurrentFrame().mSwapChainIndex);
		if (DeleteLastFrameData)
		{
			mRenderables.clear();
			m2DRenders.clear();
		}
		else
			ReusePreviousLinePoints();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;								// Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		VK_CHECK(vkBeginCommandBuffer(mViewportCommandBuffers[swapchainIndex], &beginInfo));

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = mViewportRenderPass;
		renderPassInfo.framebuffer = mViewportFramebuffers[swapchainIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = mWindowExtent;
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = &clearValues[0];

		vkCmdBeginRenderPass(mViewportCommandBuffers[swapchainIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}
	else
	{
		//now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
		VK_CHECK(vkResetCommandBuffer(GetCurrentFrame().mMainCommandBuffer, 0));

		ClearDebugLines(GetCurrentFrame().mSwapChainIndex);
		if (DeleteLastFrameData)
		{
			mRenderables.clear();
			m2DRenders.clear();
		}
		else
			ReusePreviousLinePoints();

		//start the main renderpass.
		//We will use the clear color from above, and the framebuffer of the index the swapchain gave us
		VkRenderPassBeginInfo rpInfo = vkInit::RenderPassBeginInfo(mRenderPass, mWindowExtent, mFramebuffers[GetCurrentFrame().mSwapChainIndex]);
		rpInfo.clearValueCount = 2;
		rpInfo.pClearValues = &clearValues[0];

		VkCommandBufferBeginInfo cmdBeginInfo = {};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBeginInfo.pNext = nullptr;

		cmdBeginInfo.pInheritanceInfo = nullptr;
		cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK(vkBeginCommandBuffer(GetCurrentFrame().mMainCommandBuffer, &cmdBeginInfo));

		vkCmdBeginRenderPass(GetCurrentFrame().mMainCommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	return glfwWindowShouldClose(mWindow);
}
void FVR::VulkanRenderBackEnd::EndFrame(glm::mat4 view, glm::mat4 ProjectionMatrix)
{
	if (mResized)
	{
		mResized = false;
		return;
	}
		//reset the fences
	VK_CHECK(vkResetFences(mDevice, 1, &GetCurrentFrame().mRenderFence));
	uint32_t swapchainindex = GetCurrentFrame().mSwapChainIndex;

	if(!mDebugLines[swapchainindex].mPoints.empty())
	UploadLine(mDebugLines[swapchainindex].mPoints.data(), (uint32_t)mDebugLines[swapchainindex].mPoints.size(), mDebugLines[swapchainindex].mVertexBuffer);
	

	if (EnableValidationLayers)
	{	
		Draw(view,ProjectionMatrix,mRenderables.data(),(int)mRenderables.size(),mViewportCommandBuffers[swapchainindex]);

		DebugDraw(view, ProjectionMatrix, &mDebugLines[GetCurrentFrame().mSwapChainIndex],1
			,mDebugPoints[GetCurrentFrame().mSwapChainIndex].data()
			, (int) mDebugPoints[GetCurrentFrame().mSwapChainIndex].size(), mViewportCommandBuffers[swapchainindex], (int)mRenderables.size());

		vkCmdEndRenderPass(mViewportCommandBuffers[swapchainindex]);
	
		VK_CHECK(vkEndCommandBuffer(mViewportCommandBuffers[swapchainindex]));

		//vkCmdWaitEvents(mImGuiCommandBuffers[swapchainindex], 1, &mEvents[swapchainindex], VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, nullptr, 0, nullptr, 0,nullptr);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;								// Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional
		vkBeginCommandBuffer(mImGuiCommandBuffers[swapchainindex],&beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = mImGuiRenderPass;
		renderPassInfo.framebuffer = mImGuiFramebuffers[swapchainindex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = mWindowExtent;
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		vkCmdBeginRenderPass(mImGuiCommandBuffers[swapchainindex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		


		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), mImGuiCommandBuffers[swapchainindex]);

		vkCmdEndRenderPass(mImGuiCommandBuffers[swapchainindex]);
		vkEndCommandBuffer(mImGuiCommandBuffers[swapchainindex]);

		std::vector<VkCommandBuffer> cmd;
		cmd.push_back(mViewportCommandBuffers[swapchainindex]);
		cmd.push_back(mImGuiCommandBuffers[swapchainindex]);
		Submit(cmd);
		PushFrame(&GetCurrentFrame().mRenderSemaphore, 1);
	}
	else
	{
		VkCommandBuffer cmd = GetCurrentFrame().mMainCommandBuffer;

		ImGui::Render();
		Draw(view, ProjectionMatrix, mRenderables.data(), (int)mRenderables.size(), cmd);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		//finalize the render pass
		vkCmdEndRenderPass(cmd);
		VK_CHECK(vkEndCommandBuffer(cmd));

		std::vector<VkCommandBuffer> commandBuffers;
		commandBuffers.push_back(cmd);
		Submit(commandBuffers);

		PushFrame(&GetCurrentFrame().mRenderSemaphore, 1);
	}
	//increase the number of frames drawn
	mFrameNumber++;
}
void FVR::VulkanRenderBackEnd::EndFrame2D(glm::vec4 camPos)
{
	if (mResized)
	{
		mResized = false;
		return;
	}
	//reset the fences
	VK_CHECK(vkResetFences(mDevice, 1, &GetCurrentFrame().mRenderFence));
	uint32_t swapchainindex = GetCurrentFrame().mSwapChainIndex;

	if (!mDebugLines[swapchainindex].mPoints.empty())
		UploadLine(mDebugLines[swapchainindex].mPoints.data(), (uint32_t)mDebugLines[swapchainindex].mPoints.size(), mDebugLines[swapchainindex].mVertexBuffer);

	if (EnableValidationLayers)
	{
		int size = (int)m2DRenders.size();
		Draw2D(camPos, m2DRenders.data(), size, mViewportCommandBuffers[swapchainindex]);

		DebugDraw2D(camPos, &mDebugLines[GetCurrentFrame().mSwapChainIndex], 1,
			mDebugPoints[GetCurrentFrame().mSwapChainIndex].data(),
			(int)mDebugPoints[GetCurrentFrame().mSwapChainIndex].size(), mViewportCommandBuffers[swapchainindex], size);

		vkCmdEndRenderPass(mViewportCommandBuffers[swapchainindex]);
		VK_CHECK(vkEndCommandBuffer(mViewportCommandBuffers[swapchainindex]));

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;								// Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional
		vkBeginCommandBuffer(mImGuiCommandBuffers[swapchainindex], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = mImGuiRenderPass;
		renderPassInfo.framebuffer = mImGuiFramebuffers[swapchainindex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = mWindowExtent;
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		vkCmdBeginRenderPass(mImGuiCommandBuffers[swapchainindex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), mImGuiCommandBuffers[swapchainindex]);

		vkCmdEndRenderPass(mImGuiCommandBuffers[swapchainindex]);
		vkEndCommandBuffer(mImGuiCommandBuffers[swapchainindex]);

		std::vector<VkCommandBuffer> cmd;
		cmd.push_back(mViewportCommandBuffers[swapchainindex]);
		cmd.push_back(mImGuiCommandBuffers[swapchainindex]);
		Submit(cmd);
		PushFrame(&GetCurrentFrame().mRenderSemaphore, 1);
	}
	else
	{
		VkCommandBuffer cmd = GetCurrentFrame().mMainCommandBuffer;

		int size = (int)m2DRenders.size();
		Draw2D(camPos, m2DRenders.data(), size, cmd);

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
		
		//finalize the render pass
		vkCmdEndRenderPass(cmd);
		VK_CHECK(vkEndCommandBuffer(cmd));

		std::vector<VkCommandBuffer> commandBuffers;
		commandBuffers.push_back(cmd);
		Submit(commandBuffers);

		PushFrame(&GetCurrentFrame().mRenderSemaphore, 1);
	}
	//increase the number of frames drawn
	mFrameNumber++;
}

size_t FVR::VulkanRenderBackEnd::PadUniformBufferSize(size_t originalSize)
{
		// Calculate required alignment based on minimum device offset alignment
		size_t minUboAlignment = mGpuProperties.limits.minUniformBufferOffsetAlignment;
		size_t alignedSize = originalSize;
		if (minUboAlignment > 0) {
			alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}
		return alignedSize;
}

void FVR::VulkanRenderBackEnd::BuildPipeline(const char* vertPath, const char* fragPath,VkPipeline& pipeline, VkPipelineLayout& layout,PIPELINE_TYPE type)
{
	PipelineBuilder pipelineBuilder;

	VkShaderModule FragShader;
	if (!LoadShaderModule(fragPath, &FragShader))
		std::cout << "Error when building fragment shader module path: " << fragPath << std::endl;

	VkShaderModule VertexShader;
	if (!LoadShaderModule(vertPath, &VertexShader))
		std::cout << "Error when building the  vertex shader module path: "<< vertPath << std::endl;

	pipelineBuilder.mDepthStencil = vkInit::DepthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

	//vertex input controls how to read vertices from vertex buffers. We aren't using it yet
	pipelineBuilder.mVertexInputInfo = vkInit::VertexInputStateCreateInfo();

	//input assembly is the configuration for drawing triangle lists, strips, or individual points.
	//we are just going to draw triangle list
	pipelineBuilder.mInputAssembly = vkInit::InputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	//build viewport and scissor from the swapchain extents
	pipelineBuilder.mViewport.x = 0.0f;
	pipelineBuilder.mViewport.y = 0.0f;
	pipelineBuilder.mViewport.width = (float)mWindowExtent.width;
	pipelineBuilder.mViewport.height = (float)mWindowExtent.height;
	pipelineBuilder.mViewport.minDepth = 0.0f;
	pipelineBuilder.mViewport.maxDepth = 1.0f;

	pipelineBuilder.mScissor.offset = { 0, 0 };
	pipelineBuilder.mScissor.extent = mWindowExtent;

	//configure the rasterizer to draw filled triangles
	pipelineBuilder.mRasterizer = vkInit::RasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);

	//we dont use Msaa in the debug build
	VkSampleCountFlagBits Msaa;
	if (EnableValidationLayers)
		Msaa = VK_SAMPLE_COUNT_1_BIT;
	else
		Msaa = mMaxMsaa;

	pipelineBuilder.mMultisampling = vkInit::MultisamplingStateCreateInfo(Msaa);

	//a single blend attachment with no blending and writing to RGBA
	pipelineBuilder.mColorBlendAttachment = vkInit::ColorBlendAttachmentState(VK_TRUE);

	VertexInputDescription vertexDescription;
	switch (type)
	{
	case PIPELINE_TYPE_2D:
		vertexDescription = Vertex2D::GetVertexDescription();
		break;
	case PIPELINE_TYPE_3D:
		vertexDescription = Vertex::GetVertexDescription();
		break;
	}
	//connect the pipeline builder vertex input info to the one we get from Vertex
	pipelineBuilder.mVertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
	pipelineBuilder.mVertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)vertexDescription.attributes.size();

	pipelineBuilder.mVertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
	pipelineBuilder.mVertexInputInfo.vertexBindingDescriptionCount = (uint32_t)vertexDescription.bindings.size();


	VkPipelineLayoutCreateInfo pipeline_layout_info = vkInit::PipelineLayoutCreateInfo();
	VkPushConstantRange push_constant;
	VkDescriptorSetLayout setLayouts[] = { mGlobalSetLayout, mObjectSetLayout,mSingleTextureSetLayout };
	switch (type)
	{
	case PIPELINE_TYPE_3D:
	{
		//setup push constants

		//this push constant range starts at the beginning
		push_constant.offset = 0;
		//this push constant range takes up the size of a MeshPushConstants struct
		push_constant.size = sizeof(MeshPushConstants);
		//this push constant range is accessible only in the vertex shader
		push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		//push-constant setup
		pipeline_layout_info.pPushConstantRanges = &push_constant;
		pipeline_layout_info.pushConstantRangeCount = 1;

		//hook the global set layout
	
		pipeline_layout_info.setLayoutCount = 3;
		pipeline_layout_info.pSetLayouts = setLayouts;
		break;
	}

	case PIPELINE_TYPE_2D:
	{
		//setup push constants
		//this push constant range starts at the beginning
		push_constant.offset = 0;
		//this push constant range takes up the size of a MeshPushConstants struct
		push_constant.size = sizeof(QuadPushConstant);
		//this push constant range is accessible only in the vertex shader
		push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		//push-constant setup
		pipeline_layout_info.pPushConstantRanges = &push_constant;
		pipeline_layout_info.pushConstantRangeCount = 1;

		//hook the global set layout
		pipeline_layout_info.setLayoutCount = 3;
		pipeline_layout_info.pSetLayouts = setLayouts;
		break;
	}

	}
	VK_CHECK(vkCreatePipelineLayout(mDevice, &pipeline_layout_info, nullptr, &layout));

	pipelineBuilder.mShaderStages.push_back(
		vkInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, VertexShader));

	pipelineBuilder.mShaderStages.push_back(
		vkInit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, FragShader));

	//use the triangle layout we created
	pipelineBuilder.mPipelineLayout = layout;

	//finally build the pipeline
	VkRenderPass renderpass;
	if (EnableValidationLayers)
		renderpass = mViewportRenderPass;
	else
		renderpass = mRenderPass;

	pipeline = pipelineBuilder.BuildPipeline(mDevice, renderpass);

	vkDestroyShaderModule(mDevice,FragShader,nullptr);
	vkDestroyShaderModule(mDevice,VertexShader,nullptr);
}

VkSampler FVR::VulkanRenderBackEnd::CreateImageSampler(Material* mat,MaterialInfo info)
{
	uint32_t miplevels = glm::max(mat->text.miplevels,(uint32_t)1);
	VkSamplerCreateInfo samplerInfo = vkInit::SamplerCreateInfo(info.textureInfo->filter,VK_SAMPLER_ADDRESS_MODE_REPEAT,miplevels);

	VkSampler Sampler;
	vkCreateSampler(mDevice, &samplerInfo, nullptr, &Sampler);

	//allocate the descriptor set for single-texture to use on the material
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.pNext = nullptr;
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = mDescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &mSingleTextureSetLayout;
	mDescAllocator->Allocate(&mat->textureSet, mSingleTextureSetLayout);
	//VK_CHECK(vkAllocateDescriptorSets(mDevice, &allocInfo, &mat->textureSet));

	//write to the descriptor set so that it points to our empire_diffuse texture
	VkDescriptorImageInfo imageBufferInfo;
	imageBufferInfo.sampler = Sampler;
	imageBufferInfo.imageView = mat->text.mImageView;
	imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet texture1 = vkInit::WriteDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mat->textureSet, &imageBufferInfo, 0);
	
	vkUpdateDescriptorSets(mDevice, 1, &texture1, 0, nullptr);
	return Sampler;
}
void FVR::VulkanRenderBackEnd::CreateDepthImage(VkImageView& depthImageView, AllocatedImage depthImage, VkFormat format, VkSampleCountFlagBits msaa)
{
	VkExtent3D depthImageExtent =
	{
		mWindowExtent.width,
		mWindowExtent.height,
		1
	};

	VkImageCreateInfo dimg_info = vkInit::ImageCreateInfo(format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent, 1, msaa);

	//for the depth image, we want to allocate it from GPU local memory
	VmaAllocationCreateInfo dimg_allocinfo = {};
	dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(mAllocator, &dimg_info, &dimg_allocinfo, &depthImage.mImage, &depthImage.mAllocation, nullptr);

	//build an image-view for the depth image to use for rendering
	VkImageViewCreateInfo dview_info = vkInit::ImageCreateViews(format, depthImage.mImage, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	VK_CHECK(vkCreateImageView(mDevice, &dview_info, nullptr, &depthImageView));

	mSwapChainDeletionQueue.push_function([=]()
		{
			vkDestroyImageView(mDevice, depthImageView, nullptr);
			vmaDestroyImage(mAllocator, depthImage.mImage, depthImage.mAllocation);
		});
}

void FVR::VulkanRenderBackEnd::RecreateSwapChain()
{
	int width = 0, height = 0;
	mPreviousExtend = mWindowExtent;
	glfwGetFramebufferSize(mWindow, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(mWindow, &width, &height);
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(mDevice);

	mSwapChainDeletionQueue.flush();

	InitSwapChain();
	InitViewportImage();
	InitViewportImageViews();
	InitDefaultRenderpass();
	InitImguiRenderPass();
	InitViewportRenderpass();
	InitDebugPipeline();
	RecreateMaterials();
	InitMsaa();
	InitFramebuffers();
	InitImguiFrameBuffer();
	InitViewportFrameBuffers();
	InitImguiSampler();
	InitImguiDescriptor();
	RecreateQuads();
}
void FVR::VulkanRenderBackEnd::RecreateMaterials()
{
	for (auto& it : mMaterials)
	{
		if (it.second->pipeline.use_count() == 1)
			vkDestroyPipeline(mDevice,*it.second->pipeline.get(), nullptr);
	
		if (it.second->pipelineLayout.use_count() == 1)
			vkDestroyPipelineLayout(mDevice, *it.second->pipelineLayout.get(), nullptr);
	
		it.second->pipeline.reset();
		it.second->pipelineLayout.reset();

		it.second->pipelineBuild = false;
	}

	for (auto& it : mMaterials)
	{
		bool PipelineDone = false;
		for (auto& iter : mMaterials)
		{
			if (it.second->info.fragmentPath == iter.second->info.fragmentPath && it.second->info.vertexPath == iter.second->info.vertexPath)
			{
				if (iter.second->pipeline != nullptr && iter.second->pipelineLayout != nullptr)
				{
					it.second->pipeline = iter.second->pipeline;
					it.second->pipelineLayout = iter.second->pipelineLayout;
					PipelineDone = true;
				}
				
			}
		}
		if (!PipelineDone)
		{
			if (it.second->pipeline == nullptr)
				it.second->pipeline = std::make_shared<VkPipeline>();
			if (it.second->pipelineLayout == nullptr)
				it.second->pipelineLayout = std::make_shared<VkPipelineLayout>();
			BuildPipeline(it.second->info.vertexPath, it.second->info.fragmentPath, *it.second->pipeline.get(), *it.second->pipelineLayout.get(),static_cast<PIPELINE_TYPE>(it.second->type));
		}

		it.second->pipelineBuild = true;
	}
}
void FVR::VulkanRenderBackEnd::RecreateQuads()
{
	for (auto it : mQuads)
	{
		it.second.mHeight = (it.second.mBaseHeight / (float)mScaleBase.height);
		it.second.mWidth =  (it.second.mBaseWidth / (float)mScaleBase.width) * (mScaleBase.width / mScaleBase.height);
	}
}

void FVR::VulkanRenderBackEnd::DrawModelTextured(glm::mat4 pos, FVR::MeshHandle mesh, FVR::TextureHandle text)
{
	RenderObject obj;
	obj.transformMatrix = pos;
	{
		auto it = mMaterials.find(text);
		if (it == mMaterials.end())
			throw std::runtime_error("Failed to find material");
		else
			obj.material = it->second;
	}
	{
		auto it = mMeshes.find(mesh);
		if (it == mMeshes.end())
			throw std::runtime_error("Failed to find mesh");
		else
			obj.mesh = &it->second;
	}
	mRenderables.push_back(obj);
}
void FVR::VulkanRenderBackEnd::DrawLine(glm::vec3 Begin, glm::vec3 End, glm::vec3 BeginColor,glm::vec3 EndColor)
{
	DebugLine line;
	DebugVertex v1;
	DebugVertex v2;
	v1.mColor = glm::vec4(BeginColor,1);
	v2.mColor = glm::vec4(EndColor, 1);

	v1.mPos = Begin;
	v2.mPos = End;


	mDebugLines[GetCurrentFrame().mSwapChainIndex].mPoints.push_back(v1);
	mDebugLines[GetCurrentFrame().mSwapChainIndex].mPoints.push_back(v2);
}
void FVR::VulkanRenderBackEnd::DrawPoint(glm::vec3 Begin, glm::vec3 Color)
{
	DebugPoint point;
	point.mPoint.mColor = glm::vec4(Color, 1);
	point.mPoint.mPos = Begin;
	point.Position = glm::translate(glm::mat4(1), glm::vec3(0));
	point.Position2D = glm::vec4(Begin,0) / glm::vec4(mWindowExtent.width, mWindowExtent.height, 1, 1);
	mDebugPoints[GetCurrentFrame().mSwapChainIndex].push_back(point);
}
void FVR::VulkanRenderBackEnd::DrawQuad(glm::vec4 pos,glm::vec2 Scaling, FVR::QuadHandle quad, FVR::TextureHandle text,float Rotation)
{
	for (auto& it : m2DRenders)
	{
		if (it.quadHandle == quad && it.matHandle == text)
		{
			it.position.push_back(pos / glm::vec4(mWindowExtent.width, mWindowExtent.height, 1, 1));
			it.Rotation.push_back(Rotation);
			it.Scaling.push_back(Scaling);
			return;
		}
	}

	Render2DObject obj;

	obj.position.push_back(pos / glm::vec4(mWindowExtent.width, mWindowExtent.height, 1, 1));
	obj.Rotation.push_back(Rotation);
	obj.Scaling.push_back(Scaling);
	obj.matHandle = text;
	obj.quadHandle = quad;
	{
		auto it = mMaterials.find(text);
		if (it == mMaterials.end())
			throw std::runtime_error("Failed to find material");
		else
			obj.material = it->second;
	}
	{
		auto it = mQuads.find(quad);
		if (it == mQuads.end())
			throw std::runtime_error("Failed to find mesh");
		else
		{
			obj.quad = &it->second;
		}
	}
	m2DRenders.push_back(obj);
}

void FVR::VulkanRenderBackEnd::DrawImguiViewPort()
{
	ImVec2 size = ImGui::GetContentRegionAvail();
	mImguiSize = glm::vec2(size.x, size.y);
	mImguiUsed = true;
	ImGui::Image(mDset[GetCurrentFrame().mSwapChainIndex], size);
}

glm::vec2 FVR::VulkanRenderBackEnd::ScreenSize()
{
	return mScreenSize;
}

glm::vec2 FVR::VulkanRenderBackEnd::WindowSize()
{
	if (glm::length(mImguiSize) > 0.1f)
		return mImguiSize;
	else
		return mScreenSize;
}

void FVR::VulkanRenderBackEnd::BeginTextureCreation(glm::vec2 Size)
{
	if (ImageBeingCreated)
		return;
	ImageBeingCreated = true;


	Texture text;
	text.miplevels = static_cast<uint32_t>(std::floor(std::log2(std::max(Size.x, Size.y)))) + 1;
	
	VkDeviceSize imageSize = Size.x * Size.y * 4;
	//the format R8G8B8A8 matches exactly with the pixels loaded from stb_image lib
	VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB;

	//allocate temporary buffer for holding texture data to upload
	AllocatedBuffer stagingBuffer = CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	VkExtent3D imageExtent;
	imageExtent.width = static_cast<uint32_t>(Size.x);
	imageExtent.height = static_cast<uint32_t>(Size.y);
	imageExtent.depth = 1;

	VkImageCreateInfo dimg_info = vkInit::ImageCreateInfo(image_format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, imageExtent, text.miplevels, VK_SAMPLE_COUNT_1_BIT);

	AllocatedImage newImage;

	VmaAllocationCreateInfo dimg_allocinfo = {};
	dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	//allocate and create the image
	vmaCreateImage(mAllocator, &dimg_info, &dimg_allocinfo, &newImage.mImage, &newImage.mAllocation, nullptr);

	ImmediateSubmit([&](VkCommandBuffer cmd)
		{
			VkImageSubresourceRange range;
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel = 0;
			range.levelCount = text.miplevels;
			range.baseArrayLayer = 0;
			range.layerCount = 1;

			VkImageMemoryBarrier imageBarrier_toTransfer = {};
			imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

			imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrier_toTransfer.image = newImage.mImage;
			imageBarrier_toTransfer.subresourceRange = range;

			imageBarrier_toTransfer.srcAccessMask = 0;
			imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			//barrier the image into the transfer-receive layout
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);
		});


	//make the image readable for the shader
	ImmediateSubmit([&](VkCommandBuffer cmd)
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
			vkCmdCopyBufferToImage(cmd, stagingBuffer.mBuffer, newImage.mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
		});

	//generate the mipmaps
	ImmediateSubmit([&](VkCommandBuffer cmd)
	{
	 //check if mipmaps are allowed
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(mChosenGPU, VK_FORMAT_R8G8B8A8_SRGB, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		{
			std::cout << ("texture image format does not support linear blitting!") << std::endl;
			text.miplevels = 1;
		}
		//generate mip levels
		else
		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = newImage.mImage;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.subresourceRange.levelCount = 1;

			int32_t mipWidth = Size.x;
			int32_t mipHeight = Size.y;

				for (uint32_t i = 1; i < text.miplevels; i++)
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

					vkCmdBlitImage(cmd, newImage.mImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						newImage.mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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

				barrier.subresourceRange.baseMipLevel = text.miplevels - 1;
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

	mDeletionQueue.push_function([=]() {

		vmaDestroyImage(mAllocator, newImage.mImage, newImage.mAllocation);
		});

	vmaDestroyBuffer(mAllocator, stagingBuffer.mBuffer, stagingBuffer.mAllocation);

	text.mImage = newImage;
}
