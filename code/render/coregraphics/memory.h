#pragma once
//------------------------------------------------------------------------------
/**
    Graphics memory interface

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "threading/criticalsection.h"
#include "util/array.h"


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
	MemoryPool_DeviceLocal,		/// image memory which should reside only on the GPU
	MemoryPool_ManualFlush,		/// temporary image memory which resides only on the GPU but will be discarded and allocated
	MemoryPool_HostCoherent,	/// buffer memory which should reside only on the GPU

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

	DeviceSize maxSize;
	bool mapMemory;

private:

	// allocate conservatively
	Alloc AllocateConservative(DeviceSize alignment, DeviceSize size);
	// deallocate conservatively
	bool DeallocConservative(const Alloc& alloc);
	// allocate linearly
	Alloc AllocateLinear(DeviceSize alignment, DeviceSize size);
	// deallocate linearly
	bool DeallocLinear(const Alloc& alloc);
	// create new memory block
	DeviceMemory CreateBlock(bool map, void** outMappedPtr);
	// destroy block
	void DestroyBlock(DeviceMemory mem, bool unmap);
};

extern Util::Array<MemoryPool> Pools;
extern Threading::CriticalSection AllocationLoc;

/// setup memory pools
void SetupMemoryPools(
	DeviceSize deviceLocalMemory,
	DeviceSize manuallyFlushedMemory,
	DeviceSize hostCoherentMemory);
/// discard memory pools
void DiscardMemoryPools(VkDevice dev);

/// free memory
void FreeMemory(const CoreGraphics::Alloc& alloc);
/// get mapped memory pointer
void* GetMappedMemory(const CoreGraphics::Alloc& alloc);

} // namespace CoreGraphics
