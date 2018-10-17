#pragma once
//------------------------------------------------------------------------------
/**
	@class Game::ComponentData

	ComponentData as a struct of arrays.
	Can perform garbage collection that rearranges arrays entries to avoid gaps

	(C) 2018 Individual contributors, see AUTHORS file
*/

#include "util/hashtable.h"
#include "util/stack.h"
#include "ids/id.h"
#include "util/random.h"
#include "basegamefeature/managers/entitymanager.h"
#include "util/arrayallocator.h"
#include "io/memorystream.h"
#include "io/binarywriter.h"
#include "io/binaryreader.h"

//-----------------------------------------------------------------------------
namespace Game
{

//------------------------------------------------------------------------------
/**
*/
template<class X>
__forceinline typename std::enable_if<std::is_trivial<X>::value == false, void>::type
Serialize(const Ptr<IO::BinaryWriter>& writer, const Util::Array<X>& data)
{
	static_assert(false, "Type is not trivial and does have a serialize template specialization!");
}

//------------------------------------------------------------------------------
/**
*/
template<class X>
__forceinline typename std::enable_if<std::is_trivial<X>::value == false, void>::type
Deserialize(const Ptr<IO::BinaryReader>& reader, Util::Array<X>& data, uint32_t offset, uint32_t numInstances)
{
	static_assert(false, "Type is not trivial and does have a Deserialize template specialization!");
}

//------------------------------------------------------------------------------
/**
*/
template<class X>
__forceinline typename std::enable_if<std::is_trivial<X>::value == true, void>::type
Serialize(const Ptr<IO::BinaryWriter>& writer, const Util::Array<X>& data)
{
	writer->WriteRawData((void*)&data[0], data.ByteSize());
}

//------------------------------------------------------------------------------
/**
*/
template<class X>
__forceinline typename std::enable_if<std::is_trivial<X>::value == true, void>::type
Deserialize(const Ptr<IO::BinaryReader>& reader, Util::Array<X>& data, uint32_t offset, uint32_t numInstances)
{
	reader->ReadRawData((void*)&data[offset], numInstances * data.TypeSize());
}

//------------------------------------------------------------------------------
/**
*/
template<>
__forceinline void
Serialize<Math::matrix44>(const Ptr<IO::BinaryWriter>& writer, const Util::Array<Math::matrix44>& data)
{
	writer->WriteRawData((void*)&data[0], data.ByteSize());
}

//------------------------------------------------------------------------------
/**
*/
template<>
__forceinline void
Deserialize<Math::matrix44>(const Ptr<IO::BinaryReader>& reader, Util::Array<Math::matrix44>& data, uint32_t offset, uint32_t numInstances)
{
	reader->ReadRawData((void*)&data[offset], numInstances * data.TypeSize());
}

//------------------------------------------------------------------------------
/**
*/
template<>
__forceinline void
Serialize<Math::float4>(const Ptr<IO::BinaryWriter>& writer, const Util::Array<Math::float4>& data)
{
	writer->WriteRawData((void*)&data[0], data.ByteSize());
}

//------------------------------------------------------------------------------
/**
*/
template<>
__forceinline void
Deserialize<Math::float4>(const Ptr<IO::BinaryReader>& reader, Util::Array<Math::float4>& data, uint32_t offset, uint32_t numInstances)
{
	reader->ReadRawData((void*)&data[offset], numInstances * data.TypeSize());
}

//------------------------------------------------------------------------------
/**
*/
template<>
__forceinline void
Serialize<Util::String>(const Ptr<IO::BinaryWriter>& writer, const Util::Array<Util::String>& data)
{
	// Write each string
	for (SizeT i = 0; i < data.Size(); ++i)
	{
		writer->WriteString(data[i]);
	}
}

//------------------------------------------------------------------------------
/**
*/
template<>
__forceinline void
Deserialize<Util::String>(const Ptr<IO::BinaryReader>& reader, Util::Array<Util::String>& data, uint32_t offset, uint32_t numInstances)
{
	// read each string
	for (SizeT i = 0; i < numInstances; ++i)
	{
		data[offset + i] = reader->ReadString();
	}
}

template <class...Ts, std::size_t...Is>
void WriteDataSequenced(const Util::ArrayAllocator<Game::Entity, Ts...>& data, const Ptr<IO::BinaryWriter>& writer, std::index_sequence<Is...>)
{
	writer->WriteRawData((void*)&data.GetArray<0>()[0], data.GetArray<0>().ByteSize());
	
	using expander = int[];
	(void)expander
	{
		0, (
		Serialize<Ts>(writer, data.GetArray<Is + 1>()), 0)...
	};
}

template <class...Ts, std::size_t...Is>
void ReadDataSequenced(Util::ArrayAllocator<Game::Entity, Ts...>& data, const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances, std::index_sequence<Is...>)
{
	if (data.GetArray<0>().Size() < offset + numInstances)
	{
		data.GetArray<0>().SetSize(offset + numInstances);
	}
	reader->ReadRawData((void*)&data.GetArray<0>()[offset], numInstances * data.GetArray<0>().TypeSize());

	using expander = int[];
	(void)expander
	{
		0, (
		data.GetArray<Is + 1>().SetSize(offset + numInstances),
		Deserialize<Ts>(reader, data.GetArray<Is + 1>(), offset, numInstances), 0)...
	};
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
	//void DeregisterAll();

	/// Free up all non-reserved by entity data.
	void Clean();

	/// Return the owner of a given instance
	Entity GetOwner(const uint32_t& i) const;

	/// Set the owner of a given instance. This does not care if the entity is registered or not!
	void SetOwner(const uint32_t& i, const Game::Entity& entity);

	/// retrieve the instance id of an external id for faster lookup. Will be made invalid by Optimize()
	uint32_t GetInstance(const Entity& e) const;

	/// Shortcut to set all instances values to provided values.
	void SetInstanceData(const uint32_t& index, TYPES...);

	/// Write data into writer.
	void Serialize(const Ptr<IO::BinaryWriter>& writer) const;

	/// Set data from blob
	void Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances);

