#pragma once
//------------------------------------------------------------------------------
/**
	@class Game::ComponentDataData

	Base ComponentData that provides mapping from Game::Ids to an internal array index for ComponentDatas
	Can perform garbage collection that rearranges array entries to avoid gaps
	
	(C) 2017 Individual contributors, see AUTHORS file
*/

#include "util/dictionary.h"
#include "util/stack.h"
#include "ids/id.h"
#include "util/random.h"
#include "game/entitymanager.h"

//-----------------------------------------------------------------------------
namespace Game
{

template <class InstanceData>
class ComponentData
{
public:
	///
	ComponentData();
	///
	~ComponentData();


	/// access to a single instance data block by index
	InstanceData& operator[](uint32_t instance) const;

	/// register an Id. Will create new mapping and allocate instance data
	void RegisterEntity(const Entity& e);
	/// deregister an Id. will only remove the id and zero the block
	void DeregisterEntity(const Entity& e);
	/// perform garbage collection
	void Optimize();
	/// Destroys all instances and sets all memory used free.
	void DestroyAll();

	/// retrieve the instance id of an external id for faster lookup
	/// will be made invalid by Optimize()
	uint32_t GetInstance(const Entity& e) const;
	void SetInstanceData(const Entity& e, const InstanceData& data);
protected:

	/// contains free id's that we reuse as soon as possible.
	Util::Stack<uint32_t> freeIds;

	/// Contains the link between InstanceData and Id
	Util::Dictionary<Ids::Id32, uint32_t> idMap;

	/// Contains all data for all instances of this component.
	Util::Array<InstanceData> data;
};


//------------------------------------------------------------------------------
/**
*/
template <class InstanceData>
ComponentData<InstanceData>::ComponentData()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template <class InstanceData>
ComponentData<InstanceData>::~ComponentData()
{
	this->DestroyAll();
}


//------------------------------------------------------------------------------
/**
*/
template <class InstanceData> InstanceData&
ComponentData<InstanceData>::operator[](uint32_t instance) const
{
	return this->data[instance];
}

//------------------------------------------------------------------------------
/**
*/
template <class InstanceData> void
ComponentData<InstanceData>::RegisterEntity(const Entity& e)
{
	n_assert2(!this->idMap.Contains(e.id), "ID has already been registered.");

	uint32_t index;

	if (this->freeIds.Size() > 0)
	{
		index = this->freeIds.Pop();
		this->data[index] = InstanceData();
	}
	else
	{
		index = this->data.Size();
		this->data.Append(InstanceData());
	}
	
	this->data[index].owner = e;
	this->idMap.Add(e.id, index);
}

//------------------------------------------------------------------------------
/**
*/
template <class InstanceData> void
ComponentData<InstanceData>::DeregisterEntity(const Entity& e)
{
	n_assert2(this->idMap.Contains(e.id), "Tried to remove an ID that had not been registered.");
	SizeT index = this->idMap[e.id];
	this->idMap.Erase(e.id);
	this->freeIds.Push(index);
}

//------------------------------------------------------------------------------
/**
*/
template <class InstanceData> void
ComponentData<InstanceData>::Optimize()
{
	Ptr<EntityManager> entityManager = EntityManager::Instance();
	uint numAlive = 0;
	SizeT size;
	Ids::Id32 lastId;
	uint32_t index;
	
	// Pack array
	size = this->freeIds.Size();
	for (SizeT i = 0; i < size; ++i)
	{
		index = this->freeIds.Pop();
		lastId = this->data.Back().owner.id;
		this->idMap[lastId] = index;
		this->data.EraseIndexSwap(index);
	}

	// garbage collection
	// Runs until it hits four entities that are alive.
	while (this->data.Size() > 0 && numAlive < 4)
	{
		index = Util::FastRandom() % this->data.Size();
		if (entityManager->IsAlive(this->data[index].owner))
		{
			++numAlive;
			continue;
		}
		numAlive = 0;
		// Deregister entity and make sure it's removed from the list
		// so that we don't accidentally try to delete it again.
		auto id = this->data[index].owner.id;
		n_assert2(this->idMap.Contains(id), "Garbage collection tried to remove an ID that had not been registered.");
		lastId = this->data.Back().owner.id;
		this->data.EraseIndexSwap(index);
		this->idMap[lastId] = index;
		this->idMap.Erase(id);
	}
}

//------------------------------------------------------------------------------
/**
*/
template<class InstanceData> void
ComponentData<InstanceData>::DestroyAll()
{
	this->freeIds.Clear();
	this->data.Clear();
	this->idMap.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template <class InstanceData> uint32_t
ComponentData<InstanceData>::GetInstance(const Entity& e) const
{
	n_assert(this->idMap.Contains(e.id));
	return this->idMap[e.id];
}

//------------------------------------------------------------------------------
/**
*/
template<class InstanceData> void
ComponentData<InstanceData>::SetInstanceData(const Entity& e, const InstanceData& data)
{
	this->data[this->idMap[e.id]] = data;
}

}
//-----------------------------------------------------------------------------