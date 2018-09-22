#pragma once
//------------------------------------------------------------------------------
/**
	@class Game::ComponentData

	ComponentData as a struct of arrays.
	Can perform garbage collection that rearranges arrays entries to avoid gaps

	(C) 2018 Individual contributors, see AUTHORS file
*/

#include "util/dictionary.h"
#include "util/stack.h"
#include "ids/id.h"
#include "util/random.h"
#include "basegamefeature/managers/entitymanager.h"
#include "util/arrayallocator.h"

//-----------------------------------------------------------------------------
namespace Game
{

template <class...Ts, std::size_t...Is>
void FillBlobSequenced(const Util::ArrayAllocator<Game::Entity, Ts...>& data, Util::Blob& blob, SizeT& offset, std::index_sequence<Is...>)
{
	SizeT numBytes;
	
	using expander = int[];
	(void)expander
	{
		0, (
			numBytes = data.GetArray<Is>().ByteSize(),
			blob.SetChunk(&data.GetArray<Is>()[0], numBytes, offset),
			offset += numBytes
		, 0)...
	};
}

template <class...Ts, std::size_t...Is>
void GetDataSize(const Util::ArrayAllocator<Game::Entity, Ts...>& data, SizeT& size, std::index_sequence<Is...>)
{
	using expander = int[];
	(void)expander
	{
		0,
			(size += data.GetArray<Is>().ByteSize(), 0)...
	};
}

template <class...Ts, std::size_t...Is>
void SetBlobSequenced(Util::ArrayAllocator<Game::Entity, Ts...>& data, uint from, uint to, const Util::Blob& blob, std::index_sequence<Is...>)
{

}


template <class ... TYPES>
class ComponentData
{
public:
	///
	ComponentData();
	///
	~ComponentData();

	SizeT Size() const;

	/// register an Id. Will create new mapping and allocate instance data. Returns index of new instance data
	uint32_t RegisterEntity(const Entity& e);

	/// deregister an Id. will only remove the id and zero the block
	void DeregisterEntity(const Entity& e);

	/// deregister an Id immediately. This will swap the last entity instance with this entitys' assuring a packed array.
	void DeregisterEntityImmediate(const Entity& e);

	/// deregister an Id immediately. This will swap the last entity instance with this entitys' assuring a packed array.
	void DeregisterEntityImmediate(const Entity& e, const uint32_t& index);

	/// perform garbage collection. Returns number of erased instances.
	SizeT Optimize();

	/// Destroys all instances and sets all memory used free.
	void DestroyAll();

	/// Deregister all inactive entities.
	void DeregisterAllInactive();

	/// Deregister all entities
	void DeregisterAll();

	/// Free up all non-reserved by entity data.
	void Clean();

	/// Return the owner of a given instance
	Entity GetOwner(const uint32_t& i) const;

	/// retrieve the instance id of an external id for faster lookup. Will be made invalid by Optimize()
	uint32_t GetInstance(const Entity& e) const;

	/// Shortcut to set all instances values to provided values.
	void SetInstanceData(const uint32_t& index, TYPES...);

	/// Return data as a blob.
	Util::Blob GetBlob() const;

	/// Set data from blob
	void SetBlob(uint from, uint to, const Util::Blob& data);

	/// Contains all data for all instances of this component.
	/// @note	The 0th type is always the owner Entity!
	Util::ArrayAllocator<Entity, TYPES...> data;
	
private:
	/// contains free id's that we reuse as soon as possible.
	Util::Stack<uint32_t> freeIds;