	/// Contains all data for all instances of this component.
	/// @note	The 0th type is always the owner Entity!
	Util::ArrayAllocator<Entity, TYPES...> data;
	
private:
	const static int STACK_SIZE = 4;

	/// contains free id's that we reuse as soon as possible.
	Util::Stack<uint32_t> freeIds;

	/// Contains the link between InstanceData and Entity Id
	Util::HashTable<Ids::Id32, uint32_t, STACK_SIZE> idMap;
};


//------------------------------------------------------------------------------
/**
	@todo	idMap hashtable needs to be configured depending on the amount of entities we expect to be registered.
*/
template <class ... TYPES>
ComponentData<TYPES ...>::ComponentData() :
	idMap(1024)
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
		this->idMap.ValueAtIndex(lastId, mapIndex) = index;
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
			this->idMap.ValueAtIndex(lastId, mapIndex) = index;
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
ComponentData<TYPES ...>::DeregisterAllInactive()
{
	Ptr<Game::EntityManager> manager = Game::EntityManager::Instance();
	for (SizeT i = 0; i < this->data.Size(); i++)
	{
		Entity e = this->data.Get<0>(i);
		if (!manager->IsAlive(e))
		{
			this->freeIds.Push(this->idMap.ValueAtIndex(e.id, i));
			this->idMap.Erase(e.id);
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
template<class ... TYPES>
inline void ComponentData<TYPES ...>::SetOwner(const uint32_t & i, const Game::Entity& entity)
{
	this->data.Get<0>(i) = entity;

	auto index = this->idMap.FindIndex(entity.id);
	if (index != InvalidIndex)
	{
		this->idMap.ValueAtIndex(entity.id, index) = i;
		return;
	}

	this->idMap.Add(entity.id, i);
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
		return this->idMap.ValueAtIndex(e.id, i);
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
inline void
ComponentData<TYPES...>::Serialize(const Ptr<IO::BinaryWriter>& writer) const
{
	WriteDataSequenced(this->data, writer, std::make_index_sequence<sizeof...(TYPES)>());
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
inline void
ComponentData<TYPES...>::Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances)
{
	ReadDataSequenced(this->data, reader, offset, numInstances, std::make_index_sequence<sizeof...(TYPES)>());
}

}
//-----------------------------------------------------------------------------