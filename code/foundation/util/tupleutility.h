#pragma once
//------------------------------------------------------------------------------
/**
	Tuple helper functions and typedefs

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

namespace Util
{
template <typename C>
struct get_template_type;

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
alloc_for_each_in_tuple(std::tuple<Ts...>& tuple, std::index_sequence<Is...>)
{
	(std::get<Is>(tuple).Append(typename get_template_type<Ts>::type()), ...);
}

//------------------------------------------------------------------------------
/**
	Entry point for above expansion function
*/
template<class...Ts> void
alloc_for_each_in_tuple(std::tuple<Ts...>& tuple)
{
	alloc_for_each_in_tuple(tuple, std::make_index_sequence<sizeof...(Ts)>());
}

//------------------------------------------------------------------------------
/**
	Unpacks allocations for each member in a tuple
*/
template<class...Ts, std::size_t...Is> void
clear_for_each_in_tuple(std::tuple<Ts...>& tuple, std::index_sequence<Is...>)
{
	(std::get<Is>(tuple).Clear(), ...);
}

//------------------------------------------------------------------------------
/**
	Entry point for above expansion function
*/
template<class...Ts> void
clear_for_each_in_tuple(std::tuple<Ts...>& tuple)
{
	clear_for_each_in_tuple(tuple, std::make_index_sequence<sizeof...(Ts)>());
}

//------------------------------------------------------------------------------
/**
	Entry point for moving an element between two indices
*/
template <class...Ts, std::size_t...Is> void
move_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t to, uint32_t from, std::index_sequence<Is...>)
{
	(std::get<Is>(tuple)[to] = std::get<Is>(tuple)[from], ...);
}

//------------------------------------------------------------------------------
/**
	Entry point for moving an element between two indices
*/
template <class...Ts> void
move_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t to, uint32_t from)
{
	move_for_each_in_tuple(tuple, to, from, std::make_index_sequence<sizeof...(Ts)>());
}

//------------------------------------------------------------------------------
/**
	Entry point for erasing an element. Keeps sorting but is generally slow
	due to shifting all element at i + 1 one step left.
*/
template <class...Ts, std::size_t...Is> void
erase_index_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t i, std::index_sequence<Is...>)
{
	(std::get<Is>(tuple).EraseIndex(i), ...);
}

//------------------------------------------------------------------------------
/**
	Entry point for erasing an element. Keeps sorting but is generally slow
	due to shifting all element at i + 1 one step left.
*/
template <class...Ts> void
erase_index_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t i)
{
	erase_index_for_each_in_tuple(tuple, i, std::make_index_sequence<sizeof...(Ts)>());
}

//------------------------------------------------------------------------------
/**
	Entry point for erasing an element by swapping with
	the last and reducing size.

	@note	Destroys sorting!
*/
template <class...Ts, std::size_t...Is> void
erase_index_swap_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t i, std::index_sequence<Is...>)
{
	(std::get<Is>(tuple).EraseIndexSwap(i), ...);
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
	erase_index_swap_for_each_in_tuple(tuple, i, std::make_index_sequence<sizeof...(Ts)>());
}

//------------------------------------------------------------------------------
/**
	Entry point for setting values in each array at an index
*/
template <class...Ts, std::size_t...Is, class...TYPES> void
set_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t i, std::index_sequence<Is...>, TYPES const& ... values)
{
	((std::get<Is>(tuple)[i] = values), ...);
}

//------------------------------------------------------------------------------
/**
	Entry point for setting values in each array at an index
*/
template <class...Ts, class...TYPES> void
set_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t i, TYPES const& ... values)
{
	set_for_each_in_tuple(tuple, i, std::make_index_sequence<sizeof...(Ts)>(), values...);
}

//------------------------------------------------------------------------------
/**
	Entry point for reserving in each array
*/
template <class...Ts, std::size_t...Is> void
reserve_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t size, std::index_sequence<Is...>)
{
	(std::get<Is>(tuple).Reserve(size), ...);
}

//------------------------------------------------------------------------------
/**
	Entry point for reserving in each array
*/
template <class...Ts> void
reserve_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t size)
{
	reserve_for_each_in_tuple(tuple, size, std::make_index_sequence<sizeof...(Ts)>());
}

//------------------------------------------------------------------------------
/**
    Entry point for reserving in each array
*/
template <class...Ts, std::size_t...Is> void
set_size_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t size, std::index_sequence<Is...>)
{
	(std::get<Is>(tuple).SetSize(size), ...);
}

//------------------------------------------------------------------------------
/**
    Entry point for reserving in each array
*/
template <class...Ts> void
set_size_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t size)
{
    set_size_for_each_in_tuple(tuple, size, std::make_index_sequence<sizeof...(Ts)>());
}

//------------------------------------------------------------------------------
/**
	Get type of contained element in Util::Array stored in std::tuple
*/
template <int MEMBER, class ... TYPES>
using tuple_array_t = get_template_type_t<std::tuple_element_t<MEMBER, std::tuple<Util::Array<TYPES>...>>>;
}
