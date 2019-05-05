#pragma once
//------------------------------------------------------------------------------
/**
	@class Game::Component

	The implements a structure of arrays type component "manager".
	Inherit from this to avoid having to write tons of boilerplate.

	This implements a single instance component, which means you cannot
	have multiple instances of this component registered to the same entity.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//-----------------------------------------------------------------------------
#include "util/hashtable.h"
#include "util/stack.h"
#include "ids/id.h"
#include "util/random.h"
#include "basegamefeature/managers/entitymanager.h"
#include "util/arrayallocator.h"
#include "game/component/componentinterface.h"
#include "game/attr/attrid.h"
#include "componentserialization.h"
#include "game/component/attribute.h"
#include <tuple>

//------------------------------------------------------------------------------
/**
	Use this in your component's Create() method before registering
	to the component manager to implement default behaviour
*/
#define __SetupDefaultComponentBundle(ALLOCATOR) \
	ALLOCATOR->functions.DestroyAll = DestroyAll; \
	ALLOCATOR->functions.Serialize = Serialize; \
	ALLOCATOR->functions.Deserialize = Deserialize; 

//------------------------------------------------------------------------------
/**
	Shorthand for registering to the component manager.
	Remember to include componentmanager.h!
*/
#define __RegisterComponent(ALLOCATOR, COMPONENTNAME) \
	Game::ComponentManager::Instance()->RegisterComponent(ALLOCATOR, ##COMPONENTNAME, ALLOCATOR->GetIdentifier());

//------------------------------------------------------------------------------
/**
	Declares default functionality of a component.

	Use __ImplementComponent in the source file.
*/
#define __DeclareComponent(COMPONENT) \
	public: \
		static void Create(); \
		static void Discard(); \
		static Game::InstanceId RegisterEntity(Game::Entity entity); \
		static void DeregisterEntity(Game::Entity entity); \
		static void DestroyAll(); \
		static SizeT NumRegistered(); \
		static void Serialize(const Ptr<IO::BinaryWriter>& writer); \
		static void Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances); \
		static Game::InstanceId GetInstance(Game::Entity entity); \
	private:

//------------------------------------------------------------------------------
/**
	Implements default behaviour of a component.

	Remember to write implementations for Create() and Discard()!
*/
#define __ImplementComponent(COMPONENTTYPE, ALLOCATOR) \
	Game::InstanceId COMPONENTTYPE::RegisterEntity(Game::Entity entity) { return ALLOCATOR->RegisterEntity(entity); } \
	void COMPONENTTYPE::DeregisterEntity(Game::Entity entity) { ALLOCATOR->DeregisterEntity(entity); } \
	void COMPONENTTYPE::DestroyAll() { ALLOCATOR->DestroyAll(); } \
	SizeT COMPONENTTYPE::NumRegistered() { return ALLOCATOR->NumRegistered(); } \
	void COMPONENTTYPE::Serialize(const Ptr<IO::BinaryWriter>& writer) { ALLOCATOR->Serialize(writer); } \
	void COMPONENTTYPE::Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances) { ALLOCATOR->Deserialize(reader, offset, numInstances); } \
	Game::InstanceId COMPONENTTYPE::GetInstance(Game::Entity entity) { return ALLOCATOR->GetInstance(entity); } 

//------------------------------------------------------------------------------
/**
	Implements default behaviour of a component.

	Use this if you plan to implement your own serialization functions.

	Remember to write implementations for Create() and Discard()!
*/
#define __ImplementComponent_woSerialization(COMPONENTTYPE, ALLOCATOR) \
	Game::InstanceId COMPONENTTYPE::RegisterEntity(Game::Entity entity) { return ALLOCATOR->RegisterEntity(entity); } \
	void COMPONENTTYPE::DeregisterEntity(Game::Entity entity) { ALLOCATOR->DeregisterEntity(entity); } \
	void COMPONENTTYPE::DestroyAll() { ALLOCATOR->DestroyAll(); } \
	SizeT COMPONENTTYPE::NumRegistered() { return ALLOCATOR->NumRegistered(); } \
	Game::InstanceId COMPONENTTYPE::GetInstance(Game::Entity entity) { return ALLOCATOR->GetInstance(entity); } 