	/// Contains the link between InstanceData and Entity Id
	/// @todo	Replace with hashtable for faster lookup
	Util::Dictionary<Ids::Id32, uint32_t> idMap;
};


//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
ComponentData<TYPES ...>::ComponentData()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
ComponentData<TYPES ...>::~ComponentData()
{
	this->DestroyAll();
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> SizeT
ComponentData<TYPES ...>::Size() const
{
	return this->data.Size();
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> uint32_t
ComponentData<TYPES ...>::RegisterEntity(const Entity& e)
{
	n_assert2(!this->idMap.Contains(e.id), "ID has already been registered.");

	uint32_t index;

	if (this->freeIds.Size() > 0)
	{
		index = this->freeIds.Pop();
	}
	else
	{
		index = this->data.Alloc();
	}

	this->data.Get<0>(index) = e;
	this->idMap.Add(e.id, index);

	return index;
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> void
ComponentData<TYPES ...>::DeregisterEntity(const Entity& e)
{
	n_assert2(this->idMap.Contains(e.id), "Tried to remove an ID that had not been registered.");
	uint32_t index = this->idMap[e.id];
	this->idMap.Erase(e.id);
	this->freeIds.Push(index);
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> void
ComponentData<TYPES ...>::DeregisterEntityImmediate(const Entity& e)
{
	n_assert2(this->idMap.Contains(e.id), "Tried to remove an ID that had not been registered.");
	uint32_t index = this->idMap[e.id];
	this->DeregisterEntityImmediate(e, index);
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> void
ComponentData<TYPES ...>::DeregisterEntityImmediate(const Entity& e, const uint32_t& index)
{
	auto id = e.id;
	n_assert2(this->idMap.Contains(id), "Tried to remove an ID that had not been registered.");
	Ids::Id32 lastId = this->data.Get<0>(this->data.Size() - 1).id;
	this->data.EraseIndexSwap(index);
	uint32_t mapIndex = this->idMap.FindIndex(lastId);
	if (mapIndex != InvalidIndex)
	{
		this->idMap.ValueAtIndex(mapIndex) = index;
	}
	this->idMap.Erase(id);
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> SizeT
ComponentData<TYPES ...>::Optimize()
{
	Ptr<EntityManager> entityManager = EntityManager::Instance();
	uint numAlive = 0;
	SizeT numErased = 0;
	Ids::Id32 lastId;
	uint32_t index;
	uint32_t mapIndex;

	// Pack arrays
	SizeT size = this->freeIds.Size();
	for (SizeT i = 0; i < size; ++i)
	{
		index = this->freeIds.Pop();
		lastId = this->data.Get<0>(this->data.Size() - 1).id;
		this->data.EraseIndexSwap(index);
		mapIndex = this->idMap.FindIndex(lastId);
		if (mapIndex != InvalidIndex)
		{
			this->idMap.ValueAtIndex(mapIndex) = index;
		}
		++numErased;
	}

	// garbage collection
	// Runs until it hits four entities that are alive.
	while (this->data.Size() > 0 && numAlive < 4)
	{
		index = Util::FastRandom() % this->data.Size();
		if (entityManager->IsAlive(this->data.Get<0>(index)))
		{
			++numAlive;
			continue;
		}
		numAlive = 0;
		// Deregister entity and make sure it's removed from the list
		// so that we don't accidentally try to delete it again.
		this->DeregisterEntityImmediate(this->data.Get<0>(index), index);
		++numErased;
	}

	return numErased;
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> void
ComponentData<TYPES ...>::DestroyAll()
{
	this->freeIds.Clear();
	this->data.Clear();
	this->idMap.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> void
ComponentData<TYPES ...>::DeregisterAll()
{
	for (SizeT i = 0; i < this->idMap.Size(); i++)
	{
		this->freeIds.Push(this->idMap.ValueAtIndex(i));
		this->idMap.EraseAtIndex(i);
	}
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> void
ComponentData<TYPES ...>::DeregisterAllInactive()
{
	Ptr<Game::EntityManager> manager = Game::EntityManager::Instance();
	for (SizeT i = 0; i < this->idMap.Size(); i++)
	{
		Ids::Id32 id = this->idMap.KeyAtIndex(i);
		Entity e(id);
		if (!manager->IsAlive(e))
		{
			this->freeIds.Push(this->idMap.ValueAtIndex(i));
			this->idMap.EraseAtIndex(i);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> void
ComponentData<TYPES ...>::Clean()
{
	Ptr<Game::EntityManager> entityManager = Game::EntityManager::Instance();
	SizeT index = 0;
	while (index < this->data.Size())
	{
		if (!entityManager->IsAlive(this->data.Get<0>(index)))
		{
			this->data.EraseIndexSwap(index);
			continue;
		}
		index++;
	}
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> Entity
ComponentData<TYPES ...>::GetOwner(const uint32_t& i) const
{
	n_assert(this->data.Size() > i);
	return this->data.Get<0>(i);
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> uint32_t
ComponentData<TYPES ...>::GetInstance(const Entity& e) const
{
	auto i = this->idMap.FindIndex(e.id);
	if (i != InvalidIndex)
	{
		return this->idMap.ValueAtIndex(i);
	}

	// Entity is not registered.
	return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
template<class ... TYPES> void
ComponentData<TYPES...>::SetInstanceData(const uint32_t & index, TYPES ... values)
{
	this->data.Set(index, this->data.Get<0>(index), values...);
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
inline Util::Blob
ComponentData<TYPES...>::GetBlob() const
{
	SizeT numBytes = 0;

	GetDataSize(this->data, numBytes, std::make_index_sequence<sizeof...(TYPES)+1>());

	Util::Blob blob;

	SizeT offset = 0;

	blob.Reserve(numBytes);

	FillBlobSequenced(this->data, blob, offset, std::make_index_sequence<sizeof...(TYPES)+1>());

	return blob;
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
inline void
ComponentData<TYPES...>::SetBlob(uint from, uint to, const Util::Blob& data)
{

}

}
//-----------------------------------------------------------------------------