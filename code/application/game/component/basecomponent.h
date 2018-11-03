#pragma once
//------------------------------------------------------------------------------
/**
	BaseComponent

	Components derive from this class.

	A component is a collection of data and functionality that makes up the
	behaviour of a game's entities.
	Entities are registered to components and the components handle the internal
	mapping between entity and their data instance.

	The variables that component instances hold are described as attributes.
	Each component contains a list of attribute definitions that can be read to
	get an understanding of the variables that each component instance holds.
	@see	game/attr/attributedefinition.h

	Components are usually registered to the ComponentManager and each event is
	called from there via delegates. Each event that a component want to subscribe
	to must be setup before being registered to the component manager.
	@see	basegamefeature/managers/componentmanager.h
	@see	foundation/util/delegate.h

	Components communicate with each other via messages. A component can subscribe
	to messages with delegates.
	@see	game/messaging/message.h

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
#include "game/messaging/message.h"

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
	virtual uint32_t RegisterEntity(const Entity& entity);

	/// Deregisters an entity from this component. The data will still exist in the buffer until Optimize() is called.
	/// Note that even though this keeps the data intact, registering the same entity again won't result in the same data being used.
	virtual void DeregisterEntity(const Entity& entity);
	
	/// Called from entitymanager if this component is registered with a deletion callback.
	/// Immediately removes the data instance
	virtual void OnEntityDeleted(Entity entity);

	/// Cleans up right away and frees any memory that does not belong to an entity. This can be extremely slow!
	virtual void Clean();

	/// Setup accepted messages
	virtual void SetupAcceptedMessages();

	/// Destroys all instances of this component, and deregisters every entity.
	virtual void DestroyAll();

	/// Return amount of registered entities
	virtual SizeT NumRegistered() const;

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
	virtual void Allocate(uint num);
	
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

	Util::Array<MessageListener> messageListeners;

	/// Holds all attributedefinitions that this components has available.
	Util::FixedArray<Attr::AttrId> attributeIds;
};

} // namespace Game