namespace Game
{

struct ComponentCreateInfo
{
	bool incrementalDeletion = false;
};

template <typename ... TYPES>
class Component : public ComponentInterface
{
protected:
	/// short hand for getting the component with template arguments filled
	using component_templated_t = Component<TYPES...>;

	ComponentCreateInfo settings;
public:
	Component();
	Component(ComponentCreateInfo const& settings);
	~Component();

	SizeT NumRegistered() const;

	/// register an Id. Will create new mapping and allocate instance data. Returns index of new instance data
	InstanceId RegisterEntity(Entity e);

	/// deregister an Id. will only remove the id and zero the block
	void DeregisterEntity(Entity e);
	
	/// Destroys all instances and sets all memory used free.
	void DestroyAll();

	/// Deregister all inactive entities.
	void DeregisterAllInactive();

	/// Free up all non-reserved by entity data.
	void Clean();

	/// Return the owner of a given instance
	Entity GetOwner(InstanceId i) const;

	/// Set the owner of a given instance. This does not care if the entity is registered or not!
	void SetOwner(InstanceId i, Game::Entity entity);

	/// retrieve the instance id of an external id for faster lookup. Will be made invalid by Optimize()
	InstanceId GetInstance(Entity e) const;

	/// Shortcut to set all instances values to provided values.
	void SetInstanceData(InstanceId index, typename TYPES::InnerType...);

	/// Write data into writer.
	void Serialize(const Ptr<IO::BinaryWriter>& writer) const;

	/// Set data from blob
	void Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances);
		
	/// Allocate multiple instances
	void Allocate(uint num);

	/// get single item from resource,
	template <typename ATTR>
	typename ATTR::InnerType& Get(const InstanceId instance);

	/// const version of get
	template <typename ATTR>
	const typename ATTR::InnerType& Get(const InstanceId instance) const;

	/// Reset instance to default values.
	void SetToDefault(InstanceId instance);

	/// Callback for when an entity is deleted. Essentially just call DeregisterEntityImmediate
	void OnEntityDeleted(Game::Entity entity);
	
	/// deregister an Id immediately. This will swap the last entity instance with this entitys' assuring a packed array.
	void DeregisterEntityImmediate(Entity e);

	/// deregister an Id immediately. This will swap the last entity instance with this entitys' assuring a packed array.
	void DeregisterEntityImmediate(Entity e, InstanceId index);

	/// perform garbage collection. Returns number of erased instances.
	SizeT Optimize();

	/// Returns the attribute index of a specific attribute type
	template<typename ATTR>
	static constexpr std::size_t GetAttributeIndex();

	/// Get attribute value as a variant. This is generally quite slow, so use with care!
	Util::Variant GetAttributeValue(InstanceId instance, IndexT attributeIndex);

	/// Get attribute value as a variant. This is generally quite slow, so use with care!
	Util::Variant GetAttributeValue(InstanceId instance, Util::FourCC attribute);

	/// Set attribute value as a variant. This is generally quite slow and won't propagate to other components, so use with care!
	void SetAttributeValue(InstanceId instance, IndexT attributeIndex, const Util::Variant& value);

	/// Set attribute value as a variant. This is generally quite slow and won't propagate to other components, so use with care!
	void SetAttributeValue(InstanceId instance, Util::FourCC attribute, const Util::Variant& value);

	/// Write owners into writer.
	void SerializeOwners(const Ptr<IO::BinaryWriter>& writer) const;

	/// Set owners from reader
	void DeserializeOwners(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances);

