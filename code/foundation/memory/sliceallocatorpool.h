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
#include "util/queue.h"
#include "ids/id.h"
#include <tuple>

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
	TYPE* Alloc(Ids::Id64& identifier);
	/// free a slice by pointer
	void Free(TYPE* slice);
	/// free a slice by identifier created by Alloc
	void Free(const Ids::Id64& identifier);

	/// clear all pools
	void Clear();
	/// resets all pools to be available, but does not deallocate memory
	void Reset();

private:

	struct Pool
	{
		TYPE elems[POOLSIZE];
		Util::Array<int> free;
		Util::Array<int> used;


		/// constructor
		Pool()
		{
			this->free.Reserve(POOLSIZE);
			this->used.Reserve(POOLSIZE);

			this->Reset();
		}

		/// reset pool
		void Reset()
		{
			this->used.Clear();
			this->free.Clear();
			IndexT i;
			for (i = POOLSIZE - 1; i >= 0; i--) this->free.Append(i);
		}
	};

	IndexT nextFreePool;
	Util::Queue<std::tuple<int64_t, Pool*>> freePools;
	Util::Array<Pool*> pools;
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
	Core::Id id;
	return this->Alloc(id);
}

//------------------------------------------------------------------------------
/**
*/
template <class TYPE, unsigned int POOLSIZE, bool FREEPOOLS>
inline TYPE*
Memory::SliceAllocatorPool<TYPE, POOLSIZE, FREEPOOLS>::Alloc(Ids::Id64& identifier)
{
	TYPE* ret = nullptr;

	if (!this->freePools.IsEmpty())
	{
		std::tuple<int64_t, Pool*>& elem = this->freePools.Peek();
		Pool* pool = std::get<1>(elem);
		int64_t poolId = std::get<0>(elem);
		const int id = pool->free[pool->free.Size() - 1];
		pool->free.EraseIndex(pool->free.Size() - 1);
		pool->used.Append(id);
		ret = &pool->elems[id];
		identifier.id = Core::Id::MakeId64(poolId, pool->used.Size() - 1);// ((poolId << 32) & 0xFFFFFFFF00000000) + (pool->used.Size() - 1 & 0x00000000FFFFFFFF);
		if (pool->free.IsEmpty()) this->freePools.Dequeue();
	}
	else
	{
		Pool* pool = new Pool();
		int64_t poolId = this->pools.Size();
		this->pools.Append(pool);
		this->freePools.Enqueue(std::make_tuple(poolId, pool));
		const int id = pool->free[pool->free.Size() - 1];
		pool->free.EraseIndex(pool->free.Size() - 1);
		pool->used.Append(id);
		ret = &pool->elems[id];
		identifier.id = Core::Id::MakeId64(poolId, 0);
		//identifier = ((poolId << 32) & 0xFFFFFFFF00000000) + (0 & 0x00000000FFFFFFFF);
	}

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
		std::tuple<int64_t, Pool*>& elem = this->pools[i];
		Pool* pool = std::get<1>(elem);
		
		// find the pool, the slice pointer must be bigger or equal to element 0, and less than or equal to the last element
		if (slice >= &pool->elems[0] && slice <= &pools->elems[POOLSIZE - 1])
		{
			// calculate actual index of pool, the offset should give us how many pointers in we are, divide by size of pointer to get actual index
			ptrdiff_t diff = slice - &pool->elems[0];
			IndexT idx = diff / sizeof(void*);
			IndexT item = pool->used.FindIndex(idx);
			int used = pool->used[item];
			pool->used.EraseIndex(item);
			pool->free.Append(used);
			this->freePools.Enqueue(std::make_tuple(i, pool));
			
			// if pool is empty, erase it
			if (pool->numUsed == 0 && FREEPOOLS)
			{
				this->pools.EraseIndex(i);

				IndexT j;
				for (j = 0; j < this->freePools.Size(); j++)
				{
					int64_t id = std::get<0>(this->freePools[j]);
					if (id == i)
					{
						this->freePools.EraseIndex(j);
						break;
					}
				}

				delete pool;
			}
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
template <class TYPE, unsigned int POOLSIZE, bool FREEPOOLS>
inline void
Memory::SliceAllocatorPool<TYPE, POOLSIZE, FREEPOOLS>::Free(const Ids::Id64& identifier)
{
	IndexT poolId = IndexT((identifier >> 32) & 0x00000000FFFFFFFF);
	IndexT slice = IndexT((identifier) & 0x00000000FFFFFFFF);
	std::tuple<int64_t, Pool*>& elem = this->pools[poolId];
	Pool* pool = std::get<1>(elem);
	int used = pool->used[slice];
	pool->used.EraseIndex(slice);
	pool->free.Append(used);
	this->freePools.Enqueue(std::make_tuple(poolId, pool));

	// if pool is empty, erase it
	if (pool.numUsed == 0 && FREEPOOLS)
	{
		this->pools.EraseIndex(poolId);

		IndexT j;
		for (j = 0; j < this->freePools.Size(); j++)
		{
			int64_t id = std::get<0>(this->freePools[j]);
			if (id == poolId)
			{
				this->freePools.EraseIndex(j);
				break;
			}
		}

		delete pool;
	}
}

//------------------------------------------------------------------------------
/**
*/
template <class TYPE, unsigned int POOLSIZE, bool FREEPOOLS>
void
Memory::SliceAllocatorPool<TYPE, POOLSIZE, FREEPOOLS>::Clear()
{
	IndexT i;
	for (i = 0; i < this->pools.Size(); i++)
	{
		delete this->pools[i];
	}
	this->pools.Clear();
	this->freePools.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template <class TYPE, unsigned int POOLSIZE, bool FREEPOOLS /*= false*/>
void
Memory::SliceAllocatorPool<TYPE, POOLSIZE, FREEPOOLS>::Reset()
{
	this->freePools.Clear();
	IndexT i;
	for (i = 0; i < this->pools.Size(); i++)
	{
		Pool* pool = this->pools[i];
		pool->Reset();

		this->freePools.Enqueue(std::make_tuple(i, pool));
	}
}

} // namespace Memory