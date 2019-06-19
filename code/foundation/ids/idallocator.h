#pragma once
//------------------------------------------------------------------------------
/**
	An ID allocator associates an id with a slice in an N number of arrays.

	There are two versions of this type, an unsafe and a safe one. Both are implemented
	in the same way, providing a variadic list of types which is to be contained in the allocator
	and fetching each value by providing the index into the list of types, which means the members
	are nameless. 

	The thread safe allocator requires the Get-methods to be within an Enter/Leave
	lockstep phase. 
	
	(C) 2017-2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "id.h"
#include "idpool.h"
#include "util/array.h"
#include "threading/criticalsection.h"
#include <tuple>
#include <utility>
#include "util/tupleutility.h"
#include "util/arrayallocator.h"
#include "util/arrayallocatorsafe.h"

namespace Ids
{

template<class ... TYPES>
class IdAllocator : public Util::ArrayAllocator<TYPES...>
{
public:
	/// constructor
	IdAllocator(uint32_t maxid = 0xFFFFFFFF) : maxId(maxid) {};
	
	/// Allocate an object. 
	uint32_t Alloc()
	{
		/// @note	This purposefully hides the default allocation method and should definetly not be virtual!
		
		uint32_t index;
		if (this->freeIds.Size() > 0)
		{
			index = this->freeIds.Back();
			this->freeIds.EraseBack();
		}
		else
		{
			index = Util::ArrayAllocator<TYPES...>::Alloc();
			n_assert2(this->maxId > index, "max amount of allocations exceeded!\n");
		}

		return index;
	}

	/// Deallocate an object. Just places it in freeids array for recycling
	void Dealloc(uint32_t index)
	{
		// TODO: We could possibly get better performance when defragging if we insert it in reverse order (high to low)
		this->freeIds.Append(index);
	}

	/// Returns the list of free ids.
	Util::Array<uint32_t>& FreeIds()
	{
		return this->freeIds;
	}

    /// return number of allocated ids
    const uint32_t Size() const
    {
        return this->size - freeIds.Size();
    }

private:
	uint32_t maxId = 0xFFFFFFFF;
	Util::Array<uint32_t> freeIds;
};

template<class ... TYPES>
class IdAllocatorSafe : public Util::ArrayAllocatorSafe<TYPES...>
{
public:
	/// constructor
	IdAllocatorSafe(uint32_t maxid = 0xFFFFFFFF) : maxId(maxid)
	{
	};

	/// Allocate an object. 
	uint32_t Alloc()
	{
		/// @note	This purposefully hides the default allocation method and should definetly not be virtual!
		
		this->sect.Enter();
		uint32_t index;
		if (this->freeIds.Size() > 0)
		{
			index = this->freeIds.Back();
			this->freeIds.EraseBack();
		}
		else
		{
			alloc_for_each_in_tuple(this->objects);
			index = this->size++;
			n_assert2(this->maxId > index, "max amount of allocations exceeded!\n");
		}
		this->sect.Leave();

		return index;
	}

	/// Deallocate an object. Just places it in freeids array for recycling
	void Dealloc(uint32_t index)
	{
		// TODO: We could possibly get better performance when defragging if we insert it in reverse order (high to low)
		this->sect.Enter();
		this->freeIds.Append(index);
		this->sect.Leave();
	}

	/// Returns the list of free ids.
	Util::Array<uint32_t>& FreeIds()
	{
		n_assert(this->inBeginGet);
		return this->freeIds;
	}

private:
	uint32_t maxId = 0xFFFFFFFF;
	Util::Array<uint32_t> freeIds;
};

} // namespace Ids
