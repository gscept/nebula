#pragma once
//------------------------------------------------------------------------------
/**
    @file coregraphics/memory.h

    Graphics memory interface

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "threading/criticalsection.h"
#include "util/array.h"
#include "ids/idpool.h"


#if __VULKAN__
#include "vk/vkloader.h"

typedef VkDeviceSize DeviceSize;
typedef VkDeviceMemory DeviceMemory;
#else
#error "coregraphics/memory.h is not supported for the renderer"
#endif

namespace CoreGraphics
{

enum MemoryPoolType
{
    MemoryPool_DeviceLocal,     /// Memory which should reside solely on the GPU
    MemoryPool_HostLocal,       /// Memory which should reside solely on the CPU and is coherent, requires no flush/invalidate
    MemoryPool_HostCached,      /// Memory which is cached on host, meaning memory is flushed with a call to Flush or Invalidate
    MemoryPool_DeviceAndHost,   /// Memory which is visible on both device and host side

    NumMemoryPoolTypes
};

struct Alloc
{
    DeviceMemory mem;
    DeviceSize offset;
    DeviceSize size;
    uint poolIndex;
    uint blockIndex;
};

struct AllocRange
{
    DeviceSize offset;
    DeviceSize size;
};

struct MemoryPool
{
    // make a new allocation
    Alloc AllocateMemory(uint alignment, uint size);
    // deallocate memory
    bool DeallocateMemory(const Alloc& alloc);

    // clear memory pool
    void Clear();

    // get mapped memory
    void* GetMappedMemory(const Alloc& alloc);

    DeviceSize blockSize;
    uint memoryType;
    Ids::IdPool blockPool;
    Util::Array<DeviceMemory> blocks;
    Util::Array<Util::Array<AllocRange>> blockRanges;
    Util::Array<void*> blockMappedPointers;
    DeviceSize size;

    enum AllocationMethod
    {
        MemoryPool_AllocConservative,
        MemoryPool_AllocLinear,
    };

    AllocationMethod allocMethod;

    const char* budgetCounter;
    DeviceSize maxSize;
    bool mapMemory;

private:

    // allocate conservatively
    Alloc AllocateConservative(DeviceSize alignment, DeviceSize size);
    // allocate linearly
    Alloc AllocateLinear(DeviceSize alignment, DeviceSize size);
    // create new memory block
    DeviceMemory CreateBlock(void** outMappedPtr);
    // destroy block
    void DestroyBlock(DeviceMemory mem);
    // allocate an exclusive block
    Alloc AllocateExclusiveBlock(DeviceSize alignment, DeviceSize size);
};

extern Util::Array<MemoryPool> Pools;
extern Threading::CriticalSection AllocationLock;

/// setup memory pools
void SetupMemoryPools(
    DeviceSize deviceLocalMemory,
    DeviceSize hostLocalMemory,
    DeviceSize hostToDeviceMemory,
    DeviceSize deviceToHostMemory);
/// discard memory pools
void DiscardMemoryPools(VkDevice dev);

/// free memory
void FreeMemory(const CoreGraphics::Alloc& alloc);
/// get mapped memory pointer
void* GetMappedMemory(const CoreGraphics::Alloc& alloc);

} // namespace CoreGraphics