	/// Contains all data for all instances of this component.
	/// @note	The 0th type is always the owner Entity!
	/// @note	attribute types need to be in exactly the same order as in the attribute ids list.
	Util::ArrayAllocator<Entity, typename TYPES::InnerType...> data;

private:
	
	/// Initialize attribute list.
	template<std::size_t...Is>
	void Init(std::index_sequence<Is...>);

	/// Expansion method for setting default values of all types of an instance.
	template<std::size_t...Is>
	void SetToDefault(InstanceId instance, std::index_sequence<Is...>);

	/// Allocate num amount of instances. Owner is not automatically set!
	template<std::size_t...Is>
	void Allocate(uint num, std::index_sequence<Is...>);

	/// Get attribute value expansion method
	template<std::size_t n>
	constexpr Util::Variant GetAttributeValueDynamic(InstanceId instance, IndexT attributeIndex);
	
	/// Get attribute value expansion method
	template<std::size_t n>
	constexpr Util::Variant GetAttributeValueDynamic(InstanceId instance, Util::FourCC attribute);

	/// Set attribute value expansion method
	template<std::size_t n>
	constexpr void SetAttributeValueDynamic(InstanceId instance, IndexT attributeIndex, const Util::Variant& value);

	/// Set attribute value expansion method
	template<std::size_t n>
	constexpr void SetAttributeValueDynamic(InstanceId instance, Util::FourCC attribute, const Util::Variant& value);

	/// notify callback if certain requirements are met
	void NotifyOnInstanceMoved(InstanceId index, InstanceId oldIndex);

	/// contains free id's that we reuse as soon as possible.
	Util::Stack<InstanceId> freeIds;

