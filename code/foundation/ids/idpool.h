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
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <cstdint>
#include "core/types.h"
#include "util/array.h"
#include "id.h"
#include "math/scalar.h"
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
		uint32_t remainder = Math::n_min(this->maxId - this->free.Size(), this->grow);

		// make sure we don't allocate too many indices
		n_assert2(this->free.Size() + remainder < this->maxId, "Pool is full! Be careful with how much you allocate!\n");

		// reserve more space for new Ids
		this->free.Reserve(this->free.Size() + remainder);
		IndexT i;
		for (i = this->free.Size(); i < this->free.Capacity(); i++)
		{
			// count backwards from the max id
			this->free.Append(this->maxId - this->grow + i + 1);
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
	return this->maxId - this->free.Size();
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
inline const uint32_t
IdPool::GetGrow() const
{
	return this->grow;
}

} // namespace Ids