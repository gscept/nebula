#pragma once
//------------------------------------------------------------------------------
/**
	The ArrayAllocator provides a variadic list of types which is to be contained in the allocator
	and fetching each value by providing the index into the list of types, which means the members
	are nameless. 

	Note that this is not a container for multiple arrays, but a object allocator that uses
	multiple arrays to store their variables parallelly, and should be treated as such.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "types.h"
#include "util/array.h"
#include <tuple>

namespace Util
{
template <typename C>
struct get_template_type;

//##############################################################################
// Helper functions

//------------------------------------------------------------------------------
/**
	Get inner type of two types
*/
template <template <typename > class C, typename T>
struct get_template_type<C<T>>
{
	using type = T;
};

//------------------------------------------------------------------------------
/**
	Get inner type of a constant ref outer type
*/
template <template <typename > class C, typename T>
struct get_template_type<const C<T>&>
{
	using type = T;
};

//------------------------------------------------------------------------------
/**
	Helper typedef so that the above expression can be used like decltype
*/
template <typename C>
using get_template_type_t = typename get_template_type<C>::type;

//------------------------------------------------------------------------------
/**
	Unpacks allocations for each member in a tuble
*/
template<class...Ts, std::size_t...Is> void
alloc_for_each_in_tuple(std::tuple<Ts...>& tuple, std::integer_sequence<Is...>)
{
	using expander = int[];
	(void)expander
	{
		0,
		(std::get<Is>(tuple).Append(get_template_type<Ts>::type()), 0)...
	};
}

//------------------------------------------------------------------------------
/**
	Entry point for above expansion function
*/
template<class...Ts> void
alloc_for_each_in_tuple(std::tuple<Ts...>& tuple)
{
	alloc_for_each_in_tuple(tuple, std::make_integer_sequence<sizeof...(Ts)>());
}

//------------------------------------------------------------------------------
/**
	Unpacks allocations for each member in a tuple
*/
template<class...Ts, std::size_t...Is> void
clear_for_each_in_tuple(std::tuple<Ts...>& tuple, std::integer_sequence<Is...>)
{
	using expander = int[];
	(void)expander
	{
		0,
		(std::get<Is>(tuple).Clear(), 0)...
	};
}

//------------------------------------------------------------------------------
/**
	Entry point for above expansion function
*/
template<class...Ts> void
clear_for_each_in_tuple(std::tuple<Ts...>& tuple)
{
	clear_for_each_in_tuple(tuple, std::make_integer_sequence<sizeof...(Ts)>());
}

//------------------------------------------------------------------------------
/**
	Entry point for moving an element between two indices
*/
template <class...Ts, std::size_t...Is> void
move_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t to, uint32_t from, std::integer_sequence<Is...>)
{
	using expander = int[];
	(void)expander
	{
		0,
		(std::get<Is>(tuple)[to] = std::get<Is>(tuple)[from], 0)...
	};
}

//------------------------------------------------------------------------------
/**
	Entry point for moving an element between two indices
*/
template <class...Ts> void
move_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t to, uint32_t from)
{
	move_for_each_in_tuple(tuple, to, from, std::make_integer_sequence<sizeof...(Ts)>());
}

//------------------------------------------------------------------------------
/**
	Entry point for erasing an element. Keeps sorting but is generally slow
	due to shifting all element at i + 1 one step left.
*/
template <class...Ts, std::size_t...Is> void
erase_index_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t i, std::integer_sequence<Is...>)
{
	using expander = int[];
	(void)expander
	{
		0,
		(std::get<Is>(tuple).EraseIndex(i), 0)...
	};
}

//------------------------------------------------------------------------------
/**
	Entry point for erasing an element. Keeps sorting but is generally slow
	due to shifting all element at i + 1 one step left.
*/
template <class...Ts> void
erase_index_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t i)
{
	erase_index_for_each_in_tuple(tuple, i, std::make_integer_sequence<sizeof...(Ts)>());
}

//------------------------------------------------------------------------------
/**
	Entry point for erasing an element by swapping with
	the last and reducing size.

	@note	Destroys sorting!
*/
template <class...Ts, std::size_t...Is> void
erase_index_swap_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t i, std::integer_sequence<Is...>)
{
	using expander = int[];
	(void)expander
	{
		0,
		(std::get<Is>(tuple).EraseIndexSwap(i), 0)...
	};
}

//------------------------------------------------------------------------------
/**
	Entry point for erasing an element by swapping with
	the last and reducing size.

	@note	Destroys sorting!
*/
template <class...Ts> void
erase_index_swap_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t i)
{
	erase_index_swap_for_each_in_tuple(tuple, i, std::make_integer_sequence<sizeof...(Ts)>());
}

//------------------------------------------------------------------------------
/**
	Entry point for setting values in each array at an index
*/
template <class...Ts, std::size_t...Is, class...TYPES> void
set_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t i, std::integer_sequence<Is...>, TYPES const& ... values)
{

	using expander = int[];
	(void)expander
	{
		0,
		(std::get<Is>(tuple)[i] = values, 0)...
	};
}

//------------------------------------------------------------------------------
/**
	Entry point for setting values in each array at an index
*/
template <class...Ts, class...TYPES> void
set_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t i, TYPES const& ... values)
{
	set_for_each_in_tuple(tuple, i, std::make_integer_sequence<sizeof...(Ts)>(), values...);
}

//------------------------------------------------------------------------------
/**
	Get type of contained element in Util::Array stored in std::tuple
*/
template <int MEMBER, class ... TYPES>
using tuple_array_t = get_template_type_t<std::tuple_element_t<MEMBER, std::tuple<Util::Array<TYPES>...>>>;
//------------------------------------------------------------------------------
//##############################################################################

//------------------------------------------------------------------------------
/**
	@class	ArrayAllocator
*/
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

	/// allocate a new resource, and generate new entries if required
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

	/// clear entire allocator and start from scratch.
	void Clear();

private:
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
template<class ...TYPES>
inline void ArrayAllocator<TYPES...>::EraseIndex(const uint32_t id)
{
	erase_index_for_each_in_tuple(this->objects, id);
	this->size--;
}

//------------------------------------------------------------------------------
/**
*/
template<class ...TYPES>
inline void ArrayAllocator<TYPES...>::EraseIndexSwap(const uint32_t id)
{
	erase_index_swap_for_each_in_tuple(this->objects, id);
	this->size--;
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

} // namespace Util
