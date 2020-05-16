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
	DeviceSize imageMemoryLocal,
	DeviceSize imageMemoryTemporary,
	DeviceSize bufferMemoryLocal,
	DeviceSize bufferMemoryTemporary,
	DeviceSize bufferMemoryDynamic,
	DeviceSize bufferMemoryMapped)
{
	VkPhysicalDeviceMemoryProperties props = Vulkan::GetMemoryProperties();

	ConservativeAllocationMethod.Alloc = AllocRangeConservative;
	ConservativeAllocationMethod.Dealloc = DeallocRangeConservative;
	LinearAllocationMethod.Alloc = AllocRangeLinear;
	LinearAllocationMethod.Dealloc = DeallocRangeLinear;

	uint32_t lMaxSize, dMaxSize, mMaxSize;

	uint32_t lType = UINT32_MAX, dType = UINT32_MAX, mType = UINT32_MAX;
	for (uint32_t i = 0; i < props.memoryTypeCount; i++)
	{
		VkMemoryType type = props.memoryTypes[i];
		if (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			&& lType == UINT32_MAX)
		{
			lType = i;
			lMaxSize = props.memoryHeaps[type.heapIndex].size;
		}
		else if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			&& type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT
			&& dType == UINT32_MAX)
		{
			dType = i;
			dMaxSize = props.memoryHeaps[type.heapIndex].size;
		}
		else if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			&& type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			&& mType == UINT32_MAX)
		{
			mType = i;
			mMaxSize = props.memoryHeaps[type.heapIndex].size;
		}
	}
	n_assert(lType != UINT32_MAX);
	n_assert(dType != UINT32_MAX);
	n_assert(mType != UINT32_MAX);
	n_assert(lMaxSize > imageMemoryLocal + bufferMemoryLocal);
	n_assert(dMaxSize > bufferMemoryDynamic);
	n_assert(mMaxSize > bufferMemoryMapped);
	VkDevice dev = GetCurrentDevice();

	VkMemoryAllocateInfo allocInfo =
	{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr,
		0,
		0
	};
	VkResult res;

	allocInfo.allocationSize = imageMemoryLocal;
	allocInfo.memoryTypeIndex = lType;
	res = vkAllocateMemory(dev, &allocInfo, nullptr, &ImageLocalPool.mem);
	n_assert(res == VK_SUCCESS);
	ImageLocalPool.size = allocInfo.allocationSize;
	ImageLocalPool.method = ConservativeAllocationMethod;
	ImageLocalPool.mappedMemory = nullptr;

	allocInfo.allocationSize = imageMemoryTemporary;
	allocInfo.memoryTypeIndex = lType;
	res = vkAllocateMemory(dev, &allocInfo, nullptr, &ImageTemporaryPool.mem);
	n_assert(res == VK_SUCCESS);
	ImageTemporaryPool.size = allocInfo.allocationSize;
	ImageTemporaryPool.method = LinearAllocationMethod;
	ImageTemporaryPool.mappedMemory = nullptr;

	allocInfo.allocationSize = bufferMemoryLocal;
	allocInfo.memoryTypeIndex = lType;
	res = vkAllocateMemory(dev, &allocInfo, nullptr, &BufferLocalPool.mem);
	n_assert(res == VK_SUCCESS);
	BufferLocalPool.size = allocInfo.allocationSize;
	BufferLocalPool.method = ConservativeAllocationMethod;
	BufferLocalPool.mappedMemory = nullptr;

	allocInfo.allocationSize = bufferMemoryDynamic;
	allocInfo.memoryTypeIndex = dType;
	res = vkAllocateMemory(dev, &allocInfo, nullptr, &BufferDynamicPool.mem);
	n_assert(res == VK_SUCCESS);
	BufferDynamicPool.size = allocInfo.allocationSize;
	BufferDynamicPool.method = ConservativeAllocationMethod;
	res = vkMapMemory(dev, BufferDynamicPool.mem, 0, VK_WHOLE_SIZE, 0, &BufferDynamicPool.mappedMemory);
	n_assert(res == VK_SUCCESS);

	allocInfo.allocationSize = bufferMemoryMapped;
	allocInfo.memoryTypeIndex = mType;
	res = vkAllocateMemory(dev, &allocInfo, nullptr, &BufferMappedPool.mem);
	n_assert(res == VK_SUCCESS);
	BufferMappedPool.size = allocInfo.allocationSize;
	BufferMappedPool.method = ConservativeAllocationMethod;
	res = vkMapMemory(dev, BufferMappedPool.mem, 0, VK_WHOLE_SIZE, 0, &BufferMappedPool.mappedMemory);
	n_assert(res == VK_SUCCESS);

	allocInfo.allocationSize = bufferMemoryTemporary;
	allocInfo.memoryTypeIndex = mType;
	res = vkAllocateMemory(dev, &allocInfo, nullptr, &BufferTemporaryPool.mem);
	n_assert(res == VK_SUCCESS);
	BufferTemporaryPool.size = allocInfo.allocationSize;
	BufferTemporaryPool.method = LinearAllocationMethod;
	res = vkMapMemory(dev, BufferTemporaryPool.mem, 0, VK_WHOLE_SIZE, 0, &BufferTemporaryPool.mappedMemory);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void 
