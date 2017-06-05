#pragma once
//------------------------------------------------------------------------------
/**
	A slice pool allocator allocates pools of objects but 
	an allocation only returns a single element.

	The allocator pool takes the type of data to be contained, and 
	the size of each pool. 
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/array.h"

namespace Memory
{
template <class TYPE, int POOLSIZE>
class SliceAllocatorPool
{
public:
	/// constructor
	SliceAllocatorPool();
	/// destructor
	virtual ~SliceAllocatorPool();

	/// allocate a slice
	TYPE* Alloc();
	/// free a slice
	void Free(TYPE* slice);

private:

	struct Pool
	{
		TYPE elems[POOLSIZE];
		bool free[POOLSIZE];
		SizeT numUsed;

		Pool() : numUsed(0) 
		{
			memset(this->free, 1, sizeof(this->free))
		};
	};

	Util::Array<Pool> pools;
};

//------------------------------------------------------------------------------
/**
*/
template <class TYPE, int POOLSIZE>
TYPE*
Memory::SliceAllocatorPool<TYPE, POOLSIZE>::Alloc()
{
	TYPE* ret = nullptr;
	IndexT i;
	for (i = 0; i < this->pools.Size(); i++)
	{
		// if pool has space
		if (this->pools[i].numUsed != POOLSIZE)
		{
			Pool& pool = this->pools[i];

			// find free element in pool
			IndexT j;
			for (j = 0; j < POOLSIZE; j++)
			{
				if (pool.free[j])
				{
					ret = pool.elems[j];
					pool.numUsed++;
					pool.free[j] = false;
					return ret;
				}				
			}
		}
	}

	// if we failed to find a free pool, allocate a new one
	if (ret == nullptr)
	{
		this->pools.Append(Pool);
		Pool& pool = this->pools.Back();
		ret = pool.elems[0];
		pool.free[0] = false;
		pool.numUsed++;
	}
}

//------------------------------------------------------------------------------
/**
*/
template <class TYPE, int POOLSIZE>
void
Memory::SliceAllocatorPool<TYPE, POOLSIZE>::Free(TYPE* slice)
{
	IndexT i;
	for (i = 0; i < this->pools.Size(); i++)
	{
		Pool& pool = this->pools[i];
		
		// find the pool, the slice pointer must be bigger or equal to element 0, and less than or equal to the last element
		if (slice >= &pool.elems[0] && slice <= &pools.elems[POOLSIZE - 1])
		{
			// calculate actual index of pool, the offset should give us how many pointers in we are, divide by size of pointer to get actual index
			ptrdiff_t diff = slice - &pool.elems[0];
			IndexT idx = diff / sizeof(void*);
			pool.free[idx] = true;
			pool.numUsed--;
		}
	}
}

} // namespace Memory