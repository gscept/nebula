#pragma once
//------------------------------------------------------------------------------
/**
	The Id pool implements a set of free and used consecutive integers
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <cstdint>
#include "types.h"
#include "util/array.h"
namespace Core
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

}

//------------------------------------------------------------------------------
/**
*/
inline uint32_t
IdPool::Alloc()
{
	if (this->free.Size() == this->free.Capacity())
	{
		// make sure we don't allocate too many indices
		n_assert(this->free.Size() + this->grow < this->maxId);

		// this should trigger a growth, add 
		this->free.Reserve(this->grow);
		IndexT i;
		for (i = this->free.Size(); i < this->free.Capacity(); i++)
		{
			this->free.Append(this->maxId - this->grow + i + 1);
		}
	}
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

} // namespace Core