DiscardMemoryPools(VkDevice dev)
{
	vkFreeMemory(dev, ImageLocalPool.mem, nullptr);
	vkFreeMemory(dev, ImageTemporaryPool.mem, nullptr);
	vkFreeMemory(dev, BufferLocalPool.mem, nullptr);
	vkFreeMemory(dev, BufferTemporaryPool.mem, nullptr);
	vkFreeMemory(dev, BufferDynamicPool.mem, nullptr);
	vkFreeMemory(dev, BufferMappedPool.mem, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
void
FreeMemory(const Alloc& alloc)
{
	Util::Array<AllocRange>* ranges = nullptr;
	VkDeviceMemory mem = nullptr;
	AllocationMethod* method = nullptr;

	switch (alloc.poolType)
	{
	case ImageMemory_Local:
		ranges = &ImageLocalPool.ranges;
		mem = ImageLocalPool.mem;
		method = &ImageLocalPool.method;
		break;
	case ImageMemory_Temporary:
		ranges = &ImageTemporaryPool.ranges;
		mem = ImageTemporaryPool.mem;
		method = &ImageTemporaryPool.method;
		break;
	case BufferMemory_Local:
		ranges = &BufferLocalPool.ranges;
		mem = BufferLocalPool.mem;
		method = &BufferLocalPool.method;
		break;
	case BufferMemory_Temporary:
		ranges = &BufferTemporaryPool.ranges;
		mem = BufferTemporaryPool.mem;
		method = &BufferTemporaryPool.method;
		break;
	case BufferMemory_Dynamic:
		ranges = &BufferDynamicPool.ranges;
		mem = BufferDynamicPool.mem;
		method = &BufferDynamicPool.method;
		break;
	case BufferMemory_Mapped:
		ranges = &BufferMappedPool.ranges;
		mem = BufferMappedPool.mem;
		method = &BufferMappedPool.method;
		break;
	default:
		n_crash("AllocateMemory(): Unknown pool type");
	}

	// dealloc
	AllocationLoc.Enter();
	bool res = method->Dealloc(ranges, alloc.offset);
	AllocationLoc.Leave();
	n_assert(res);
}

//------------------------------------------------------------------------------
/**
*/
void* GetMappedMemory(MemoryPoolType type)
{
	switch (type)
	{
	case ImageMemory_Local:
	case ImageMemory_Temporary:
	case BufferMemory_Local:
		n_crash("Image and local buffer memory doesn't support mapping");
		return nullptr;
	case BufferMemory_Temporary:
		return BufferTemporaryPool.mappedMemory;
	case BufferMemory_Dynamic:
		return BufferDynamicPool.mappedMemory;
	case BufferMemory_Mapped:
		return BufferMappedPool.mappedMemory;
	default:
		n_crash("AllocateMemory(): Unknown pool type");
		return nullptr;
	}
}

} // namespace CoreGraphics


namespace Vulkan
{

using namespace CoreGraphics;

// undefine when we know all memory allocations are acceptable
#define VK_MEMORY_TEST_TYPE 1
#if VK_MEMORY_TEST_TYPE
//------------------------------------------------------------------------------
/**
*/
bool
TestMemoryType(VkMemoryRequirements req, VkMemoryPropertyFlags flags)
{
	VkPhysicalDeviceMemoryProperties props = Vulkan::GetMemoryProperties();
	uint32_t bits = req.memoryTypeBits;
	for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
	{
		if ((bits & 1) == 1)
		{
			if ((props.memoryTypes[i].propertyFlags & flags) == flags)
			{
				return true;
			}
		}
		bits >>= 1;
	}
	return false;
}
#else
//------------------------------------------------------------------------------
/**
*/
bool TestMemoryType(VkMemoryRequirements req, VkMemoryPropertyFlags flags)
{
	return true;
}
#endif

//------------------------------------------------------------------------------
/**
*/
Alloc
AllocateMemory(const VkDevice dev, const VkImage& img, MemoryPoolType type)
{
	VkMemoryRequirements req;
	vkGetImageMemoryRequirements(dev, img, &req);

	// get ranges
	Util::Array<AllocRange>* ranges = nullptr;
	VkDeviceMemory mem = VK_NULL_HANDLE;
	AllocationMethod* method = nullptr;
	VkDeviceSize size = 0;

	switch (type)
	{
	case ImageMemory_Local:
		ranges = &ImageLocalPool.ranges;
		mem = ImageLocalPool.mem;
		method = &ImageLocalPool.method;
		size = ImageLocalPool.size;
		n_assert(TestMemoryType(req, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
		break;
	case ImageMemory_Temporary:
		ranges = &ImageTemporaryPool.ranges;
		mem = ImageTemporaryPool.mem;
		method = &ImageTemporaryPool.method;
		size = ImageTemporaryPool.size;
		n_assert(TestMemoryType(req, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
		break;
	default:
		n_crash("AllocateMemory(): Only image pool types are allowed for image memory");
	}

	// find a new range
	AllocationLoc.Enter();
	AllocRange range = method->Alloc(ranges, req.alignment, req.size);
	AllocationLoc.Leave();
	n_assert(range.offset + range.size < size);

	Alloc ret{ mem, range.offset, range.size, type };
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

	Util::Array<AllocRange>* ranges = nullptr;
	VkDeviceMemory mem = nullptr;
	AllocationMethod* method = nullptr;
	VkDeviceSize size = 0;

	switch (type)
	{
	case BufferMemory_Local:
		ranges = &BufferLocalPool.ranges;
		mem = BufferLocalPool.mem;
		method = &BufferLocalPool.method;
		size = BufferLocalPool.size;
		n_assert(TestMemoryType(req, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
		break;
	case BufferMemory_Temporary:
		ranges = &BufferTemporaryPool.ranges;
		mem = BufferTemporaryPool.mem;
		method = &BufferTemporaryPool.method;
		size = BufferTemporaryPool.size;
		n_assert(TestMemoryType(req, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		break;
	case BufferMemory_Dynamic:
		ranges = &BufferDynamicPool.ranges;
		mem = BufferDynamicPool.mem;
		method = &BufferDynamicPool.method;
		size = BufferDynamicPool.size;
		n_assert(TestMemoryType(req, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT));
		break;
	case BufferMemory_Mapped:
		ranges = &BufferMappedPool.ranges;
		mem = BufferMappedPool.mem;
		method = &BufferMappedPool.method;
		size = BufferMappedPool.size;
		n_assert(TestMemoryType(req, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		break;
	default:
		n_crash("AllocateMemory(): Only buffer pool types are allowed for buffer memory");
	}

	// find a new range
	AllocationLoc.Enter();
	AllocRange range = method->Alloc(ranges, req.alignment, req.size);
	AllocationLoc.Leave();
	n_assert(range.offset + range.size < size);

	Alloc ret{ mem, range.offset, range.size, type };
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::Alloc 
AllocateMemory(const VkDevice dev, VkDeviceSize alignment, VkDeviceSize allocSize)
{
	Util::Array<AllocRange>* ranges = &ImageLocalPool.ranges;
	VkDeviceMemory mem = ImageLocalPool.mem;
	AllocationMethod* method = &ImageLocalPool.method;
	VkDeviceSize size = ImageLocalPool.size;

	// find a new range
	AllocationLoc.Enter();
	AllocRange range = method->Alloc(ranges, alignment, allocSize);
	AllocationLoc.Leave();
	n_assert(range.offset + range.size < size);

	Alloc ret{ mem, range.offset, range.size, ImageMemory_Local };
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
Flush(const VkDevice dev, const Alloc& alloc)
{
	VkPhysicalDeviceProperties props = Vulkan::GetCurrentProperties();
	VkMappedMemoryRange range;
	range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range.pNext = nullptr;
	range.offset = Math::n_align_down(alloc.offset, props.limits.nonCoherentAtomSize);
	range.size = Math::n_align(alloc.size, props.limits.nonCoherentAtomSize);
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
			if ((props.memoryTypes[i].propertyFlags & flags) == flags)
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