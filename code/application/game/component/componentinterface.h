#pragma once
//------------------------------------------------------------------------------
/**
	ComponentInterface

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
	OnLoad			= 7,
	OnSave			= 9,
	NumEvents		= 10
};

class ComponentInterface
{
	__DeclareClass(ComponentInterface)
public:
	ComponentInterface();
	~ComponentInterface();

	/// register an Id. Will create new mapping and allocate instance data. Returns index of new instance data
	virtual uint32_t RegisterEntity(Entity e) = 0;

	/// deregister an Id. will only remove the id and zero the block
	virtual void DeregisterEntity(Entity e) = 0;

	/// Returns a bitfield containing the events this component is subscribed to.
	const Util::BitField<ComponentEvent::NumEvents>& SubscribedEvents() const;

	/// Returns attribute id at index.
	const Attr::AttrId& GetAttributeId(IndexT index) const;

	/// Returns an array with all attribute ids for this component
	const Util::FixedArray<Attr::AttrId>& GetAttributeIds() const;

	/// Callback for when an entity has been deleted.
	/// This in only used when immediate instance deletion is required.
	virtual void OnEntityDeleted(Game::Entity) = 0;

	/// Allocate a number of instances
	virtual void Allocate(uint num) = 0;

	/// Returns the number of registered entities
	virtual SizeT NumRegistered() const = 0;

	/// Returns the owner (entity) of an instance
	virtual Game::Entity GetOwner(uint instance) const = 0;

	/// Sets the owner of an instance. This should be used cautiously.
	virtual void SetOwner(uint32_t i, Game::Entity entity) = 0;

	/// Get an attribute value as a variant type
	virtual Util::Variant GetAttributeValue(uint32_t i, IndexT attributeIndex) = 0;
	
	/// Set an attribute value from a variant type
	virtual void SetAttributeValue(uint32_t i, IndexT attributeIndex, const Util::Variant& value) = 0;

	/// Returns the instance of an entity; or InvalidIndex if not registered.
	virtual uint32_t GetInstance(Entity e) const = 0;

	/// Subsequently calls functionbundle serialize.
	virtual void SerializeOwners(const Ptr<IO::BinaryWriter>& writer) const = 0;

	/// Subsequently calls functionbundle deserialize.
	virtual void DeserializeOwners(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances) = 0;

	struct FunctionBundle
	{
		/// Called upon activation of component instance
		void(*OnActivate)(uint32_t instance);

		/// Called upon deactivation of component instance
		void(*OnDeactivate)(uint32_t instance);

		/// called at beginning of frame
		void(*OnBeginFrame)();

		/// called before rendering happens
		void(*OnRender)();

		/// called at the end of a frame
		void(*OnEndFrame)();

		/// called when game debug visualization is on
		void(*OnRenderDebug)();

		/// called after an entity has been loaded from file.
		void(*OnLoad)(uint32_t instance);

		/// called after an entity has been save to a file.
		void(*OnSave)(uint32_t instance);

		/// Serialize the components attributes (excluding owners)
		void(*Serialize)(const Ptr<IO::BinaryWriter>& writer);

		/// Deserialize the components attributes (excluding owners)
		void(*Deserialize)(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances);

		/// Garbage collect
		SizeT(*Optimize)();

		/// Destroy all instances
		void(*DestroyAll)();

		/// Callback for when entities has been loaded and you need to hook into the hierarchy update.
		void(*SetParents)(uint32_t start, uint32_t end, const Util::Array<Entity>& entities, const Util::Array<uint32_t>& parentIndices);
	} functions;

	Util::Array<MessageListener> messageListeners;
protected:
	/// Holds all events this component is subscribed to.
	Util::BitField<ComponentEvent::NumEvents> events;
	
	/// Holds all attributedefinitions that this components has available.
	Util::FixedArray<Attr::AttrId> attributeIds;
};

} // namespace Game