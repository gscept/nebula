#pragma once
//------------------------------------------------------------------------------
/**
	The ArrayAllocator provides a variadic list of types which is to be contained in the allocator
	and fetching each value by providing the index into the list of types, which means the members
	are nameless.

	Note that this is not a container for multiple arrays, but a object allocator that uses
	multiple arrays to store their variables parallelly, and should be treated as such.

	There are two versions of this type, an unsafe and a safe one. Both are implemented
	in the same way.

	The thread safe allocator requires the Get-methods to be within an Enter/Leave
	lockstep phase.

	@see	arrayallocatorsafe.h

	(C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "types.h"
#include "util/array.h"
#include <tuple>
#include "tupleutility.h"
namespace Util
{

template <class ... TYPES>
class ArrayAllocator
{
public:
	/// constructor
	ArrayAllocator();

	/// move constructor
	ArrayAllocator(ArrayAllocator<TYPES...>&& rhs);

	/// copy constructor
	ArrayAllocator(const ArrayAllocator<TYPES...>& rhs);

	/// destructor
	~ArrayAllocator();

	/// assign operator
	void operator=(const ArrayAllocator<TYPES...>& rhs);

	/// move operator
	void operator=(ArrayAllocator<TYPES...>&& rhs);

	/// allocate a new resource
	uint32_t Alloc();

	/// Erase element for each
	void EraseIndex(const uint32_t id);
	/// Erase element for each
	void EraseIndexSwap(const uint32_t id);
	/// erase range
	void EraseRange(const uint32_t start, const uint32_t end);

	/// get single item from resource, template expansion might give you cancer
	template <int MEMBER>
	tuple_array_t<MEMBER, TYPES...>& Get(const uint32_t index);

	/// same as 32 bit get, but const
	template <int MEMBER>
	const tuple_array_t<MEMBER, TYPES...>& Get(const uint32_t index) const;

	/// set single item
	template <int MEMBER>
	void Set(const uint32_t index, const tuple_array_t<MEMBER, TYPES...>& type);

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

    /// set size of arrays to param size
    void SetSize(uint32_t size);

	/// clear entire allocator and start from scratch.
	void Clear();

	/// Any reserve and direct array access might mess with the size.
	/// This will update the size to reflect the first member array size in objects.
	void UpdateSize();

protected:
	uint32_t size;
	std::tuple<Util::Array<TYPES>...> objects;
};

//------------------------------------------------------------------------------
/**
*/
template<class ... TYPES>
inline ArrayAllocator<TYPES...>::ArrayAllocator() :
	size(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline ArrayAllocator<TYPES...>::ArrayAllocator(ArrayAllocator<TYPES...>&& rhs)
{
	this->objects = rhs.objects;
	this->size = rhs.size;
	rhs.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline ArrayAllocator<TYPES...>::ArrayAllocator(const ArrayAllocator<TYPES...>& rhs)
{
	this->objects = rhs.objects;
	this->size = rhs.size;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline ArrayAllocator<TYPES...>::~ArrayAllocator()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void
ArrayAllocator<TYPES...>::operator=(const ArrayAllocator<TYPES...>& rhs)
{
	this->objects = rhs.objects;
	this->size = rhs.size;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void
ArrayAllocator<TYPES...>::operator=(ArrayAllocator<TYPES...>&& rhs)
{
	this->objects = rhs.objects;
	this->size = rhs.size;
	rhs.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline uint32_t ArrayAllocator<TYPES...>::Alloc()
{
	alloc_for_each_in_tuple(this->objects);
	return this->size++;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES> inline void 
ArrayAllocator<TYPES...>::EraseIndex(const uint32_t id)
{
	erase_index_for_each_in_tuple(this->objects, id);
	this->size--;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES> inline void 
ArrayAllocator<TYPES...>::EraseIndexSwap(const uint32_t id)
{
	erase_index_swap_for_each_in_tuple(this->objects, id);
	this->size--;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES> inline void 
ArrayAllocator<TYPES...>::EraseRange(const uint32_t start, const uint32_t end)
{
	erase_range_for_each_in_tuple(this->objects, start, end);
	this->size -= (end + 1) - start;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline const uint32_t
ArrayAllocator<TYPES...>::Size() const
{
	return this->size;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void
ArrayAllocator<TYPES...>::Reserve(uint32_t num)
{
	reserve_for_each_in_tuple(this->objects, num);
	// Size is still the same.
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void
ArrayAllocator<TYPES...>::SetSize(uint32_t size)
{
    set_size_for_each_in_tuple(this->objects, size);
    this->size = size;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void
ArrayAllocator<TYPES...>::Clear()
{
	clear_for_each_in_tuple(this->objects);
	this->size = 0;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
template<int MEMBER>
inline tuple_array_t<MEMBER, TYPES...>&
ArrayAllocator<TYPES...>::Get(const uint32_t index)
{
	return std::get<MEMBER>(this->objects)[index];
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
template<int MEMBER>
inline const tuple_array_t<MEMBER, TYPES...>&
ArrayAllocator<TYPES...>::Get(const uint32_t index) const
{
	return std::get<MEMBER>(this->objects)[index];
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
template<int MEMBER>
inline void 
ArrayAllocator<TYPES...>::Set(const uint32_t index, const tuple_array_t<MEMBER, TYPES...>& type)
{
	std::get<MEMBER>(this->objects)[index] = type;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
template<int MEMBER>
inline const Util::Array<tuple_array_t<MEMBER, TYPES...>>&
ArrayAllocator<TYPES...>::GetArray() const
{
	return std::get<MEMBER>(this->objects);
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
template<int MEMBER>
inline Util::Array<tuple_array_t<MEMBER, TYPES...>>&
ArrayAllocator<TYPES...>::GetArray()
{
	return std::get<MEMBER>(this->objects);
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES> void
ArrayAllocator<TYPES...>::Set(const uint32_t index, TYPES... values)
{
	set_for_each_in_tuple(this->objects, index, values...);
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES> void
ArrayAllocator<TYPES...>::UpdateSize()
{
	this->size = this->GetArray<0>().Size();
}

} // namespace Util
