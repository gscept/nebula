#pragma once
//------------------------------------------------------------------------------
/**
	BaseComponent

	Components derive from this class.

	@todo	Add Allocate virtual function for quickly allocating multiple instances.
			this->Alloc(numInstances)
			Also, we need a set data function for quickly setting instance data in a certain span
			this->SetData(start, end, void*)
			SetData function should automatically split the void* buffer into each struct of arrays blocks.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/entity.h"
#include "core/refcounted.h"
#include "util/bitfield.h"
#include "game/attr/attrid.h"
#include "game/attr/attributedefinition.h"
#include "game/entityattr.h"
#include "io/binaryreader.h"
#include "io/binarywriter.h"

namespace Game
{

enum ComponentEvent
{
	OnBeginFrame	= 0,
	OnRender		= 1,
	OnEndFrame		= 2,
	OnRenderDebug	= 3,
	OnActivate		= 4,
	OnDeactivate	= 5,
	NumEvents		= 6
};

class BaseComponent : public Core::RefCounted
{
	__DeclareClass(BaseComponent)
public:
	BaseComponent();
	~BaseComponent();

	/// Returns a bitfield containing the events this component is subscribed to.
	const Util::BitField<ComponentEvent::NumEvents>& SubscribedEvents() const;

	/// Registers an entity to this component. Will try to reuse old datablocks (clearing them first)
	/// within componentData and create new data if no free id is available.
	virtual void RegisterEntity(const Entity& entity);

	/// Deregisters an entity from this component. The data will still exist in the buffer until Optimize() is called.
	/// Note that even though this keeps the data intact, registering the same entity again won't result in the same data being used.
	virtual void DeregisterEntity(const Entity& entity);
	
	/// Called from entitymanager if this component is registered with a deletion callback.
	/// Immediately removes the data instance
	virtual void OnEntityDeleted(Entity entity);

	/// Deregister all non-alive entities from both inactive and active. This can be extremely slow!
	virtual void DeregisterAllDead();

	/// Cleans up right away and frees any memory that does not belong to an entity. This can be extremely slow!
	virtual void CleanData();

	/// Destroys all instances of this component, and deregisters every entity.
	virtual void DestroyAll();

	/// Return amount of registered entities
	virtual uint32_t GetNumInstances() const;

	/// returns an index to the instance data within the data buffer.
	virtual uint32_t GetInstance(const Entity& entity) const;

	/// returns a instances owner entity id
	virtual Entity GetOwner(const uint32_t& instance) const;

	/// Set the owner of a given instance. This does not care if the entity is registered or not!
	virtual void SetOwner(const uint32_t& i, const Game::Entity& entity);

	/// perform garbage collection. Returns number of erased instances.
	virtual SizeT Optimize();

	/// Returns an attribute value as a variant from index. Needs to be overloaded in subclass.
	virtual Util::Variant GetAttributeValue(uint32_t instance, IndexT attributeIndex) const;
	
	/// Returns an attribute value as a variant from attribute id. Needs to be overloaded in subclass.
	virtual Util::Variant GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const;
	
	/// Set an attribute value from index
	virtual void SetAttributeValue(uint32_t instance, IndexT attributeIndex, Util::Variant value);

	/// Set an attribute value from attribute id
	virtual void SetAttributeValue(uint32_t instance, Attr::AttrId attributeId, Util::Variant value);

	/// Returns attribute id at index.
	const Attr::AttrId& GetAttributeId(IndexT index) const;

	/// Returns an array with all attribute ids for this component
	const Util::FixedArray<Attr::AttrId>& GetAttributeIds() const;
		
	
	// Methods for custom implementations
	// ----------------------------------

	/// Allocate multiple instances quickly
	virtual void AllocInstances(uint num);
	
	/// Returns all attributes that are of Entity type.
	/// Just returns a pointer to each of the arrays containing the entity-ids
	virtual Util::Array<Util::Array<Game::Entity>*> GetEntityAttributes();

	/// Serialize component and write to binary stream
	virtual void Serialize(const Ptr<IO::BinaryWriter>& writer) const;

	/// Deserialize component from binary stream
	virtual void Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances);

	/// Called when relationships needs to be reconstructed
	/// parentIndices point into the entities array
	virtual void SetParents(const uint32_t& start, const uint32_t& end, const Util::Array<Entity>& entities, const Util::Array<uint32_t>& parentIndices);

	/// Called upon activation of component instance
	virtual void OnActivate(const uint32_t& instance);
	
	/// Called upon deactivation of component instance
	virtual void OnDeactivate(const uint32_t& instance);

	/// called at beginning of frame
	virtual void OnBeginFrame();

	/// called before rendering happens
	virtual void OnRender();

	/// called at the end of a frame
	virtual void OnEndFrame();

	/// called when game debug visualization is on
	virtual void OnRenderDebug();
	
protected:
	/// Holds all events this component is subscribed to.
	Util::BitField<ComponentEvent::NumEvents> events;

	/// Holds all attributedefinitions that this components has available.
	Util::FixedArray<Attr::AttrId> attributeIds;
};

} // namespace Game