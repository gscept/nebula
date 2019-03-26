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

namespace Ids
{

template <class ... TYPES>
class IdAllocator
{
public:
	/// constructor
	IdAllocator(uint32_t maxid = 0xFFFFFFFF, uint32_t grow = 512) : pool(maxid, grow), size(0) {};
	/// move constructor
	IdAllocator(IdAllocator<TYPES...>&& rhs)
	{
		this->objects = rhs.objects;
		Util::clear_for_each_in_tuple(rhs.objects);
	}
	/// copy constructor
	IdAllocator(const IdAllocator<TYPES...>& rhs)
	{
		this->objects = rhs.objects;
	}
	/// destructor
	~IdAllocator() {};

	/// assign operator
	void operator=(const IdAllocator<TYPES...>& rhs)
	{
		this->objects = rhs.objects;
	}
	/// move operator
	void operator=(IdAllocator<TYPES...>&& rhs)
	{
		this->objects = rhs.objects;
		Util::clear_for_each_in_tuple(rhs.objects);
	}

	/// allocate a new resource, and generate new entries if required
	Ids::Id32 AllocObject()
	{
		Ids::Id32 id = this->pool.Alloc();
		if (id >= this->size)
		{
			Util::alloc_for_each_in_tuple(this->objects);
			this->size++;
		}
		return id;
	}

	/// recycle id
	void DeallocObject(const Ids::Id32 id)
	{ 
		this->pool.Dealloc(id);
		this->size--;
	}

	/// defragment allocator to make data tightly organized
	void Defragment(Util::Array<Ids::Id32>& usedIds)
	{
		// go through free, and run callback for every free index
		this->pool.ForEachFree([&usedIds, this](uint32_t idx, uint32_t i)
		{
			Ids::Id32 elem = usedIds.Back();
			this->pool.Move(i, elem);
			Util::move_for_each_in_tuple(this->objects, idx, elem);
			usedIds.Erase(usedIds.End()-1);
		}, usedIds.Size());
	}

	/// get single item from resource, template expansion might give you cancer
	template <int MEMBER>
	inline Util::tuple_array_t<MEMBER, TYPES...>&
	Get(const Ids::Id32 index)
	{
		return std::get<MEMBER>(this->objects)[index];
	}

	/// same as 32 bit get, but const
	template <int MEMBER>
	const inline Util::tuple_array_t<MEMBER, TYPES...>&
	Get(const Ids::Id32 index) const
	{
		return std::get<MEMBER>(this->objects)[index];
	}

	/// get using 64 bit id
	template <int MEMBER>
	inline Util::tuple_array_t<MEMBER, TYPES...>&
	Get(const Ids::Id64 index)
	{
		Ids::Id24 resId = Ids::Id::GetLow(Ids::Id::GetBig(index));
		return std::get<MEMBER>(this->objects)[resId];
	}

	/// same as 64 bit get, but const
	template <int MEMBER>
	const inline Util::tuple_array_t<MEMBER, TYPES...>&
	Get(const Ids::Id64 index) const
	{
		Ids::Id24 resId = Ids::Id::GetLow(Ids::Id::GetBig(index));
		return std::get<MEMBER>(this->objects)[resId];
	}

	/// get array const reference
	template <int MEMBER>
	const inline Util::Array<Util::tuple_array_t<MEMBER, TYPES...>>&
	GetArray() const
	{
		return std::get<MEMBER>(this->objects);
	}

	/// get array
	template <int MEMBER>
	inline Util::Array<Util::tuple_array_t<MEMBER, TYPES...>>&
	GetArray()
	{
		return std::get<MEMBER>(this->objects);
	}

	/// get number of used indices
	const inline uint32_t
	GetNumUsed() const
	{
		return pool.GetNumUsed();
	}

private:

	Ids::IdPool pool;
	uint32_t size;
	std::tuple<Util::Array<TYPES>...> objects;
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