	const static int STACK_SIZE = 1;
	const static int TABLE_SIZE = 1024;
	/// Contains the link between InstanceData and Entity Id
	Util::HashTable<Ids::Id32, InstanceId, TABLE_SIZE, STACK_SIZE> idMap;
};

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
Component<TYPES...>::Component()
{
	this->Init(std::make_index_sequence<sizeof...(TYPES)>());
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
Component<TYPES...>::Component(ComponentCreateInfo const& settings) :
	settings(settings)
{
	this->Init(std::make_index_sequence<sizeof...(TYPES)>());
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
template <std::size_t...Is>
void Component<TYPES...>::Init(std::index_sequence<Is...>)
{
	this->attributes.SetSize(sizeof...(TYPES) + 1);
	// We always have an owner
	this->attributes[0] = Attr::Owner(0);

	using expander = int[];
	(void)expander
	{
		0, (
			this->attributes[Is + 1] = TYPES(Is + 1),
		0)...
	};
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
Component<TYPES ...>::~Component()
{
	// this->DestroyAll();
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> SizeT
Component<TYPES ...>::NumRegistered() const
{
	return this->data.Size();
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
void Component<TYPES...>::SetToDefault(InstanceId instance)
{
	this->SetToDefault(instance, std::make_index_sequence<sizeof...(TYPES)>());
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
template <std::size_t...Is>
void Component<TYPES...>::SetToDefault(InstanceId instance, std::index_sequence<Is...>)
{
	using expander = int[];
	(void)expander
	{
		0, (
		this->data.Get<Is + 1>(instance) = TYPES::DefaultValue(),
		0)...
	};
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
void Component<TYPES...>::Allocate(uint num)
{
	this->Allocate(num, std::make_index_sequence<sizeof...(TYPES)>());
}

//------------------------------------------------------------------------------
/**
*/
template<class ... TYPES>
template<typename ATTR>
inline typename ATTR::InnerType&
Component<TYPES...>::Get(const InstanceId instance)
{
	constexpr std::size_t n = GetAttributeIndex<ATTR>();
	return this->data.Get<n>(instance);
}

//------------------------------------------------------------------------------
/**
*/
template<class ... TYPES>
template<typename ATTR>
inline const typename ATTR::InnerType&
Component<TYPES...>::Get(const InstanceId instance) const
{
	return this->data.Get<GetAttributeIndex<ATTR>()>(instance);
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
template <std::size_t...Is>
void Component<TYPES...>::Allocate(uint num, std::index_sequence<Is...>)
{
	SizeT first = this->data.Size();
	this->data.Reserve(first + num);
	this->data.GetArray<0>().SetSize(first + num);
	using expander = int[];
	(void)expander
	{
		0, (
		this->data.GetArray<Is + 1>().SetSize(first + num),
		this->data.GetArray<Is + 1>().Fill(first, num, TYPES::DefaultValue()),
		0)...
	};

	// Size is never updated when reserve is called, so we need to call it explicitly
	this->data.UpdateSize();
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> InstanceId
Component<TYPES ...>::RegisterEntity(Entity e)
{
	IndexT i = this->idMap.FindIndex(e.id);
	if (i != InvalidIndex)
	{
		return this->idMap.ValueAtIndex(e.id, i);
	}

	InstanceId index;

	if (this->freeIds.Size() > 0)
	{
		index = this->freeIds.Pop();
	}
	else
	{
		index = this->data.Alloc();
	}

	this->data.Get<0>(index) = e;
	this->SetToDefault(index, std::make_index_sequence<sizeof...(TYPES)>());

	this->idMap.Add(e.id, index);

	if (this->events.IsSet<ComponentEvent::OnActivate>())
	{
		this->functions.OnActivate(index);
	}

	if (!this->settings.incrementalDeletion)
	{
		Game::EntityManager::Instance()->RegisterDeletionCallback(e, this);
	}

	return index;
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> void
Component<TYPES ...>::DeregisterEntity(Entity e)
{
	n_assert2(this->idMap.Contains(e.id), "Tried to remove an ID that had not been registered.");
	
	InstanceId index = this->idMap[e.id];
	if (index == InvalidIndex)
		return;

	if (this->events.IsSet<ComponentEvent::OnDeactivate>())
	{
		this->functions.OnDeactivate(index);
	}

	if (!this->settings.incrementalDeletion)
	{
		Game::EntityManager::Instance()->DeregisterDeletionCallback(e, this);
	}

	this->idMap.Erase(e.id);
	this->freeIds.Push(index);
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> void
Component<TYPES ...>::OnEntityDeleted(Game::Entity entity)
{
	InstanceId index = this->GetInstance(entity);
	if (index != InvalidIndex)
	{
		if (this->events.IsSet<ComponentEvent::OnDeactivate>())
		{
			this->functions.OnDeactivate(index);
		}

		this->DeregisterEntityImmediate(entity, index);
		return;
	}
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> void
Component<TYPES ...>::DeregisterEntityImmediate(Entity e)
{
	n_assert2(this->idMap.Contains(e.id), "Tried to remove an ID that had not been registered.");
	InstanceId index = this->idMap[e.id];
	if (index == InvalidIndex)
		return;
	this->DeregisterEntityImmediate(e, index);
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> void
Component<TYPES ...>::DeregisterEntityImmediate(Entity e, InstanceId index)
{
	auto id = e.id;
	n_assert2(this->idMap.Contains(id), "Tried to remove an ID that had not been registered.");
	InstanceId oldIndex = this->data.Size() - 1;
	Ids::Id32 lastId = this->data.Get<0>(this->data.Size() - 1).id;
	this->data.EraseIndexSwap(index);
	InstanceId mapIndex = this->idMap.FindIndex(lastId);
	if (mapIndex != InvalidIndex)
	{
		this->idMap.ValueAtIndex(lastId, mapIndex) = index;
	}
	this->idMap.Erase(id);

	this->NotifyOnInstanceMoved(index, oldIndex);
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> SizeT
Component<TYPES ...>::Optimize()
{
	Ptr<EntityManager> entityManager = EntityManager::Instance();
	uint numAlive = 0;
	SizeT numErased = 0;
	Ids::Id32 lastId;
	InstanceId index;
	InstanceId oldIndex;
	InstanceId mapIndex;

	// Pack arrays
	SizeT size = this->freeIds.Size();
	for (SizeT i = 0; i < size; ++i)
	{
		index = this->freeIds.Pop();
		oldIndex = this->data.Size() - 1;
		lastId = this->data.Get<0>(oldIndex).id;
		this->data.EraseIndexSwap(index);
		mapIndex = this->idMap.FindIndex(lastId);
		if (mapIndex != InvalidIndex)
		{
			this->idMap.ValueAtIndex(lastId, mapIndex) = index;
		}

		this->NotifyOnInstanceMoved(index, oldIndex);
		++numErased;
	}

	if (this->settings.incrementalDeletion)
	{
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
	}

	return numErased;
}


// Dirty c++ ahead
template <typename T, typename... Ts>
struct Index;

template <typename T, typename... Ts>
struct Index<T, T, Ts...> : std::integral_constant<std::size_t, 0> {};

template <typename T, typename U, typename... Ts>
struct Index<T, U, Ts...> : std::integral_constant<std::size_t, 1 + Index<T, Ts...>::value> {};

//------------------------------------------------------------------------------
/**
*/
template<typename ... TYPES>
template<typename ATTR>
constexpr std::size_t
Component<TYPES...>::GetAttributeIndex()
{
	// Index<...> calculates during compiletime at which index the first occurence of ATTR is.
	return Index<ATTR, Attr::Owner, TYPES...>::value;
}

//------------------------------------------------------------------------------
/**
*/
template<typename ... TYPES>
inline Util::Variant
Component<TYPES...>::GetAttributeValue(InstanceId instance, IndexT attributeIndex)
{
	n_assert2(attributeIndex <= sizeof...(TYPES), "Index out of range");
	n_assert2(instance < this->data.Size(), "Invalid instance id");
	// Start at index 0
	return this->GetAttributeValueDynamic<1>(instance, attributeIndex);
}

//------------------------------------------------------------------------------
/**
*/
template<typename ... TYPES>
inline Util::Variant
Component<TYPES...>::GetAttributeValue(InstanceId instance, Util::FourCC attribute)
{
	n_assert2(instance < this->data.Size(), "Invalid instance id");
	return this->GetAttributeValueDynamic<1>(instance, attribute);
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
template <std::size_t n>
constexpr Util::Variant
Component<TYPES...>::GetAttributeValueDynamic(InstanceId instance, IndexT attributeIndex)
{
	if (attributeIndex == n)
		return this->data.Get<n>(instance);
	else // Recurse and increase n by 1
		return GetAttributeValueDynamic<(n < sizeof...(TYPES) ? n + 1 : 1)>(instance, attributeIndex);
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
template <std::size_t n>
constexpr Util::Variant
Component<TYPES...>::GetAttributeValueDynamic(InstanceId instance, Util::FourCC attribute)
{
	using T = typename std::tuple_element<n, std::tuple<Game::Entity, TYPES...> >::type;
	if (attribute == T::FourCC())
		return this->data.Get<n>(instance);
	else // Recurse and increase n by 1
		return GetAttributeValueDynamic<(n < sizeof...(TYPES) ? n + 1 : 1)>(instance, attribute);
}

//------------------------------------------------------------------------------
/**
*/
template<typename ... TYPES>
inline void
Component<TYPES...>::SetAttributeValue(InstanceId instance, IndexT attributeIndex, const Util::Variant & value)
{
	n_assert2(attributeIndex <= sizeof...(TYPES), "Index out of range");
	n_assert2(instance < this->data.Size(), "Invalid instance id");
	
	this->SetAttributeValueDynamic<1>(instance, attributeIndex, value);
}

//------------------------------------------------------------------------------
/**
*/
template<typename ... TYPES>
void
Component<TYPES...>::SetAttributeValue(InstanceId instance, Util::FourCC attribute, const Util::Variant & value)
{
	n_assert2(instance < this->data.Size(), "Invalid instance id");

	this->SetAttributeValueDynamic<1>(instance, attribute, value);
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
template <std::size_t n>
constexpr void
Component<TYPES...>::SetAttributeValueDynamic(InstanceId instance, IndexT attributeIndex, const Util::Variant& value)
{
	if (attributeIndex == n)
	{
		auto& val = this->data.Get<n>(instance);
		val = value.Get<std::remove_reference<decltype(val)>::type>();
	}
	else // Recurse and increase n by 1
		SetAttributeValueDynamic<(n < sizeof...(TYPES) ? n + 1 : 1)>(instance, attributeIndex, value);
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
template <std::size_t n>
constexpr void
Component<TYPES...>::SetAttributeValueDynamic(InstanceId instance, Util::FourCC attribute, const Util::Variant& value)
{
	using T = typename std::tuple_element<n, std::tuple<Game::Entity, TYPES...>>::type;
	if (attribute == T::FourCC())
	{
		auto& val = this->data.Get<n>(instance);
		val = value.Get<T::InnerType>();
	}
	else // Recurse and increase n by 1
		SetAttributeValueDynamic<(n < sizeof...(TYPES) ? n + 1 : 1)>(instance, attribute, value);
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> void
Component<TYPES ...>::DestroyAll()
{
	if (!this->settings.incrementalDeletion)
	{
		SizeT length = this->data.Size();
		for (SizeT i = 0; i < length; i++)
		{
			Game::EntityManager::Instance()->DeregisterDeletionCallback(this->GetOwner(i), this);
		}
	}

	this->freeIds.Clear();
	this->data.Clear();
	this->idMap.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES> void
Component<TYPES ...>::DeregisterAllInactive()
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
Component<TYPES ...>::Clean()
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
Component<TYPES ...>::GetOwner(InstanceId i) const
{
	n_assert(this->data.Size() > i);
	return this->data.Get<0>(i);
}

//------------------------------------------------------------------------------
/**
*/
template<class ... TYPES>
inline void Component<TYPES ...>::SetOwner(InstanceId i, Game::Entity entity)
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
template <class ... TYPES> InstanceId
Component<TYPES ...>::GetInstance(Entity e) const
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
Component<TYPES...>::SetInstanceData(InstanceId index, typename TYPES::InnerType ... values)
{
	this->data.Set(index, this->data.Get<0>(index), values...);
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
inline void
Component<TYPES...>::Serialize(const Ptr<IO::BinaryWriter>& writer) const
{
	WriteDataSequenced(this->data, writer, std::make_index_sequence<sizeof...(TYPES)>());
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
inline void
Component<TYPES...>::Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances)
{
	ReadDataSequenced(this->data, reader, offset, numInstances, std::make_index_sequence<sizeof...(TYPES)>());
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
inline void
Component<TYPES...>::SerializeOwners(const Ptr<IO::BinaryWriter>& writer) const
{
	Game::Serialize(writer, this->data.GetArray<0>());
}

//------------------------------------------------------------------------------
/**
*/
template <class ... TYPES>
inline void
Component<TYPES...>::DeserializeOwners(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances)
{
	Game::Deserialize(reader, this->data.GetArray<0>(), offset, numInstances);
}

//------------------------------------------------------------------------------
/**
*/
template<class ... TYPES>
inline void
Component<TYPES...>::NotifyOnInstanceMoved(InstanceId index, InstanceId oldIndex)
{
	if (this->functions.OnInstanceMoved != nullptr)
	{
		// make sure the instance has actually moved and that
		// we actually have any registered entities left
		if (NumRegistered() != 0 && index != oldIndex)
			this->functions.OnInstanceMoved(index, oldIndex);
	}
}

}
//-----------------------------------------------------------------------------