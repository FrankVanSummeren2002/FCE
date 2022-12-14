#pragma once
#include "vector"
#include "unordered_map"
#include "vulkan/vulkan.h"
namespace FVR
{
	class DescriptorAllocator
	{
	public:

		struct PoolSizes
		{
			std::vector<std::pair<VkDescriptorType, float>> sizes =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
			};
		};

		void ResetPools();
		bool Allocate(VkDescriptorSet* Set, VkDescriptorSetLayout Layout);

		void Init(VkDevice Device);

		void Cleanup();

		VkDevice mDevice{};
	private:
		VkDescriptorPool GrabPool();

		VkDescriptorPool mCurrentPool{ VK_NULL_HANDLE };
		PoolSizes mDescriptorSizes;
		std::vector<VkDescriptorPool> mUsedPools;
		std::vector<VkDescriptorPool> mFreePools;
	};


	class DescriptorLayoutCache
	{
	public:
		void Init(VkDevice Device);
		void Cleanup();

		VkDescriptorSetLayout CreateDescriptorLayout(VkDescriptorSetLayoutCreateInfo* Info);

		struct DescriptorLayoutInfo
		{
			//good idea to turn this into a inlined array
			std::vector<VkDescriptorSetLayoutBinding> bindings;

			bool operator==(const DescriptorLayoutInfo& other) const;

			size_t Hash() const;
		};

	private:

		struct DescriptorLayoutHash
		{

			std::size_t operator()(const DescriptorLayoutInfo& k) const
			{
				return k.Hash();
			}
		};

		std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> mLayoutCache;
		VkDevice mDevice;
	};

	class DescriptorBuilder
	{
	public:
		static DescriptorBuilder Begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator);

		DescriptorBuilder& BindBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
		DescriptorBuilder& BindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

		bool Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
		bool Build(VkDescriptorSet& set);
	private:

		std::vector<VkWriteDescriptorSet> mWrites;
		std::vector<VkDescriptorSetLayoutBinding> mBindings;

		DescriptorLayoutCache* mCache = nullptr;
		DescriptorAllocator* mAlloc = nullptr;
	};
}