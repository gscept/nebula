//------------------------------------------------------------------------------
// stretchybuffer.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "stretchybuffer.h"

namespace CoreGraphics
{

__ImplementAbstractClass(CoreGraphics::StretchyBuffer, 'STBU', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
StretchyBuffer::StretchyBuffer() :
	size(0),
	stride(0),
	grow(8)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
StretchyBuffer::~StretchyBuffer()
{
	// empty
}

#define ROUND_TO_POW(n, p) ((n + p - 1) & ~(p - 1))
//------------------------------------------------------------------------------
/**
	Performs an expansion of the uniform buffer, based on how many instances we want to allocate.
	If the buffer is full it is expanded to a new size.
	If the buffer has free instances we reuse them by returning the offset into that buffer.
	If a continuous range of numInstances fits in the free list, the offset is the offset to the first element.
	If there is no continuous range fitting numInstances, alloc and return offset without disturbing other free instances.
*/
SizeT
StretchyBuffer::AllocateInstance(SizeT numInstances /*= 1*/)
{
	n_assert(numInstances > 0);
	SizeT offset;

	// pool is exhausted, allocate new elements
	if (this->freeIndices.IsEmpty())
	{
		// calculate new capacity by number of instances, or grow number if bigger
		SizeT capacity = this->usedIndices.Size();

		// calculate current offset
		offset = ROUND_TO_POW(capacity * this->stride, this->byteAlignment);
		uint32_t alignedSize = this->Grow(capacity, numInstances);

		// add instance to used indices
		IndexT i;
		for (i = 0; i < numInstances; i++)
		{
			this->usedIndices.Append(capacity + i);
		}
	}
	else
	{
		// if pool is not exhausted, return index
		if (numInstances == 1)
		{
			// grab index from free indices list
			IndexT index = this->freeIndices.Front();
			this->freeIndices.EraseIndex(0);

			// return offset
			offset = ROUND_TO_POW(index * this->stride, this->byteAlignment);
			this->usedIndices.Append(index);
		}
		else if (this->freeIndices.Size() > numInstances) // pool could possibly fit range, find range
		{
			// try to find range of instances
			SizeT validRangeCount = 1;
			IndexT freeIndex = this->freeIndices.Front();
			IndexT i;
			for (i = 1; i < this->freeIndices.Size(); i++)
			{
				IndexT idx = this->freeIndices[i];
				if (idx - 1 == freeIndex) validRangeCount++;
				else
				{
					validRangeCount = 1;
				}

				// if a valid range is found, find the offset and return
				if (validRangeCount == numInstances)
				{
					// find offset to first index
					offset = ROUND_TO_POW((idx - numInstances) * this->stride, this->byteAlignment);
					break;
				}
				freeIndex = idx;
			}

			// if no range was found, alloc
			if (validRangeCount != numInstances)
			{
				SizeT capacity = this->usedIndices.Size() + this->freeIndices.Size();

				// calculate offset
				offset = ROUND_TO_POW(capacity * this->stride, this->byteAlignment);
				uint32_t alignedSize = this->Grow(capacity, numInstances);

				// add instance to used indices
				IndexT i;
				for (i = 0; i < numInstances; i++)
				{
					this->usedIndices.Append(capacity + i);
				}
			}
		}
		else // fitting is impossible, but pool is not exhausted, so grow!
		{
			SizeT capacity = this->usedIndices.Size() + this->freeIndices.Size();

			// calculate offset
			offset = ROUND_TO_POW(capacity * this->stride, this->byteAlignment);
			uint32_t alignedSize = this->Grow(capacity, numInstances);

			// add instance to used indices
			IndexT i;
			for (i = 0; i < numInstances; i++)
			{
				this->usedIndices.Append(capacity + i);
			}
		}
	}

	return offset;
}

//------------------------------------------------------------------------------
/**
*/
void
StretchyBuffer::FreeInstance(SizeT offset)
{
	n_assert(offset >= 0);

	IndexT instance = offset / this->stride;
	IndexT idx = this->usedIndices.FindIndex(instance);
	n_assert(idx != InvalidIndex);
	this->usedIndices.EraseIndex(idx);
	this->freeIndices.Append(instance);
}

//------------------------------------------------------------------------------
/**
*/
void
StretchyBuffer::SetFree(uint start, SizeT num)
{
	IndexT i;
	for (i = 0; i < num; i++)
	{
		this->freeIndices.Append(start + i);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
StretchyBuffer::SetFull()
{
	// move free indices to used indices
	IndexT i;
	for (i = 0; i < this->freeIndices.Size(); i++)
	{
		this->usedIndices.Append(i);
	}
	this->freeIndices.Clear();
}

} // namespace CoreGraphics