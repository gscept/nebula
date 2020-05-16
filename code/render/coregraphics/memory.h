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
	ImageMemory_Local,			/// image memory which should reside only on the GPU
	ImageMemory_Temporary,		/// temporary image memory which resides only on the GPU but will be discarded and allocated
	BufferMemory_Local,			/// buffer memory which should reside only on the GPU
	BufferMemory_Temporary,		/// temporary buffer memory which resides only on the GPU but will be discarded and allocated
	BufferMemory_Dynamic,		/// buffer memory which will frequently be updated by the CPU side but requires an explicit flush
	BufferMemory_Mapped,		/// buffer memory which is slightly slower than Dynamic, but which requires no flushing

	NumMemoryPoolTypes
};

struct Alloc
{
	DeviceMemory mem;
	DeviceSize offset;
	DeviceSize size;
	CoreGraphics::MemoryPoolType poolType;
};

struct AllocRange
{
	DeviceSize offset;
	DeviceSize size;
};

struct AllocationMethod
{
	AllocRange(*Alloc)(Util::Array<AllocRange>*, DeviceSize, DeviceSize);
	bool (*Dealloc)(Util::Array<AllocRange>*, DeviceSize);
};

struct MemoryPool
{
	DeviceMemory mem;
	Util::Array<AllocRange> ranges;
	DeviceSize size;
	AllocationMethod method;
	void* mappedMemory;
};

extern AllocationMethod ConservativeAllocationMethod;
extern AllocationMethod LinearAllocationMethod;
extern MemoryPool ImageLocalPool;
extern MemoryPool ImageTemporaryPool;
extern MemoryPool BufferLocalPool;
extern MemoryPool BufferTemporaryPool;
extern MemoryPool BufferDynamicPool;
extern MemoryPool BufferMappedPool;
extern Threading::CriticalSection AllocationLoc;

AllocRange AllocRangeConservative(Util::Array<AllocRange>* ranges, DeviceSize alignment, DeviceSize size);
bool DeallocRangeConservative(Util::Array<AllocRange>* ranges, DeviceSize offset);

AllocRange AllocRangeLinear(Util::Array<AllocRange>* ranges, DeviceSize alignment, DeviceSize size);
bool DeallocRangeLinear(Util::Array<AllocRange>* ranges, VkDeviceSize offset);

/// setup memory pools
void SetupMemoryPools(
	DeviceSize imageMemoryLocal,
	DeviceSize imageMemoryTemporary,
	DeviceSize bufferMemoryLocal,
	DeviceSize bufferMemoryTemporary,
	DeviceSize bufferMemoryDynamic,
	DeviceSize bufferMemoryMapped);
/// discard memory pools
void DiscardMemoryPools(VkDevice dev);

/// free memory
void FreeMemory(const CoreGraphics::Alloc& alloc);
/// get mapped memory pointer
void* GetMappedMemory(CoreGraphics::MemoryPoolType type);

} // namespace CoreGraphics
