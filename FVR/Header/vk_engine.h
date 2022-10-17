// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.
#pragma once

#include "vk_Helpers.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "Handles.h"
//TODO add resizing
//TODO ImguiViewPort
//TODO Figure out how to do multiple different shader stuff

enum PIPELINE_TYPE
{
	PIPELINE_TYPE_2D,
	PIPELINE_TYPE_3D
};

namespace FVR
{
	class DescriptorAllocator;
	constexpr unsigned int FRAME_OVERLAP = 2;

	class VulkanRenderBackEnd
	{
		UploadContext mUploadContext;

		std::vector<RenderObject> mRenderables;
		std::vector<Render2DObject> m2DRenders;

		std::unordered_map<TextureHandle, Material*> mMaterials;
		std::unordered_map<MeshHandle, Mesh> mMeshes;
		std::unordered_map<QuadHandle, Quad> mQuads;
		FrameData mFrames[FRAME_OVERLAP];

		bool mIsInitialised{ false };
		bool mResized{ false };
		int mFrameNumber{ 0 };
		int mMinImageCount{ 0 };

		VkSampleCountFlagBits mMaxMsaa;
		
		uint32_t MeshIndex = 0;
		uint32_t QuadIndex = 0;
		uint32_t MaterialIndex = 0;

		VkExtent2D mPreviousExtend;
		VkExtent2D mWindowExtent{ 1700 , 900 };
		VkExtent2D mScaleBase = mWindowExtent;
		GLFWwindow* mWindow{nullptr};

		VkInstance mInstance; // Vulkan library handle
		VkAllocationCallbacks* mInstanceAllocCallBack;
		VkDebugUtilsMessengerEXT mDebugMessenger; // Vulkan debug output handle
		VkDevice mDevice; // Vulkan device for commands
		VkSurfaceKHR mSurface; // Vulkan window surface

		VkSwapchainKHR mSwapChain; // from other articles
		VkFormat mSwapChainImageFormat;	// image format expected by the windowing system
		std::vector<VkImage> mSwapChainImages;	//array of images from the swapchain
		std::vector<VkImageView> mSwapChainImageViews;	//array of image-views from the swapchain

		//viewport
		std::vector<VkImage> mViewportImages;
		std::vector<VkDeviceMemory> mDstImageMemory;
		std::vector<VkImageView> mViewportImageViews;
		VkRenderPass mViewportRenderPass;
		VkCommandPool mViewportCommandPool;
		std::vector<VkFramebuffer> mViewportFramebuffers;
		std::vector<VkCommandBuffer> mViewportCommandBuffers;
		VkSampler mViewportSampler;
		std::vector<VkDescriptorSet> mDset;
		VkImageView mViewportDepthImageView;
		AllocatedImage mViewportDepthImage;

		//Imgui
		VkDescriptorPool mImGuiDescriptorPool;
		VkRenderPass mImGuiRenderPass;
		std::vector<VkFramebuffer> mImGuiFramebuffers;
		VkCommandPool mImGuiCommandPool;
		std::vector<VkCommandBuffer> mImGuiCommandBuffers;
		VkDescriptorPool mImguiPool;

		//depthImage
		VkImageView mDepthImageView;
		AllocatedImage mDepthImage;
		//the format for the depth image
		VkFormat mDepthFormat;

		//queue
		VkQueue mGraphicsQueue; //queue we will submit to
		uint32_t mGraphicsQueueFamily; //family of that queue

		//default rendering
		VkRenderPass mRenderPass;
		std::vector<VkFramebuffer> mFramebuffers;

		VkDescriptorSetLayout mSingleTextureSetLayout;

		VkDescriptorSetLayout mGlobalSetLayout;
		VkDescriptorPool mDescriptorPool;

		VkDescriptorSetLayout mObjectSetLayout;

		VkPhysicalDeviceProperties mGpuProperties;

		GPUSceneData mSceneParameters;
		AllocatedBuffer mSceneParameterBuffer;

		
		FVR::AllocatedBuffer mQuadIndexBuffer;
		FVR::AllocatedBuffer mQuadVertexBuffer;
		//MSAA
		AllocatedImage mMsaaImage;
		VkImageView mMsaaImageView;

		glm::vec2 mScreenSize;
		glm::vec2 mImguiSize{0,0};
		bool mImguiUsed = false;

		DescriptorAllocator* mDescAllocator;

		//debug
		const std::vector<const char*> mValidationLayers = { "VK_LAYER_KHRONOS_validation" };
		VkPipeline mLinePipeline;
		VkPipelineLayout mLinePipelineLayout;
		VkPipeline mPointPipeline;
		VkPipelineLayout mPointPipelineLayout;
		VkPipeline mLinePipeline2D;
		VkPipelineLayout mLinePipelineLayout2D;
		VkPipeline mPointPipeline2D;
		VkPipelineLayout mPointPipelineLayout2D;


