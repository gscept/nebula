//------------------------------------------------------------------------------
//  vkmemory.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkmemory.h"
#include "vkgraphicsdevice.h"
namespace CoreGraphics
{

using namespace Vulkan;

//------------------------------------------------------------------------------
/**
*/
void 
SetupMemoryPools(
	DeviceSize deviceLocalMemory,
	DeviceSize manuallyFlushedMemory,
	DeviceSize hostCoherentMemory)
{
	VkPhysicalDeviceMemoryProperties props = Vulkan::GetMemoryProperties();

	// setup a pool for every memory type
	CoreGraphics::Pools.Resize(VK_MAX_MEMORY_TYPES);
	for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
	{
		CoreGraphics::MemoryPool& pool = CoreGraphics::Pools[i];
		pool.maxSize = props.memoryHeaps[props.memoryTypes[i].heapIndex].size;
		pool.memoryType = i;
		pool.mapMemory = false;
		pool.blockSize = 0;

		if (CheckBits(props.memoryTypes[i].propertyFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		{
			// setup pool info
			pool.mapMemory = false;
			pool.blockSize = deviceLocalMemory;
			pool.allocMethod = MemoryPool::MemoryPool_AllocConservative;
		}
		else if (CheckBits(props.memoryTypes[i].propertyFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT))
		{
			// setup pool info
			pool.mapMemory = true;
			pool.blockSize = manuallyFlushedMemory;
			pool.allocMethod = MemoryPool::MemoryPool_AllocConservative;
		}
		else if (CheckBits(props.memoryTypes[i].propertyFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		{
			// setup pool info
			pool.mapMemory = true;
			pool.blockSize = hostCoherentMemory;
			pool.allocMethod = MemoryPool::MemoryPool_AllocLinear;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
DiscardMemoryPools(VkDevice dev)
{
	for (IndexT i = 0; i < CoreGraphics::Pools.Size(); i++)
	{
		CoreGraphics::Pools[i].Clear();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FreeMemory(const Alloc& alloc)
{
	// dealloc
	CoreGraphics::MemoryPool& pool = CoreGraphics::Pools[alloc.poolIndex];
	AllocationLoc.Enter();
	bool res = pool.DeallocateMemory(alloc);
	AllocationLoc.Leave();

	// assert result
	n_assert(res);
}

//------------------------------------------------------------------------------
/**
*/
void* 
GetMappedMemory(const CoreGraphics::Alloc& alloc)
{
	return CoreGraphics::Pools[alloc.poolIndex].GetMappedMemory(alloc);
}

//------------------------------------------------------------------------------
/**
*/
DeviceMemory 
MemoryPool::CreateBlock(bool map, void** outMappedPtr)
{
	VkDevice dev = GetCurrentDevice();
	VkMemoryAllocateInfo allocInfo =
	{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr,
		this->blockSize,
		this->memoryType
	};
	VkDeviceMemory mem;
	VkResult res = vkAllocateMemory(dev, &allocInfo, nullptr, &mem);
	n_assert(res == VK_SUCCESS);

	if (map)
	{
		res = vkMapMemory(dev, mem, 0, VK_WHOLE_SIZE, 0, outMappedPtr);
		n_assert(res == VK_SUCCESS);
	}

	return mem;
}

//------------------------------------------------------------------------------
/**
*/
void 
MemoryPool::DestroyBlock(DeviceMemory mem, bool unmap)
{
	VkDevice dev = GetCurrentDevice();
	if (unmap)
	{
		vkUnmapMemory(dev, mem);
	}
	vkFreeMemory(dev, mem, nullptr);
}

} // namespace CoreGraphics


namespace Vulkan
{

using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
Alloc
AllocateMemory(const VkDevice dev, const VkImage& img, MemoryPoolType type)
{
	VkMemoryRequirements req;
	vkGetImageMemoryRequirements(dev, img, &req);

	VkMemoryPropertyFlags flags;

	switch (type)
	{
	case MemoryPool_DeviceLocal:
		flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		break;
	default:
		n_crash("AllocateMemory(): Only buffer pool types are allowed for buffer memory");
	}

	uint32_t poolIndex;
	VkResult res = GetMemoryType(req.memoryTypeBits, flags, poolIndex);
	n_assert(res == VK_SUCCESS);
	CoreGraphics::MemoryPool& pool = CoreGraphics::Pools[poolIndex];

	// allocate
	AllocationLoc.Enter();
	Alloc ret = pool.AllocateMemory(req.alignment, req.size);
	AllocationLoc.Leave();

	// make sure we are not over-allocating, and return
	n_assert(ret.offset + ret.size < pool.maxSize);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
Alloc
AllocateMemory(const VkDevice dev, const VkBuffer& buf, MemoryPoolType type)
{
	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(dev, buf, &req);

	VkMemoryPropertyFlags flags;

	switch (type)
	{
	case MemoryPool_DeviceLocal:
		flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		break;
	case MemoryPool_HostCoherent:
		flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		break;
	case MemoryPool_ManualFlush:
		flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		break;
	default:
		n_crash("AllocateMemory(): Only buffer pool types are allowed for buffer memory");
	}

	uint32_t poolIndex;
	VkResult res = GetMemoryType(req.memoryTypeBits, flags, poolIndex);
	n_assert(res == VK_SUCCESS);
	CoreGraphics::MemoryPool& pool = CoreGraphics::Pools[poolIndex];

	// allocate
	AllocationLoc.Enter();
	Alloc ret = pool.AllocateMemory(req.alignment, req.size);
	AllocationLoc.Leave();

	// make sure we are not over-allocating, and return
	n_assert(ret.offset + ret.size < pool.maxSize);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::Alloc 
AllocateMemory(const VkDevice dev, VkMemoryRequirements reqs, VkDeviceSize allocSize)
{
	uint32_t poolIndex;
	VkResult res = GetMemoryType(reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, poolIndex);
	n_assert(res == VK_SUCCESS);
	CoreGraphics::MemoryPool& pool = CoreGraphics::Pools[poolIndex];

	// allocate
	AllocationLoc.Enter();
	Alloc ret = pool.AllocateMemory(reqs.alignment, allocSize);
	AllocationLoc.Leave();

	// make sure we are not over-allocating, and return
	n_assert(ret.offset + ret.size < pool.maxSize);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
Flush(const VkDevice dev, const Alloc& alloc)
{
	VkPhysicalDeviceProperties props = Vulkan::GetCurrentProperties();
	CoreGraphics::MemoryPool& pool = CoreGraphics::Pools[alloc.poolIndex];
	VkMappedMemoryRange range;
	range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range.pNext = nullptr;
	range.offset = Math::n_align_down(alloc.offset, props.limits.nonCoherentAtomSize);
	range.size = Math::n_min(
		Math::n_align(alloc.size + (alloc.offset - range.offset), props.limits.nonCoherentAtomSize),
		pool.blockSize);
	range.memory = alloc.mem;
	VkResult res = vkFlushMappedMemoryRanges(dev, 1, &range);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
VkResult
GetMemoryType(uint32_t bits, VkMemoryPropertyFlags flags, uint32_t& index)
{
	VkPhysicalDeviceMemoryProperties props = Vulkan::GetMemoryProperties();
	for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
	{
		if ((bits & 1) == 1)
		{
			if (CheckBits(props.memoryTypes[i].propertyFlags, flags))
			{
				index = i;
				return VK_SUCCESS;
			}
		}
		bits >>= 1;
	}
	return VK_ERROR_FEATURE_NOT_PRESENT;
}

} // namespace Vulkan