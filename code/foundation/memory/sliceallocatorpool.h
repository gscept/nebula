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
template <class TYPE, unsigned int POOLSIZE, bool FREEPOOLS = false>
class SliceAllocatorPool
{
public:
	/// constructor
	SliceAllocatorPool();
	/// destructor
	~SliceAllocatorPool();

	/// allocate a slice
	TYPE* Alloc();
	/// allocate a slice
	TYPE* Alloc(int64_t& identifier);
	/// free a slice by pointer
	void Free(TYPE* slice);
	/// free a slice by identifier created by Alloc
	void Free(int64_t identifier);

	/// clear all pools
	void Clear();

private:

	struct Pool
	{
		TYPE elems[POOLSIZE];
		Util::Array<int> free;
		Util::Array<int> used;

		Pool()
		{
			this->free.Reserve(POOLSIZE);
			this->used.Reserve(POOLSIZE);

			IndexT i;
			for (i = POOLSIZE-1; i >= 0; i--) this->free.Append(i);
		}
	};

	IndexT nextFreePool;
	Util::Array<Pool> pools;
};

//------------------------------------------------------------------------------
/**
*/
template <class TYPE, unsigned int POOLSIZE, bool FREEPOOLS>
inline
Memory::SliceAllocatorPool<TYPE, POOLSIZE, FREEPOOLS>::SliceAllocatorPool() :
	nextFreePool(InvalidIndex)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template <class TYPE, unsigned int POOLSIZE, bool FREEPOOLS>
inline
Memory::SliceAllocatorPool<TYPE, POOLSIZE, FREEPOOLS>::~SliceAllocatorPool()
{
	this->Clear();
}

//------------------------------------------------------------------------------
/**
*/
template <class TYPE, unsigned int POOLSIZE, bool FREEPOOLS>
inline TYPE*
Memory::SliceAllocatorPool<TYPE, POOLSIZE, FREEPOOLS>::Alloc()
{
	int64_t id;
	return this->Alloc(id);
}

//------------------------------------------------------------------------------
/**
*/
template <class TYPE, unsigned int POOLSIZE, bool FREEPOOLS>
inline TYPE*
Memory::SliceAllocatorPool<TYPE, POOLSIZE, FREEPOOLS>::Alloc(int64_t& identifier)
{
	TYPE* ret = nullptr;

	// if we know the next free pool, go for it!
	if (this->nextFreePool != InvalidIndex)
	{
		Pool& pool = this->pools[this->nextFreePool];
		const int id = pool.free[pool.free.Size() - 1];
		pool.free.EraseIndex(pool.free.Size() - 1);
		pool.used.Append(id);
		ret = &pool.elems[id];
		identifier = ((int64_t(this->nextFreePool) << 32) & 0xFFFFFFFF00000000) + (pool.used.Size() - 1 & 0x00000000FFFFFFFF);
		if (pool.free.IsEmpty()) this->nextFreePool = InvalidIndex;
		goto skip;
	}
	else
	{
		IndexT i;
		for (i = 0; i < this->pools.Size(); i++)
		{
			// if pool has space
			if (!this->pools[i].free.IsEmpty())
			{
				Pool& pool = this->pools[i];
				const int id = pool.free[pool.free.Size() - 1];
				pool.free.EraseIndex(pool.free.Size() - 1);
				pool.used.Append(id);
				ret = &pool.elems[id];
				identifier = ((int64_t(i) << 32) & 0xFFFFFFFF00000000) + (pool.used.Size() - 1 & 0x00000000FFFFFFFF);
				if (pool.free.IsEmpty()) this->nextFreePool = InvalidIndex;
				else					 this->nextFreePool = i;
				goto skip;
			}
		}
	}
	

	// if we failed to find a free pool, allocate a new one
	if (ret == nullptr)
	{
		this->pools.Append(Pool());
		Pool& pool = this->pools.Back();
		const int id = pool.free[pool.free.Size() - 1];
		pool.free.EraseIndex(pool.free.Size() - 1);
		pool.used.Append(id);
		ret = &pool.elems[id];
		identifier = ((int64_t(this->pools.Size() - 1) << 32) & 0xFFFFFFFF00000000) + (0 & 0x00000000FFFFFFFF);
		this->nextFreePool = this->pools.Size() - 1;
	}
skip:

	n_assert(ret != nullptr);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
template <class TYPE, unsigned int POOLSIZE, bool FREEPOOLS>
inline void
Memory::SliceAllocatorPool<TYPE, POOLSIZE, FREEPOOLS>::Free(TYPE* slice)
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
			IndexT item = pool.used.FindIndex(idx);
			int used = pool.used[item];
			pool.used.EraseIndex(item);
			pool.free.Append(used);
			this->nextFreePool = i;
			
			// if pool is empty, erase it
			if (pool.numUsed == 0 && FREEPOOLS) this->pools.EraseIndex(i);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
template <class TYPE, unsigned int POOLSIZE, bool FREEPOOLS>
inline void
Memory::SliceAllocatorPool<TYPE, POOLSIZE, FREEPOOLS>::Free(int64_t identifier)
{
	IndexT poolId = IndexT((identifier >> 32) & 0x00000000FFFFFFFF);
	IndexT slice = IndexT((identifier) & 0x00000000FFFFFFFF);
	Pool& pool = this->pools[poolId];
	int used = pool.used[slice];
	pool.used.EraseIndex(slice);
	pool.free.Append(used);
	this->nextFreePool = poolId;

	// if pool is empty, erase it
	if (pool.numUsed == 0 && FREEPOOLS) this->pools.EraseIndex(poolId);
}

//------------------------------------------------------------------------------
/**
*/
template <class TYPE, unsigned int POOLSIZE, bool FREEPOOLS>
void
Memory::SliceAllocatorPool<TYPE, POOLSIZE, FREEPOOLS>::Clear()
{
	if (FREEPOOLS)	this->pools.Clear();
	else
	{
		IndexT i;
		for (i = 0; i < this->pools.Size(); i++)
		{
			Pool& pool = this->pools[i];
			pool.free.Clear();
			pool.used.Clear();

			IndexT j;
			for (j = POOLSIZE-1; j >= 0; j--) pool.free.Append(j);
		}
		this->nextFreePool = 0;
	}
}

} // namespace Memory