		std::vector<DebugLine> mDebugLines;
		std::vector<std::vector<DebugPoint>> mDebugPoints;
		AllocatedBuffer mPointBuffer;
		DebugVertex mPoint;
		//Image Creation
		bool ImageBeingCreated{ false };

	public:
		bool mImguiScreenUsed{false};
		bool mNeedsResizing{ false };

		VmaAllocator mAllocator;
		DeletionQueue mDeletionQueue;
		DeletionQueue mSwapChainDeletionQueue;
		VkPhysicalDevice mChosenGPU; // GPU chosen as the default device
	private:
		void InitSwapChain();
		void InitVulkan();
		void InitWindow();
		void InitCommands();
		void InitDefaultRenderpass();
		void InitFramebuffers();
		void InitSyncStructures();
		void InitDescriptors();
		void InitImgui();
		void InitMsaa();
		
		void InitViewportImage();
		void InitViewportImageViews();
		void InitViewportFrameBuffers();
		void InitViewportRenderpass();

		void InitImguiRenderPass();
		void InitImguiCommandPool();
		void InitImguiCommandBuffers();
		void InitImguiFrameBuffer();
		void InitImguiSampler();
		void InitImguiDescriptor();
		
		void InitQuadIndexBuffer();

		void InitDebugPipeline();

		Material* CreateMaterial(MaterialInfo matInfo, TextureHandle& texthHandle, PIPELINE_TYPE type);
		VkSampler CreateImageSampler(Material* mat, MaterialInfo info);
		void CreateDepthImage(VkImageView& depthImageView,AllocatedImage depthImage , VkFormat format, VkSampleCountFlagBits msaa);

		void RecreateSwapChain();
		void RecreateMaterials();

		void RecreateQuads();

		bool LoadShaderModule(const char* filePath, VkShaderModule* outShaderModule);
		Texture LoadImage(const char* path);

		void UploadMesh(Mesh& mesh);
		void UploadQuad();
		void UploadLine(DebugVertex* start, uint32_t count, AllocatedBuffer& buffer);

		void ClearDebugLines(int frame);

		size_t PadUniformBufferSize(size_t originalSize);

		void BuildPipeline(const char* vertPath, const char* fragPath, VkPipeline& pipeline, VkPipelineLayout& layout,PIPELINE_TYPE type);

		void Draw(glm::mat4 view, glm::mat4 ProjectionMatrix, RenderObject* first, int count, VkCommandBuffer cmd);
		void DebugDraw(glm::mat4 view, glm::mat4 Proj, DebugLine* FirstLine, int LineCount ,DebugPoint* FirstPoint, int PointCount, VkCommandBuffer cmd, int offset);
		void Draw2D(glm::vec4 camPos, Render2DObject* first, int& count, VkCommandBuffer cmd);

		void DebugDraw2D(glm::vec4 CamPos, DebugLine* FirstLine, int LineCount, DebugPoint* FirstPoint, int PointCount, VkCommandBuffer cmd, int offset);

		void PushFrame(VkSemaphore* first,int count);
		void Submit(std::vector<VkCommandBuffer> cmd);
		//getter for the frame we are rendering to right now.
		FrameData& GetCurrentFrame();

		void ReusePreviousLinePoints();
	public:
		AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function,VkCommandPool cmdPool);

		void Init();
		
		void CleanUp();

		MeshHandle GetMesh(const char* path);
		TextureHandle GetTexture(MaterialInfo, PIPELINE_TYPE type);
		QuadHandle GetQuad(uint32_t width, uint32_t height, glm::vec2 BaseOffset, glm::vec2 textureMapSize, glm::vec2 TextureSize);

		bool UnloadMesh(MeshHandle handle);
		bool UnloadTexture(TextureHandle);
		bool UnloadQuad(QuadHandle handle);

		//a boolean for deleting the last frames data
		//you usually want this to be set to true unless you have paused the updates for your game
		//but still want to display the last frame
		int BeginFrame(bool DeleteLastFlightData = true);
		
		void EndFrame(glm::mat4 View, glm::mat4 ProjectionMatrix);
		void EndFrame2D(glm::vec4 camPos);

		void DrawModelTextured(glm::mat4 pos, MeshHandle mesh, TextureHandle text);
		void DrawLine(glm::vec3 Begin, glm::vec3 End, glm::vec3 BeginColor,glm::vec3 EndColor);
		void DrawPoint(glm::vec3 Begin, glm::vec3 Color);
		void DrawQuad(glm::vec4 pos,glm::vec2 Scaling, FVR::QuadHandle handle, FVR::TextureHandle text,float Rotation);
		void DrawImguiViewPort();
	
		
		glm::vec2 ScreenSize();
		glm::vec2 WindowSize();
		bool ShouldCloseWindow() { return !(glfwWindowShouldClose(mWindow) != 1); };
		GLFWwindow* GetWindow() { return mWindow; };

		void BeginTextureCreation(glm::vec2 Size);
	};
}