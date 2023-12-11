//------------------------------------------------------------------------------
//  vkmemory.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "vkmemory.h"
#include "vkgraphicsdevice.h"
namespace CoreGraphics
{

using namespace Vulkan;

N_DECLARE_COUNTER(N_DEVICE_ONLY_GPU_MEMORY, Device Only GPU Memory);
N_DECLARE_COUNTER(N_HOST_ONLY_GPU_MEMORY, Host Only GPU Memory);
N_DECLARE_COUNTER(N_DEVICE_TO_HOST_GPU_MEMORY, Device To Host GPU Memory);
N_DECLARE_COUNTER(N_HOST_TO_DEVICE_GPU_MEMORY, Host To Device GPU Memory);

//------------------------------------------------------------------------------
/**
*/
void 
SetupMemoryPools(
    DeviceSize deviceLocalMemory,
    DeviceSize hostLocalMemory,
    DeviceSize hostCachedMemory,
    DeviceSize deviceAndHostMemory)
{
    VkPhysicalDeviceMemoryProperties props = Vulkan::GetMemoryProperties();

    // setup a pool for every memory type
    CoreGraphics::Pools.Resize(VK_MAX_MEMORY_TYPES);
    bool deviceLocalFound = false, hostLocalFound = false, hostAndDeviceFound = false, hostCachedFound = false;
    for (uint32 i = 0; i < props.memoryHeapCount; i++)
    {
        CoreGraphics::MemoryHeap heap;
        heap.space = props.memoryHeaps[i].size;
        CoreGraphics::Heaps.Append(heap);
    }
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
    {
        CoreGraphics::MemoryPool& pool = CoreGraphics::Pools[i];
        VkMemoryType type = props.memoryTypes[i];
        VkMemoryHeap heap = props.memoryHeaps[type.heapIndex];
        
        pool.heap = &CoreGraphics::Heaps[type.heapIndex];
        pool.maxSize = heap.size;
        pool.memoryType = i;
        pool.mapMemory = false;
        pool.blockSize = 0;
        pool.size = 0;

        if (AllBits(type.propertyFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && !deviceLocalFound)
        {
            // device memory is persistent, allocate conservatively
            pool.mapMemory = false;
            pool.blockSize = deviceLocalMemory;
            deviceLocalFound = true;
            pool.budgetCounter = N_DEVICE_ONLY_GPU_MEMORY;
            N_BUDGET_COUNTER_SETUP(N_DEVICE_ONLY_GPU_MEMORY, pool.maxSize);
        }
        else if (AllBits(type.propertyFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) && !hostLocalFound)
        {
            // host memory is used for transient transfer buffers, make it allocate and deallocate fast
            pool.mapMemory = true;
            pool.blockSize = hostLocalMemory;
            hostLocalFound = true;
            pool.budgetCounter = N_HOST_ONLY_GPU_MEMORY;
            N_BUDGET_COUNTER_SETUP(N_HOST_ONLY_GPU_MEMORY, pool.maxSize);
        }
        else if (AllBits(type.propertyFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT) && !hostCachedFound)
        {
            // memory used to read from the GPU should also be conservative
            pool.mapMemory = true;
            pool.blockSize = hostCachedMemory;
            hostCachedFound = true;
            pool.budgetCounter = N_DEVICE_TO_HOST_GPU_MEMORY;
            N_BUDGET_COUNTER_SETUP(N_DEVICE_TO_HOST_GPU_MEMORY, pool.maxSize);
        }
        else if (AllBits(type.propertyFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && !hostAndDeviceFound)
        {
            // memory used to directly write to the device with a flush should be allocated conservatively
            pool.mapMemory = true;
            pool.blockSize = deviceAndHostMemory;
            hostAndDeviceFound = true;
            pool.budgetCounter = N_HOST_TO_DEVICE_GPU_MEMORY;
            N_BUDGET_COUNTER_SETUP(N_HOST_TO_DEVICE_GPU_MEMORY, pool.maxSize);
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
    n_assert(res);
    N_BUDGET_COUNTER_DECR(pool.budgetCounter, alloc.size);
    AllocationLock.Leave();    
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
    n_assert(this->heap->space >= this->blockSize);
    this->heap->space -= this->blockSize;

    VkDevice dev = GetCurrentDevice();

    VkMemoryAllocateFlagsInfo flags =
    {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        nullptr,
        VkMemoryAllocateFlagBits::VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        0xFFFFFFFF
    };
    VkMemoryAllocateInfo allocInfo =
    {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        &flags,
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
    N_BUDGET_COUNTER_INCR(pool.budgetCounter, ret.size);
    AllocationLock.Leave();

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
    VkPhysicalDeviceProperties props = Vulkan::GetCurrentProperties();

    VkMemoryPropertyFlags flags = 0;

    switch (type)
    {
    case MemoryPool_DeviceLocal:
        flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
    case MemoryPool_HostLocal:
        flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
    case MemoryPool_HostCached:
        flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        break;
    case MemoryPool_DeviceAndHost:
        flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        // Memory needs to be aligned to non coherent atom size for flushing
        req.size = Math::align(req.size, props.limits.nonCoherentAtomSize);
        req.alignment = Math::align(req.alignment, props.limits.nonCoherentAtomSize);
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
    N_BUDGET_COUNTER_INCR(pool.budgetCounter, ret.size);
    AllocationLock.Leave();

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
    N_BUDGET_COUNTER_INCR(pool.budgetCounter, ret.size);
    AllocationLock.Leave();

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
