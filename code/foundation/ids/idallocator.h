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
	
	(C) 2017-2018 Individual contributors, see AUTHORS file
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
		this->freeIds.InsertSorted(index);
	}

	/// Returns the list of free ids.
	Util::Array<uint32_t>& FreeIds()
	{
		return this->freeIds;
	}

private:
	uint32_t maxId = 0xFFFFFFFF;
	Util::Array<uint32_t> freeIds;
};


template <class ... TYPES>
class IdAllocatorSafe
{
public:
	/// constructor
	IdAllocatorSafe(uint32_t maxid = 0xFFFFFFFF, uint32_t grow = 512) : pool(maxid, grow), size(0), inBeginGet(false) {};
	/// copy constructor
	IdAllocatorSafe(const IdAllocatorSafe<TYPES...>& rhs)
	{
		this->objects = rhs.objects;
	}
	/// move operator
	IdAllocatorSafe(IdAllocatorSafe<TYPES...>&& rhs)
	{
		this->objects = rhs.objects;
		Util::clear_for_each_in_tuple(rhs.objects);
	}
	/// destructor
	~IdAllocatorSafe() {};

	/// assign operator
	void
	operator=(const IdAllocatorSafe<TYPES...>& rhs)
	{
		this->objects = rhs.objects;
	}
	/// move operator
	void
	operator=(IdAllocatorSafe<TYPES...>&& rhs)
	{
		this->objects = rhs.objects;
		Util::clear_for_each_in_tuple(rhs.objects);
	}

	/// allocate a new resource, and generate new entries if required
	Ids::Id32
	AllocObject()
	{
		this->sect.Enter();
		Ids::Id32 id = this->pool.Alloc();
		if (id >= this->size)
		{
			Util::alloc_for_each_in_tuple(this->objects);
			this->size++;
		}
		this->sect.Leave();
		return id;
	}

	/// recycle id
	void
	DeallocObject(const Ids::Id32 id) 
	{ 
		this->sect.Enter();
		this->pool.Dealloc(id);
		this->sect.Leave();
	}

	/// enter thread safe get-mode
	void
	EnterGet()
	{
		n_assert(!this->inBeginGet);
		this->sect.Enter();
		this->inBeginGet = true;		
	}

	/// get single item from within Enter/Leave phase
	template <int MEMBER>
	inline Util::tuple_array_t<MEMBER, TYPES...>&
	Get(const Ids::Id32 index)
	{
		n_assert(this->inBeginGet);
		return std::get<MEMBER>(this->objects)[index];
	}

	/// get const
	template <int MEMBER>
	const inline Util::tuple_array_t<MEMBER, TYPES...>&
	Get(const Ids::Id32 index) const
	{
		n_assert(this->inBeginGet);
		return std::get<MEMBER>(this->objects)[index];
	}

	/// get single item from within Enter/Leave phase
	template <int MEMBER>
	inline Util::tuple_array_t<MEMBER, TYPES...>&
	Get(const Ids::Id64 index)
	{
		n_assert(this->inBeginGet);
		Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(index));
		return std::get<MEMBER>(this->objects)[resId];
	}

	/// 64 bit get const
	template <int MEMBER>
	const inline Util::tuple_array_t<MEMBER, TYPES...>&
	Get(const Ids::Id64 index) const
	{
		n_assert(this->inBeginGet);
		Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(index));
		return std::get<MEMBER>(this->objects)[resId];
	}

	/// get array const
	template <int MEMBER>
	const inline Util::Array<Util::tuple_array_t<MEMBER, TYPES...>>&
	GetArray() const
	{
		n_assert(this->inBeginGet);
		return std::get<MEMBER>(this->objects);
	}

	/// get array
	template <int MEMBER>
	inline Util::Array<Util::tuple_array_t<MEMBER, TYPES...>>&
	GetArray()
	{
		n_assert(this->inBeginGet);
		return std::get<MEMBER>(this->objects);
	}

	/// leave thread safe get-mode
	void
	LeaveGet()
	{
		n_assert(this->inBeginGet);
		this->sect.Leave();
		this->inBeginGet = false;
	}

	/// get single item safely 
	template <int MEMBER>
	inline Util::tuple_array_t<MEMBER, TYPES...>&
	GetSafe(const Ids::Id32 index)
	{
		this->sect.Enter();
		Util::tuple_array_t<MEMBER, TYPES...>& res = std::get<MEMBER>(this->objects)[index];
		this->sect.Leave();
		return res;
	}

	/// get single item safely 
	template <int MEMBER>
	inline Util::tuple_array_t<MEMBER, TYPES...>&
	GetSafe(const Ids::Id64 index)
	{
		Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(index));
		this->sect.Enter();
		Util::tuple_array_t<MEMBER, TYPES...>& res = std::get<MEMBER>(this->objects)[resId];
		this->sect.Leave();
		return res;
	}

	/// get single item unsafe (use with extreme caution)
	template <int MEMBER>
	inline Util::tuple_array_t<MEMBER, TYPES...>&
	GetUnsafe(const Ids::Id32 index)
	{
		return std::get<MEMBER>(this->objects)[index];
	}

	/// get single item unsafe (use with extreme caution)
	template <int MEMBER>
	inline Util::tuple_array_t<MEMBER, TYPES...>&
	GetUnsafe(const Ids::Id64 index)
	{
		Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(index));
		return std::get<MEMBER>(this->objects)[resId];
	}

	/// get number of used indices
	const inline uint32_t
	GetNumUsed() const
	{
		return pool.GetNumUsed();
	}

private:

	bool inBeginGet;
	Threading::CriticalSection sect;
	Ids::IdPool pool;
	uint32_t size;
	std::tuple<Util::Array<TYPES>...> objects;
};

} // namespace Ids
