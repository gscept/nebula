#pragma once
//------------------------------------------------------------------------------
/**
    @class Memory::RangeAllocator
    
    An allocator that reserves a range of memory and outputs an offset to that
    range. Doesn't allocate any memory but is supposed to be used on a single
    memory block. 

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "util/array.h"
#include "math/scalar.h"
namespace Memory
{

class RangeAllocator
{
public:

    /// constructor
    RangeAllocator();
    /// destructor
    ~RangeAllocator();

    /// resize allocator buffer with number of elements
    void Resize(SizeT numElements);

    /// allocate a range of memory and return the index, returns false if failed because range could not be found
    bool Alloc(SizeT numElements, SizeT alignment, IndexT& outIndex);
    /// deallocate a range of memory
    SizeT Dealloc(IndexT startIndex);
private:
    struct Range
    {
        IndexT start;
        SizeT length;
    };
    Util::Dictionary<IndexT, Range> ranges;
    SizeT size;
};

//------------------------------------------------------------------------------
/**
*/
inline 
RangeAllocator::RangeAllocator()
    : size(0)
{
}

//------------------------------------------------------------------------------
/**
*/
inline 
RangeAllocator::~RangeAllocator()
{
}

//------------------------------------------------------------------------------
/**
*/
inline void 
RangeAllocator::Resize(SizeT numElements)
{
    this->size = numElements;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
RangeAllocator::Alloc(SizeT numElements, SizeT alignment, IndexT& outIndex)
{
    bool ret = false;
    IndexT currentOffset = 0;
    IndexT i;
    for (i = 0; i < this->ranges.Size(); i++)
    {
        const Range& range = this->ranges.ValueAtIndex(i);

        if (currentOffset > range.start)
            goto next;
        else if (range.start - currentOffset >= size)
            break;

    next:
        currentOffset = Math::align(range.start + range.length, alignment);
    }

    // if we can't fit the range, return invalid index and false
    if (currentOffset + numElements > this->size)
        outIndex = InvalidIndex;
    else
    {
        // otherwise, return true, output index and insert range
        ret = true;
        outIndex = currentOffset;

        this->ranges.Add(currentOffset, Range{ currentOffset, numElements });
    }

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT 
RangeAllocator::Dealloc(IndexT startIndex)
{
    IndexT index = this->ranges.FindIndex(startIndex);
    if (index != InvalidIndex)
    {
        SizeT ret = this->ranges.ValueAtIndex(index).length;
        this->ranges.EraseAtIndex(index);
        return ret;
    }
    n_error("Tried to dealloc non-existant range");
    return 0;
}

} // namespace Memory
