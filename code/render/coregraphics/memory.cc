//------------------------------------------------------------------------------
//  memory.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "memory.h"
#include "math/scalar.h"
namespace CoreGraphics
{

AllocationMethod ConservativeAllocationMethod;
AllocationMethod LinearAllocationMethod;
MemoryPool ImageLocalPool;
MemoryPool ImageTemporaryPool;
MemoryPool BufferLocalPool;
MemoryPool BufferTemporaryPool;
MemoryPool BufferDynamicPool;
MemoryPool BufferMappedPool;
Threading::CriticalSection AllocationLoc;

//------------------------------------------------------------------------------
/**
	Slowest but most conservative memory allocation method:
		Go through the list of all available ranges to find a suitable gap.
		Return that gap.
		Supports releasing and reusing memory
*/
AllocRange
AllocRangeConservative(Util::Array<AllocRange>* ranges, DeviceSize alignment, DeviceSize size)
{
	// walk through and find a hole
	VkDeviceSize offset = 0;
	IndexT i;
	for (i = 0; i < ranges->Size(); i++)
	{
		// get the current offset
		// if the gap between the previous offset and the current
		// one fits our memory, break the loop and return the offset
		// and index, so we can insert a new range for later
		VkDeviceSize curOffset = (*ranges)[i].offset;
		VkDeviceSize curSize = (*ranges)[i].size;
		if (offset > curOffset)
			goto next;
		else if (curOffset - offset >= size)
		{
			// break, this will insert a new range after the previous but before this one
			break;
		}

	next:
		// set the new offset to be the end of the current range
		offset = Math::n_align(curOffset + curSize, alignment);
	}

	// create a new range and insert it into the list
	AllocRange range;
	range.offset = offset;
	range.size = size;
	ranges->Insert(i, range);
	return range;
}

//------------------------------------------------------------------------------
/**
*/
bool
DeallocRangeConservative(Util::Array<AllocRange>* ranges, DeviceSize offset)
{
	for (IndexT i = 0; i < ranges->Size(); i++)
	{
		if ((*ranges)[i].offset = offset)
		{
			ranges->EraseIndex(i);
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------------
/**
	Fast but wasteful allocation method, which always inserts a new block
	after the last range
*/
AllocRange
AllocRangeLinear(Util::Array<AllocRange>* ranges, DeviceSize alignment, DeviceSize size)
{
	AllocRange range;

	if (ranges->Size() > 0)
	{
		const AllocRange& lastRange = ranges->Back();
		range.offset = Math::n_align(lastRange.offset + lastRange.size, alignment);
		range.size = size;
		ranges->Append(range);
		return range;
	}
	else
	{
		range.offset = 0;
		range.size = size;
		ranges->Append(range);
		return range;
	};
}

//------------------------------------------------------------------------------
/**
	Performs a dealloc, but because of the nature of alloc, the memory
	wont get reused until the last range is deallocated
*/
bool
DeallocRangeLinear(Util::Array<AllocRange>* ranges, VkDeviceSize offset)
{
	for (IndexT i = 0; i < ranges->Size(); i++)
	{
		if ((*ranges)[i].offset = offset)
		{
			ranges->EraseIndex(i);
			return true;
		}
	}
	return false;
}

} // namespace CoreGraphics
