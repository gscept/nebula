#pragma once
//------------------------------------------------------------------------------
/**
	A stretchy buffer implements a GPU memory pool, which uses the same buffer
	and expands its memory backing as needed. Implement the Grow function
	in the implementation class to perform the resize.

	The StretchyBuffer needs to be attached to the object which holds the memory,
	and that type must implement the Grow function. The Grow function
	takes two arguments, the old size and the extra bytes to allocate.

	When constructing, the stride is the size for each element, and alignment
	is the required byte alignment per element.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
namespace CoreGraphics
{
template <class T>
class StretchyBuffer
{
public:
	/// constructor
	StretchyBuffer();
	/// destructor
	~StretchyBuffer();

	/// setup stretchy buffer on a target
	void Setup(T* target, SizeT stride, SizeT alignment, SizeT numElements);

	/// set all free indices to be considered used up
	void SetFull();
	/// set all indices to be free
	void SetEmpty();
	/// set range of values to be considered free
	void SetFree(uint start, SizeT num);
	/// allocates instance memory, and returns offset into buffer at new instance
	SizeT AllocateInstance(SizeT numInstances = 1);
	/// deallocates instance memory
	void FreeInstance(SizeT offset);
protected:

	Util::Array<IndexT> freeIndices;
	Util::Array<IndexT> usedIndices;
	SizeT stride;
	SizeT byteAlignment;
	T* target;
};


//------------------------------------------------------------------------------
/**
*/
template <class T> inline
StretchyBuffer<T>::StretchyBuffer() :
	target(nullptr),
	stride(0),
	byteAlignment(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template <class T> inline
StretchyBuffer<T>::~StretchyBuffer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template<class T> inline 
void StretchyBuffer<T>::Setup(T* target, SizeT stride, SizeT alignment, SizeT numElements)
{
	this->target = target;
	this->stride = stride;
	this->byteAlignment = alignment;

	for (IndexT i = 0; i < numElements; i++)
		this->freeIndices.Append(i);
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
template <class T>
inline SizeT
StretchyBuffer<T>::AllocateInstance(SizeT numInstances)
{
	n_assert(numInstances > 0);
	n_assert(this->target != nullptr);
	SizeT offset;

	// pool is exhausted, allocate new elements
	if (this->freeIndices.IsEmpty())
	{
		// calculate new capacity by number of instances, or grow number if bigger
		SizeT capacity = this->usedIndices.Size();

		// calculate current offset
		SizeT newCapacity;
		offset = ROUND_TO_POW(capacity * this->stride, this->byteAlignment);
		uint32_t alignedSize = this->target->Grow(capacity, numInstances, newCapacity);
		this->SetFree(capacity + numInstances, newCapacity - capacity - numInstances);

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
				SizeT newCapacity;
				offset = ROUND_TO_POW(capacity * this->stride, this->byteAlignment);
				uint32_t alignedSize = this->target->Grow(capacity, numInstances, newCapacity);
				this->SetFree(capacity + numInstances, newCapacity - capacity - numInstances);

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
			SizeT newCapacity;
			offset = ROUND_TO_POW(capacity * this->stride, this->byteAlignment);
			uint32_t alignedSize = this->target->Grow(capacity, numInstances, newCapacity);
			this->SetFree(capacity + numInstances, newCapacity - capacity - numInstances);

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
template <class T>
inline void
StretchyBuffer<T>::FreeInstance(SizeT offset)
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
template <class T>
inline void
StretchyBuffer<T>::SetFree(uint start, SizeT num)
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
template <class T>
inline void
StretchyBuffer<T>::SetFull()
{
	// move free indices to used indices
	IndexT i;
	for (i = 0; i < this->freeIndices.Size(); i++)
	{
		this->usedIndices.Append(i);
	}
	this->freeIndices.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<class T>
inline void StretchyBuffer<T>::SetEmpty()
{
	// move free indices to used indices
	IndexT i;
	for (i = 0; i < this->usedIndices.Size(); i++)
	{
		this->freeIndices.Append(this->usedIndices[i]);
	}
	this->usedIndices.Clear();
}

} // namespace CoreGraphics