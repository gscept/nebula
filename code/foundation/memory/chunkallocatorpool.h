#pragma once
//------------------------------------------------------------------------------
/**
	A chunk pool allocator returns an array of objects, 
	however the chunk must be smaller than the size of a single pool.

	This allocator needs to be implemented, 
	keeping track of blocks is harder than single elements...
	
	(C) 2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

namespace Memory
{
template <class TYPE, int POOLSIZE>
class ChunkAllocatorPool
{
public:
	/// constructor
	ChunkAllocatorPool();
	/// destructor
	virtual ~ChunkAllocatorPool();

	/// allocate a chunk of elements with a given size
	TYPE* Alloc(SizeT num);
	/// free a slice
	void Free(TYPE* slice, SizeT num);

private:

	struct Pool
	{
		TYPE elems[POOLSIZE];
		bool free[POOLSIZE];
		int block[POOLSIZE];
		SizeT numUsed;

		Pool() : numUsed(0)
		{
			memset(this->free, 1, sizeof(this->free));
			memset(this->block, 0, sizeof(this->block));
		};
	};

	Util::Array<Pool> pools;
};

//------------------------------------------------------------------------------
/**
*/
template <class TYPE, int POOLSIZE>
TYPE*
Memory::ChunkAllocatorPool<TYPE, POOLSIZE>::Alloc(SizeT num)
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
	}
}

//------------------------------------------------------------------------------
/**
*/
template <class TYPE, int POOLSIZE>
void
Memory::ChunkAllocatorPool<TYPE, POOLSIZE>::Free(TYPE* slice, SizeT num)
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