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
    DeviceSize hostLocalMemory,
    DeviceSize deviceToHostMemory,
    DeviceSize hostToDeviceMemory)
{
    VkPhysicalDeviceMemoryProperties props = Vulkan::GetMemoryProperties();

    // setup a pool for every memory type
    CoreGraphics::Pools.Resize(VK_MAX_MEMORY_TYPES);
    bool deviceLocalFound = false, hostLocalFound = false, hostToDeviceFound = false, deviceToHostFound = false;
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
    {
        CoreGraphics::MemoryPool& pool = CoreGraphics::Pools[i];
        pool.maxSize = props.memoryHeaps[props.memoryTypes[i].heapIndex].size;
        pool.memoryType = i;
        pool.mapMemory = false;
        pool.blockSize = 0;
        pool.size = 0;

        if (AllBits(props.memoryTypes[i].propertyFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && !deviceLocalFound)
        {
            // device memory is persistent, allocate conservatively
            pool.mapMemory = false;
            pool.blockSize = deviceLocalMemory;
            pool.allocMethod = MemoryPool::MemoryPool_AllocConservative;
            deviceLocalFound = true;
        }
        else if (AllBits(props.memoryTypes[i].propertyFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) && !hostLocalFound)
        {
            // host memory is used for transient transfer buffers, make it allocate and deallocate fast
            pool.mapMemory = true;
            pool.blockSize = hostLocalMemory;
            pool.allocMethod = MemoryPool::MemoryPool_AllocLinear;
            hostLocalFound = true;
        }
        else if (AllBits(props.memoryTypes[i].propertyFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT) && !deviceToHostFound)
        {
            // memory used to read from the GPU should also be conservative
            pool.mapMemory = true;
            pool.blockSize = deviceToHostMemory;
            pool.allocMethod = MemoryPool::MemoryPool_AllocConservative;
            deviceToHostFound = true;
        }
        else if (AllBits(props.memoryTypes[i].propertyFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && !hostToDeviceFound)
        {
            // memory used to directly write to the device with a flush should be allocated conservatively
            pool.mapMemory = true;
            pool.blockSize = hostToDeviceMemory;
            pool.allocMethod = MemoryPool::MemoryPool_AllocConservative;
            hostToDeviceFound = true;
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
    AllocationLock.Enter();
    bool res = pool.DeallocateMemory(alloc);
    AllocationLock.Leave();

    N_COUNTER_DECR(N_GPU_MEMORY_COUNTER, alloc.size);

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
MemoryPool::CreateBlock(void** outMappedPtr)
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
	n_assert(mem != nullptr);

    if (this->mapMemory)
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
MemoryPool::DestroyBlock(DeviceMemory mem)
{
	n_assert(mem != nullptr);
    VkDevice dev = GetCurrentDevice();
    if (this->mapMemory)
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
CoreGraphics::Alloc
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
        n_crash("AllocateMemory(): Only image pool types are allowed for image memory");
    }

    uint32_t poolIndex;
    VkResult res = GetMemoryType(req.memoryTypeBits, flags, poolIndex);
    n_assert(res == VK_SUCCESS);
    CoreGraphics::MemoryPool& pool = CoreGraphics::Pools[poolIndex];

    // allocate
    AllocationLock.Enter();
    Alloc ret = pool.AllocateMemory(req.alignment, req.size);
    AllocationLock.Leave();

    N_COUNTER_INCR(N_GPU_MEMORY_COUNTER, ret.size);

    // make sure we are not over-allocating, and return
    n_assert(ret.offset + ret.size < pool.maxSize);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::Alloc
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
    case MemoryPool_HostLocal:
        flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
    case MemoryPool_DeviceToHost:
        flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        break;
    case MemoryPool_HostToDevice:
        flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
    default:
        n_crash("AllocateMemory(): Only buffer pool types are allowed for buffer memory");
    }

    uint32_t poolIndex;
    VkResult res = GetMemoryType(req.memoryTypeBits, flags, poolIndex);
    n_assert(res == VK_SUCCESS);
    CoreGraphics::MemoryPool& pool = CoreGraphics::Pools[poolIndex];

    // allocate
    AllocationLock.Enter();
    Alloc ret = pool.AllocateMemory(req.alignment, req.size);
    AllocationLock.Leave();

    N_COUNTER_INCR(N_GPU_MEMORY_COUNTER, ret.size);

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
    AllocationLock.Enter();
    Alloc ret = pool.AllocateMemory(reqs.alignment, allocSize);
    AllocationLock.Leave();

    N_COUNTER_INCR(N_GPU_MEMORY_COUNTER, ret.size);

    // make sure we are not over-allocating, and return
    n_assert(ret.offset + ret.size < pool.maxSize);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
Flush(const VkDevice dev, const Alloc& alloc, IndexT offset, SizeT size)
{
    VkPhysicalDeviceProperties props = Vulkan::GetCurrentProperties();
    CoreGraphics::MemoryPool& pool = CoreGraphics::Pools[alloc.poolIndex];
    VkMappedMemoryRange range;
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.pNext = nullptr;
    range.offset = Math::align_down(alloc.offset + offset, props.limits.nonCoherentAtomSize);
    uint flushSize = size == NEBULA_WHOLE_BUFFER_SIZE ? alloc.size : Math::min(size, (SizeT)alloc.size);
    range.size = Math::min(
        (VkDeviceSize)Math::align(flushSize + (alloc.offset + offset - range.offset), props.limits.nonCoherentAtomSize),
        pool.blockSize);
    range.memory = alloc.mem;
    VkResult res = vkFlushMappedMemoryRanges(dev, 1, &range);
    n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void 
Invalidate(const VkDevice dev, const CoreGraphics::Alloc& alloc, IndexT offset, SizeT size)
{
    VkPhysicalDeviceProperties props = Vulkan::GetCurrentProperties();
    CoreGraphics::MemoryPool& pool = CoreGraphics::Pools[alloc.poolIndex];
    VkMappedMemoryRange range;
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.pNext = nullptr;
    range.offset = Math::align_down(alloc.offset + offset, props.limits.nonCoherentAtomSize);
    uint flushSize = size == NEBULA_WHOLE_BUFFER_SIZE ? alloc.size : Math::min((VkDeviceSize)size, alloc.size);
    range.size = Math::min(
        (VkDeviceSize)Math::align(flushSize + (alloc.offset + offset - range.offset), props.limits.nonCoherentAtomSize),
        pool.blockSize);
    range.memory = alloc.mem;
    VkResult res = vkInvalidateMappedMemoryRanges(dev, 1, &range);
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
            if (AllBits(props.memoryTypes[i].propertyFlags, flags))
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
