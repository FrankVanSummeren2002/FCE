#include "vk_DescriptorBuilder.h"
#include "algorithm"
using namespace FVR;
VkDescriptorPool createPool(VkDevice device, const DescriptorAllocator::PoolSizes& poolSizes, int count, VkDescriptorPoolCreateFlags flags)
{
	std::vector<VkDescriptorPoolSize> sizes;
	sizes.reserve(poolSizes.sizes.size());
	for (auto sz : poolSizes.sizes) {
		sizes.push_back({ sz.first, uint32_t(sz.second * count) });
	}
	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = flags;
	pool_info.maxSets = count;
	pool_info.poolSizeCount = (uint32_t)sizes.size();
	pool_info.pPoolSizes = sizes.data();

	VkDescriptorPool descriptorPool;
	vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);

	return descriptorPool;
}




void DescriptorAllocator::ResetPools()
{
	//reset all used pools and add them to the free pools
	for (auto p : mUsedPools) 
	{
		vkResetDescriptorPool(mDevice, p, 0);
		mFreePools.push_back(p);
	}

	//clear the used pools, since we've put them all in the free pools
	mUsedPools.clear();

	//reset the current pool handle back to null
	mCurrentPool = VK_NULL_HANDLE;
}

bool DescriptorAllocator::Allocate(VkDescriptorSet* Set, VkDescriptorSetLayout Layout)
{
	//initialize the currentPool handle if it's null
	if (mCurrentPool == VK_NULL_HANDLE) 
	{
		mCurrentPool = GrabPool();
		mUsedPools.push_back(mCurrentPool);
	}

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;

	allocInfo.pSetLayouts = &Layout;
	allocInfo.descriptorPool = mCurrentPool;
	allocInfo.descriptorSetCount = 1;

	//try to allocate the descriptor set
	VkResult allocResult = vkAllocateDescriptorSets(mDevice, &allocInfo, Set);
	bool needReallocate = false;

	switch (allocResult) 
	{
	case VK_SUCCESS:
		//all good, return
		return true;
	case VK_ERROR_FRAGMENTED_POOL:
	case VK_ERROR_OUT_OF_POOL_MEMORY:
		//reallocate pool
		needReallocate = true;
		break;
	default:
		//unrecoverable error
		return false;
	}

	if (needReallocate) 
	{
		//allocate a new pool and retry
		mCurrentPool = GrabPool();
		mUsedPools.push_back(mCurrentPool);

		allocResult = vkAllocateDescriptorSets(mDevice, &allocInfo, Set);

		//if it still fails then we have big issues
		if (allocResult == VK_SUCCESS) 
		{
			return true;
		}
	}

	return false;
}

void DescriptorAllocator::Init(VkDevice Device)
{
	mDevice = Device;
}

void DescriptorAllocator::Cleanup()
{
	for (auto p : mFreePools)
	{
		vkDestroyDescriptorPool(mDevice, p, nullptr);
	}
	for (auto p : mUsedPools)
	{
		vkDestroyDescriptorPool(mDevice, p, nullptr);
	}
}

VkDescriptorPool DescriptorAllocator::GrabPool()
{
	if (mFreePools.size() > 0)
	{
		//grab pool from the back of the vector and remove it from there.
		VkDescriptorPool pool = mFreePools.back();
		mFreePools.pop_back();
		return pool;
	}
	else
	{
		//no pools availible, so create a new one
		return createPool(mDevice, mDescriptorSizes, 1000, 0);
	}
}






void DescriptorLayoutCache::Init(VkDevice Device)
{
	mDevice = Device;
}

void DescriptorLayoutCache::Cleanup()
{
	for (auto pair : mLayoutCache) 
		vkDestroyDescriptorSetLayout(mDevice, pair.second, nullptr);
}

