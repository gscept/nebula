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
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "id.h"
#include "idpool.h"
#include "util/array.h"
#include "threading/criticalsection.h"
#include <tuple>
namespace Ids
{
template <typename C>
struct get_template_type;

/// get inner type of two types
template <template <typename > class C, typename T>
struct get_template_type<C<T>>
{
	using type = T;
};

/// get inner type of a constant ref outer type
template <template <typename > class C, typename T>
struct get_template_type<const C<T>&>
{
	using type = T;
};

/// helper typedef so that the above expression can be used like decltype
template <typename C>
using get_template_type_t = typename get_template_type<C>::type;

/// unpacks allocations for each member in a tuble
template<class...Ts, std::size_t...Is>
void alloc_for_each_in_tuple(std::tuple<Ts...>& tuple, std::index_sequence<Is...>)
{
	using expander = int[];
	(void)expander
	{
		0, 
		(std::get<Is>(tuple).Append(get_template_type<Ts>::type()), 0)...
	};
}

/// entry point for above expansion function
template<class...Ts>
void alloc_for_each_in_tuple(std::tuple<Ts...>& tuple)
{
	alloc_for_each_in_tuple(tuple, std::make_index_sequence<sizeof...(Ts)>());
}

/// unpacks allocations for each member in a tuble
template<class...Ts, std::size_t...Is>
void clear_for_each_in_tuple(std::tuple<Ts...>& tuple, std::index_sequence<Is...>)
{
	using expander = int[];
	(void)expander
	{
		0,
		(std::get<Is>(tuple).Clear(), 0)...
	};
}

/// entry point for above expansion function
template<class...Ts>
void clear_for_each_in_tuple(std::tuple<Ts...>& tuple)
{
	clear_for_each_in_tuple(tuple, std::make_index_sequence<sizeof...(Ts)>());
}

/// entry point for moving an element between two indices
template <class...Ts, std::size_t...Is>
void move_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t to, uint32_t from, std::index_sequence<Is...>)
{
	using expander = int[];
	(void)expander
	{
		0,
		(std::get<Is>(tuple)[to] = std::get<Is>(tuple)[from], 0)...
	};
}

/// entry point for moving an element between two indices
template <class...Ts>
void move_for_each_in_tuple(std::tuple<Ts...>& tuple, uint32_t to, uint32_t from)
{
	move_for_each_in_tuple(tuple, to, from, std::make_index_sequence<sizeof...(Ts)>());
}

