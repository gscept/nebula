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
void alloc_for_each_in_tuple(std::tuple<Ts...> & tuple, std::index_sequence<Is...>)
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
void alloc_for_each_in_tuple(std::tuple<Ts...> & tuple)
{
	alloc_for_each_in_tuple(tuple, std::make_index_sequence<sizeof...(Ts)>());
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
	/// destructor
	~IdAllocator() {};

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
	/// destructor
	~IdAllocatorSafe() {};

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

private:

	bool inBeginGet;
	Threading::CriticalSection sect;
	Ids::IdPool pool;
	uint32_t size;
	std::tuple<Util::Array<TYPES>...> objects;
};

} // namespace Ids