#pragma once
//------------------------------------------------------------------------------
/**
	The ArrayAllocatorSafe provides a thread safe variadic list of types which is to be contained in the allocator
	and fetching each value by providing the index into the list of types, which means the members
	are nameless.

	Note that this is not a container for multiple arrays, but a object allocator that uses
	multiple arrays to store their variables parallelly, and should be treated as such.

	There are two versions of this type, an unsafe and a safe one. Both are implemented
	in the same way.

	The thread safe allocator requires the Get-methods to be within an Enter/Leave
	lockstep phase. 

	@see	arrayallocator.h

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "types.h"
#include "util/array.h"
#include <tuple>
#include "tupleutility.h"
#include "ids/id.h"

namespace Util
{

template <class ... TYPES>
class ArrayAllocatorSafe
{
public:
	/// constructor
	ArrayAllocatorSafe();

	/// move constructor
	ArrayAllocatorSafe(ArrayAllocatorSafe<TYPES...>&& rhs);

	/// copy constructor
	ArrayAllocatorSafe(const ArrayAllocatorSafe<TYPES...>& rhs);

	/// destructor
	~ArrayAllocatorSafe();

	/// assign operator
	void operator=(const ArrayAllocatorSafe<TYPES...>& rhs);

	/// move operator
	void operator=(ArrayAllocatorSafe<TYPES...>&& rhs);

	/// allocate a new resource
	uint32_t Alloc();

	/// Erase element for each
	void EraseIndex(const uint32_t id);

	/// Erase element for each
	void EraseIndexSwap(const uint32_t id);

	/// get single item from resource, template expansion might give you cancer
	template <int MEMBER>
	tuple_array_t<MEMBER, TYPES...>& Get(const uint32_t index);

	/// same as 32 bit get, but const
	template <int MEMBER>
	const tuple_array_t<MEMBER, TYPES...>& Get(const uint32_t index) const;

	/// get array const reference
	template <int MEMBER>
	const Util::Array<tuple_array_t<MEMBER, TYPES...>>& GetArray() const;

	/// get array
	template <int MEMBER>
	Util::Array<tuple_array_t<MEMBER, TYPES...>>& GetArray();

	/// set for each in tuple
	void Set(const uint32_t index, TYPES...);

	/// get number of used indices
	const uint32_t Size() const;

	/// grow capacity of arrays to size
	void Reserve(uint32_t size);

	/// clear entire allocator and start from scratch.
	void Clear();

	/// Any reserve and direct array access might mess with the size.
	/// This will update the size to reflect the first member array size in objects.
	void UpdateSize();

	/// enter thread safe get-mode
	void EnterGet();
	/// leave thread safe get-mode
	void LeaveGet();

	/// get single item safely 
	template<int MEMBER>
	Util::tuple_array_t<MEMBER, TYPES...>& GetSafe(const Ids::Id32 index);
	/// get single item safely 
	template<int MEMBER>
	Util::tuple_array_t<MEMBER, TYPES...>& GetSafe(const Ids::Id64 index);
	/// get single item unsafe (use with extreme caution)
	template<int MEMBER>
	Util::tuple_array_t<MEMBER, TYPES...>& GetUnsafe(const Ids::Id32 index);
	/// get single item unsafe (use with extreme caution)
	template<int MEMBER>
	Util::tuple_array_t<MEMBER, TYPES...>& GetUnsafe(const Ids::Id64 index);

protected:
	bool inBeginGet;
	Threading::CriticalSection sect;
	uint32_t size;
	std::tuple<Util::Array<TYPES>...> objects;
};

//------------------------------------------------------------------------------
/**
*/
template<class ... TYPES>
inline ArrayAllocatorSafe<TYPES...>::ArrayAllocatorSafe() :
	size(0),
	inBeginGet(false)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline ArrayAllocatorSafe<TYPES...>::ArrayAllocatorSafe(ArrayAllocatorSafe<TYPES...>&& rhs)
{
	this->objects = rhs.objects;
	this->size = rhs.size;
	rhs.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline ArrayAllocatorSafe<TYPES...>::ArrayAllocatorSafe(const ArrayAllocatorSafe<TYPES...>& rhs)
{
	this->objects = rhs.objects;
	this->size = rhs.size;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline ArrayAllocatorSafe<TYPES...>::~ArrayAllocatorSafe()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void
ArrayAllocatorSafe<TYPES...>::operator=(const ArrayAllocatorSafe<TYPES...>& rhs)
{
	this->objects = rhs.objects;
	this->size = rhs.size;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void
ArrayAllocatorSafe<TYPES...>::operator=(ArrayAllocatorSafe<TYPES...>&& rhs)
{
	this->objects = rhs.objects;
	this->size = rhs.size;
	rhs.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline uint32_t ArrayAllocatorSafe<TYPES...>::Alloc()
{
	this->sect.Enter();
	alloc_for_each_in_tuple(this->objects);
	auto i = this->size++;
	this->sect.Leave();
	return i;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void ArrayAllocatorSafe<TYPES...>::EraseIndex(const uint32_t id)
{
	this->sect.Enter();
	erase_index_for_each_in_tuple(this->objects, id);
	this->size--;
	this->sect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void ArrayAllocatorSafe<TYPES...>::EraseIndexSwap(const uint32_t id)
{
	this->sect.Enter();
	erase_index_swap_for_each_in_tuple(this->objects, id);
	this->size--;
	this->sect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline const uint32_t
ArrayAllocatorSafe<TYPES...>::Size() const
{
	this->sect.Enter();
	return this->size;
	this->sect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void
ArrayAllocatorSafe<TYPES...>::Reserve(uint32_t num)
{
	this->sect.Enter();
	reserve_for_each_in_tuple(this->objects, num);
	this->sect.Leave();
	// Size is still the same.
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void
ArrayAllocatorSafe<TYPES...>::Clear()
{
	this->sect.Enter();
	clear_for_each_in_tuple(this->objects);
	this->sect.Leave();
	this->size = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
template<int MEMBER>
inline tuple_array_t<MEMBER, TYPES...>&
ArrayAllocatorSafe<TYPES...>::Get(const uint32_t index)
{
	n_assert(this->inBeginGet);
	return std::get<MEMBER>(this->objects)[index];
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
template<int MEMBER>
inline const tuple_array_t<MEMBER, TYPES...>&
ArrayAllocatorSafe<TYPES...>::Get(const uint32_t index) const
{
	n_assert(this->inBeginGet);
	return std::get<MEMBER>(this->objects)[index];
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
template<int MEMBER>
inline const Util::Array<tuple_array_t<MEMBER, TYPES...>>&
ArrayAllocatorSafe<TYPES...>::GetArray() const
{
	n_assert(this->inBeginGet);
	return std::get<MEMBER>(this->objects);
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
template<int MEMBER>
inline Util::Array<tuple_array_t<MEMBER, TYPES...>>&
ArrayAllocatorSafe<TYPES...>::GetArray()
{
	n_assert(this->inBeginGet);
	return std::get<MEMBER>(this->objects);
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES> void
ArrayAllocatorSafe<TYPES...>::Set(const uint32_t index, TYPES... values)
{
	n_assert(this->inBeginGet);
	set_for_each_in_tuple(this->objects, index, values...);
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES> void
ArrayAllocatorSafe<TYPES...>::UpdateSize()
{
	this->size = this->GetArray<0>().Size();
}

template<class ... TYPES> void
ArrayAllocatorSafe<TYPES...>::EnterGet()
{
	n_assert(!this->inBeginGet);
	this->sect.Enter();
	this->inBeginGet = true;
}

template<class ... TYPES> void
ArrayAllocatorSafe<TYPES...>::LeaveGet()
{
	n_assert(this->inBeginGet);
	this->sect.Leave();
	this->inBeginGet = false;
}

/// get single item safely 
template<class ... TYPES>
template<int MEMBER>
Util::tuple_array_t<MEMBER, TYPES...>&
ArrayAllocatorSafe<TYPES...>::GetSafe(const Ids::Id32 index)
{
	this->sect.Enter();
	Util::tuple_array_t<MEMBER, TYPES...>& res = std::get<MEMBER>(this->objects)[index];
	this->sect.Leave();
	return res;
}

/// get single item safely 
template<class ... TYPES>
template<int MEMBER>
Util::tuple_array_t<MEMBER, TYPES...>&
ArrayAllocatorSafe<TYPES...>::GetSafe(const Ids::Id64 index)
{
	Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(index));
	this->sect.Enter();
	Util::tuple_array_t<MEMBER, TYPES...>& res = std::get<MEMBER>(this->objects)[resId];
	this->sect.Leave();
	return res;
}

/// get single item unsafe (use with extreme caution)
template<class ... TYPES>
template<int MEMBER>
Util::tuple_array_t<MEMBER, TYPES...>&
ArrayAllocatorSafe<TYPES...>::GetUnsafe(const Ids::Id32 index)
{
	return std::get<MEMBER>(this->objects)[index];
}

/// get single item unsafe (use with extreme caution)
template<class ... TYPES>
template<int MEMBER>
Util::tuple_array_t<MEMBER, TYPES...>&
ArrayAllocatorSafe<TYPES...>::GetUnsafe(const Ids::Id64 index)
{
	Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(index));
	return std::get<MEMBER>(this->objects)[resId];
}

} // namespace Util