/// get type of contained element in Util::Array stored in std::tuple
template <int MEMBER, class ... TYPES>
using tuple_array_t = get_template_type_t<std::tuple_element_t<MEMBER, std::tuple<Util::Array<TYPES>...>>>;

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
		clear_for_each_in_tuple(rhs.objects);
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
		clear_for_each_in_tuple(rhs.objects);
	}

	/// allocate a new resource, and generate new entries if required
	Ids::Id32 AllocObject()
	{
		Ids::Id32 id = this->pool.Alloc();
		if (id >= this->size)
		{
			alloc_for_each_in_tuple(this->objects);
			this->size++;
		}
		return id;
	}

	/// recycle id
	void DeallocObject(const Ids::Id32 id) { this->pool.Dealloc(id); }

	/// defragment allocator to make data tightly organized
	void Defragment(Util::Array<Ids::Id32>& usedIds)
	{
		// go through free, and run callback for every free index
		this->pool.ForEachFree([&usedIds, this](uint32_t idx, uint32_t i)
		{
			Ids::Id32 elem = usedIds.Back();
			this->pool.Move(i, elem);
			move_for_each_in_tuple(this->objects, idx, elem);
			usedIds.Erase(usedIds.End()-1);
		});
	}

	/// get single item from resource, template expansion might give you cancer
	template <int MEMBER>
	inline tuple_array_t<MEMBER, TYPES...>&
	Get(const Ids::Id32 index)
	{
		return std::get<MEMBER>(this->objects)[index];
	}

	/// same as 32 bit get, but const
	template <int MEMBER>
	const inline tuple_array_t<MEMBER, TYPES...>&
	Get(const Ids::Id32 index) const
	{
		return std::get<MEMBER>(this->objects)[index];
	}

	/// get using 64 bit id
	template <int MEMBER>
	inline tuple_array_t<MEMBER, TYPES...>&
	Get(const Ids::Id64 index)
	{
		Ids::Id24 resId = Ids::Id::GetLow(Ids::Id::GetBig(index));
		return std::get<MEMBER>(this->objects)[resId];
	}

	/// same as 64 bit get, but const
	template <int MEMBER>
	const inline tuple_array_t<MEMBER, TYPES...>&
	Get(const Ids::Id64 index) const
	{
		Ids::Id24 resId = Ids::Id::GetLow(Ids::Id::GetBig(index));
		return std::get<MEMBER>(this->objects)[resId];
	}

	/// get array const reference
	template <int MEMBER>
	const inline Util::Array<tuple_array_t<MEMBER, TYPES...>>&
	GetArray() const
	{
		return std::get<MEMBER>(this->objects);
	}

	/// get array
	template <int MEMBER>
	inline Util::Array<tuple_array_t<MEMBER, TYPES...>>&
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
		clear_for_each_in_tuple(rhs.objects);
	}
	/// destructor
	~IdAllocatorSafe() {};

	/// assign operator
	void operator=(const IdAllocatorSafe<TYPES...>& rhs)
	{
		this->objects = rhs.objects;
	}
	/// move operator
	void operator=(IdAllocatorSafe<TYPES...>&& rhs)
	{
		this->objects = rhs.objects;
		clear_for_each_in_tuple(rhs.objects);
	}

	/// allocate a new resource, and generate new entries if required
	Ids::Id32 AllocObject()
	{
		this->sect.Enter();
		Ids::Id32 id = this->pool.Alloc();
		if (id >= this->size)
		{
			alloc_for_each_in_tuple(this->objects);
			this->size++;
		}
		this->sect.Leave();
		return id;
	}

	/// recycle id
	void DeallocObject(const Ids::Id32 id) 
	{ 
		this->sect.Enter();
		this->pool.Dealloc(id);
		this->sect.Leave();
	}

	/// enter thread safe get-mode
	void EnterGet()
	{
		n_assert(!this->inBeginGet);
		this->sect.Enter();
		this->inBeginGet = true;		
	}

	/// get single item from within Enter/Leave phase
	template <int MEMBER>
	inline tuple_array_t<MEMBER, TYPES...>&
	Get(const Ids::Id32 index)
	{
		n_assert(this->inBeginGet);
		return std::get<MEMBER>(this->objects)[index];
	}

	/// get const
	template <int MEMBER>
	const inline tuple_array_t<MEMBER, TYPES...>&
	Get(const Ids::Id32 index) const
	{
		n_assert(this->inBeginGet);
		return std::get<MEMBER>(this->objects)[index];
	}

	/// get single item from within Enter/Leave phase
	template <int MEMBER>
	inline tuple_array_t<MEMBER, TYPES...>&
	Get(const Ids::Id64 index)
	{
		n_assert(this->inBeginGet);
		Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(index));
		return std::get<MEMBER>(this->objects)[resId];
	}

	/// 64 bit get const
	template <int MEMBER>
	const inline tuple_array_t<MEMBER, TYPES...>&
	Get(const Ids::Id64 index) const
	{
		n_assert(this->inBeginGet);
		Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(index));
		return std::get<MEMBER>(this->objects)[resId];
	}

	/// get array const
	template <int MEMBER>
	const inline Util::Array<tuple_array_t<MEMBER, TYPES...>>&
	GetArray() const
	{
		n_assert(this->inBeginGet);
		return std::get<MEMBER>(this->objects);
	}

	/// get array
	template <int MEMBER>
	inline Util::Array<tuple_array_t<MEMBER, TYPES...>>&
	GetArray()
	{
		n_assert(this->inBeginGet);
		return std::get<MEMBER>(this->objects);
	}

	/// leave thread safe get-mode
	void LeaveGet()
	{
		n_assert(this->inBeginGet);
		this->sect.Leave();
		this->inBeginGet = false;
	}

	/// get single item safely 
	template <int MEMBER>
	inline tuple_array_t<MEMBER, TYPES...>&
	GetSafe(const Ids::Id32 index)
	{
		this->sect.Enter();
		tuple_array_t<MEMBER, TYPES...>& res = std::get<MEMBER>(this->objects)[index];
		this->sect.Leave();
		return res;
	}

	/// get single item safely 
	template <int MEMBER>
	inline tuple_array_t<MEMBER, TYPES...>&
	GetSafe(const Ids::Id64 index)
	{
		Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(index));
		this->sect.Enter();
		tuple_array_t<MEMBER, TYPES...>& res = std::get<MEMBER>(this->objects)[resId];
		this->sect.Leave();
		return res;
	}

	/// get single item unsafe (use with extreme caution)
	template <int MEMBER>
	inline tuple_array_t<MEMBER, TYPES...>&
	GetUnsafe(const Ids::Id32 index)
	{
		return std::get<MEMBER>(this->objects)[index];
	}

	/// get single item unsafe (use with extreme caution)
	template <int MEMBER>
	inline tuple_array_t<MEMBER, TYPES...>&
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