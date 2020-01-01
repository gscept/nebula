#pragma once
//------------------------------------------------------------------------------
/**
	This simple Id pool implements a set of free and used consecutive integers.
	The pool can be configured to only release new Ids up to a certain maximum,
	after which it asserts. 

	Essentially, it allocates a certain amount of new Ids when needed 
	(not following the Array method of doubling its size) and can recycle
	the indices for later. Unlike the IdGenerationPool, this one does not
	keep track of how many times an id has been reused, so use this with
	caution!

	Maximum amount of bits produced by this system is 32.
	
	(C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <cstdint>
#include "core/types.h"
#include "util/array.h"
#include "id.h"
#include "math/scalar.h"
#include <functional>
namespace Ids
{
class IdPool
{
public:
	/// constructor
	IdPool();
	/// constructor with maximum size
	IdPool(const uint32_t max, const uint32_t grow = 512);
	/// destructor
	~IdPool();

	/// get new id
	uint32_t Alloc();
	/// free id
	void Dealloc(uint32_t id);
	/// get number of active ids
	uint32_t GetNumUsed() const;
	/// get number of free elements
	uint32_t GetNumFree() const;
	/// iterate free indices
	void ForEachFree(const std::function<void(uint32_t, uint32_t)> fun, SizeT num);
	/// frees up lhs and erases rhs
	void Move(uint32_t lhs, uint32_t rhs);
	/// get grow
	const uint32_t GetGrow() const;
private:

	Util::Array<uint32_t> free;
	uint32_t maxId;
	uint32_t grow;
};

//------------------------------------------------------------------------------
/**
*/
inline
IdPool::IdPool() :
	maxId(0xFFFFFFFF),		// int max
	grow(512)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
inline
IdPool::IdPool(const uint32_t max, const uint32_t grow) :
	maxId(max),
	grow(grow)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
inline
IdPool::~IdPool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
inline uint32_t
IdPool::Alloc()
{
	// if we're out of ids, allocate more, but with a controlled grow (not log2 as per Array)
	if (this->free.Size() == 0)
	{
		// calculate how many more indices we are allowed to get
		SizeT growTo = Math::n_min(this->maxId, this->grow);
		SizeT oldCapacity = this->free.Capacity();

		// make sure we don't allocate too many indices
		n_assert2((uint32_t)(oldCapacity + growTo) < this->maxId, "Pool is full! Be careful with how much you allocate!\n");

		// reserve more space for new Ids
		this->free.Reserve(oldCapacity + growTo);
		IndexT i;
		for (i = this->free.Capacity()-1; i >= oldCapacity; i--)
		{
			// count backwards from the max id
			this->free.Append(this->maxId - i);
		}
	}

	// if we do an inverse erase, we don't have to move elements, and since we know the max id, subtract it
	uint32_t id = this->maxId - this->free.Back();
	this->free.EraseIndex(this->free.Size() - 1);
	return id;
}

//------------------------------------------------------------------------------
/**
*/
inline void
IdPool::Dealloc(uint32_t id)
{
	this->free.Append(this->maxId - id);
}

//------------------------------------------------------------------------------
/**
*/
inline uint32_t
IdPool::GetNumUsed() const
{
	return this->free.Capacity() - this->free.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline uint32_t
IdPool::GetNumFree() const
{
	return this->free.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline void
IdPool::ForEachFree(const std::function<void(uint32_t, uint32_t)> fun, SizeT num)
{
	SizeT size = this->free.Size();
	for (IndexT i = size - 1; i >= 0; i--)
	{
		const uint32_t id = this->maxId - this->free[i];
		if (id < (uint32_t)num)
		{
			fun(id, i);
			num--;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
inline void
IdPool::Move(uint32_t idx, uint32_t id)
{
	this->free.Append(this->maxId - id);
	this->free.EraseIndex(idx);
}

//------------------------------------------------------------------------------
/**
*/
inline const uint32_t
IdPool::GetGrow() const
{
	return this->grow;
}

} // namespace Ids