VkDescriptorSetLayout DescriptorLayoutCache::CreateDescriptorLayout(VkDescriptorSetLayoutCreateInfo* Info)
{
	DescriptorLayoutInfo layoutinfo;
	layoutinfo.bindings.reserve(Info->bindingCount);
	bool isSorted = true;
	uint32_t lastBinding = -1;

	//copy from the direct info struct into our own one
	for (uint32_t i = 0; i < Info->bindingCount; i++) 
	{
		layoutinfo.bindings.push_back(Info->pBindings[i]);

		//check that the bindings are in strict increasing order
		if (Info->pBindings[i].binding > lastBinding) 
		{
			lastBinding = Info->pBindings[i].binding;
		}
		else 
		{
			isSorted = false;
		}
	}
	//sort the bindings if they aren't in order
	if (!isSorted) 
	{
		std::sort(layoutinfo.bindings.begin(), layoutinfo.bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b) 
			{
			return a.binding < b.binding;
			});
	}

	//try to grab from cache
	auto it = mLayoutCache.find(layoutinfo);
	if (it != mLayoutCache.end()) 
	{
		return (*it).second;
	}
	else 
	{
		//create a new one (not found)
		VkDescriptorSetLayout layout;
		vkCreateDescriptorSetLayout(mDevice, Info, nullptr, &layout);

		//add to cache
		mLayoutCache[layoutinfo] = layout;
		return layout;
	}
}

bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const
{
	if (other.bindings.size() != bindings.size())
		return false;
	else 
	{
		//compare each of the bindings is the same. Bindings are sorted so they will match
		for (int i = 0; i < bindings.size(); i++) 
		{
			if (other.bindings[i].binding != bindings[i].binding) 
				return false;
			if (other.bindings[i].descriptorType != bindings[i].descriptorType) 
				return false;
			if (other.bindings[i].descriptorCount != bindings[i].descriptorCount)
				return false;
			if (other.bindings[i].stageFlags != bindings[i].stageFlags)
				return false;
		}
		return true;
	}
}

size_t DescriptorLayoutCache::DescriptorLayoutInfo::Hash() const
{
	using std::size_t;
	using std::hash;

	size_t result = hash<size_t>()(bindings.size());

	for (const VkDescriptorSetLayoutBinding& b : bindings)
	{
		//pack the binding data into a single int64. Not fully correct but it's ok
		size_t binding_hash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

		//shuffle the packed binding data and xor it with the main hash
		result ^= hash<size_t>()(binding_hash);
	}

	return result;
}






DescriptorBuilder DescriptorBuilder::Begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator)
{
	DescriptorBuilder builder;

	builder.mCache = layoutCache;
	builder.mAlloc = allocator;
	return builder;
}

DescriptorBuilder& DescriptorBuilder::BindBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags)
{
	//create the descriptor binding for the layout
	VkDescriptorSetLayoutBinding newBinding{};

	newBinding.descriptorCount = 1;
	newBinding.descriptorType = type;
	newBinding.pImmutableSamplers = nullptr;
	newBinding.stageFlags = stageFlags;
	newBinding.binding = binding;

	mBindings.push_back(newBinding);

	//create the descriptor write
	VkWriteDescriptorSet newWrite{};
	newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	newWrite.pNext = nullptr;

	newWrite.descriptorCount = 1;
	newWrite.descriptorType = type;
	newWrite.pBufferInfo = bufferInfo;
	newWrite.dstBinding = binding;

	mWrites.push_back(newWrite);
	return *this;
}

DescriptorBuilder& DescriptorBuilder::BindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags)
{
	VkDescriptorSetLayoutBinding newBinding{};

	newBinding.descriptorCount = 1;
	newBinding.descriptorType = type;
	newBinding.pImmutableSamplers = nullptr;
	newBinding.stageFlags = stageFlags;
	newBinding.binding = binding;

	mBindings.push_back(newBinding);

	VkWriteDescriptorSet newWrite{};
	newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	newWrite.pNext = nullptr;

	newWrite.descriptorCount = 1;
	newWrite.descriptorType = type;
	newWrite.pImageInfo = imageInfo;
	newWrite.dstBinding = binding;

	mWrites.push_back(newWrite);
	return *this;
}

bool DescriptorBuilder::Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout)
{
	//build layout first
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;

	layoutInfo.pBindings = mBindings.data();
	layoutInfo.bindingCount = (uint32_t)mBindings.size();

	layout = mCache->CreateDescriptorLayout(&layoutInfo);

	//allocate descriptor
	bool success = mAlloc->Allocate(&set, layout);
	if (!success) { return false; };

	//write descriptor
	for (VkWriteDescriptorSet& w : mWrites)
		w.dstSet = set;

	vkUpdateDescriptorSets(mAlloc->mDevice, (uint32_t)mWrites.size(), mWrites.data(), 0, nullptr);

	return true;
}

bool DescriptorBuilder::Build(VkDescriptorSet& set)
{
	VkDescriptorSetLayout layout;
	Build(set, layout);
	return false;
}