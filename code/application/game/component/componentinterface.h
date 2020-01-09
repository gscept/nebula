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

	(C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/entity.h"
#include "core/refcounted.h"
#include "util/bitfield.h"
// #include "game/component/attribute.h"
#include "game/entityattr.h"
#include "io/binaryreader.h"
#include "io/binarywriter.h"
#include "game/messaging/message.h"
#include "attr/attrid.h"

namespace Game
{

typedef Ids::Id32 InstanceId;

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

static constexpr const char* ComponentEventNames[] = {
	"OnBeginFrame",
	"OnRender",
	"OnEndFrame",
	"OnRenderDebug",
	"OnActivate",
	"OnDeactivate",
	"OnLoad",
	"OnSave"
};

class ComponentInterface
{
public:
	ComponentInterface();
	~ComponentInterface();

	const Util::StringAtom& GetName() const;

	Util::FourCC GetIdentifier() const;

	/// register an Id. Will create new mapping and allocate instance data. Returns index of new instance data
	virtual InstanceId RegisterEntity(Entity e) = 0;

	/// deregister an Id. will only remove the id and zero the block
	virtual void DeregisterEntity(Entity e) = 0;

	/// Deregister all inactive entities.
	virtual void DeregisterAllInactive() = 0;

	/// Free up all non-reserved by entity data.
	virtual void Clean() = 0;

	/// Returns a bitfield containing the events this component is subscribed to.
	const Util::BitField<ComponentEvent::NumEvents>& SubscribedEvents() const;

	/// Returns attribute id at index.
	const Attr::AttrId& GetAttribute(IndexT index) const;

	/// Returns an array with all attribute ids for this component
	const Util::FixedArray<Attr::AttrId>& GetAttributes() const;

	/// Callback for when an entity has been deleted.
	/// This in only used when immediate instance deletion is required.
	virtual void OnEntityDeleted(Game::Entity) = 0;

	/// Allocate a number of instances
	virtual void Allocate(uint num) = 0;

	/// Returns the number of registered entities
	virtual SizeT NumRegistered() const = 0;

	/// Returns the owner (entity) of an instance
	virtual Game::Entity GetOwner(InstanceId instance) const = 0;

	/// Sets the owner of an instance. This should be used cautiously.
	virtual void SetOwner(InstanceId i, Game::Entity entity) = 0;

	/// Get an attribute value as a variant type
	virtual Util::Variant GetAttributeValue(InstanceId i, IndexT attributeIndex) = 0;
	
	/// Get an attribute value as a variant type
	virtual Util::Variant GetAttributeValue(InstanceId i, Util::FourCC attributeId) = 0;
	
	/// Set an attribute value from a variant type
	virtual void SetAttributeValue(InstanceId i, IndexT attributeIndex, const Util::Variant& value) = 0;
	
	/// Set an attribute value from a variant type
	virtual void SetAttributeValue(InstanceId i, Util::FourCC attributeId, const Util::Variant& value) = 0;

	/// Returns the instance of an entity; or InvalidIndex if not registered.
	virtual InstanceId GetInstance(Entity e) const = 0;

    /// return the owner map
    virtual Util::Array<Game::Entity> const& GetOwners() const = 0;

    /// sets owners of offset -> (newOwners.Size() + offset) to newOwners.
    virtual void SetOwners(uint offset, Util::Array<Game::Entity> const& newOwners) = 0;

	/// Subsequently calls functionbundle serialize.
	virtual void SerializeOwners(const Ptr<IO::BinaryWriter>& writer) const = 0;

	/// Subsequently calls functionbundle deserialize.
	virtual void DeserializeOwners(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances) = 0;

	/// Garbage collect
	virtual SizeT Optimize() = 0;

	/// Enable an event listener
	void EnableEvent(ComponentEvent eventId);

	/// Returns whether a component container is enabled.
	bool Enabled() const;

	struct FunctionBundle
	{
		/// Called upon activation of component instance.
		/// When loading an entity from file, this is called after
		/// all components have been loaded.
		void(*OnActivate)(InstanceId instance);

		/// Called upon deactivation of component instance
		void(*OnDeactivate)(InstanceId instance);

		/// called at beginning of frame
		void(*OnBeginFrame)();

		/// called before rendering happens
		void(*OnRender)();

		/// called at the end of a frame
		void(*OnEndFrame)();

		/// called when game debug visualization is on
		void(*OnRenderDebug)();

		/// called when the component has been loaded from file.
		/// this does not guarantee that all components have been loaded.
		void(*OnLoad)(InstanceId instance);

		/// called after an entity has been save to a file.
		void(*OnSave)(InstanceId instance);

		/// Serialize the components attributes (excluding owners)
		void(*Serialize)(const Ptr<IO::BinaryWriter>& writer);

		/// Deserialize the components attributes (excluding owners)
		void(*Deserialize)(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances);

		/// Destroy all instances
		void(*DestroyAll)();

		/// Called after an instance has been moved from one index to another.
		/// Used in very special cases when you rely on for example instance id relations
		/// within your components.
		void(*OnInstanceMoved)(InstanceId instance, InstanceId oldIndex);

		/// Callback for when entities has been loaded and you need to hook into the hierarchy update.
		void(*SetParents)(InstanceId start, InstanceId end, const Util::Array<Entity>& entities, const Util::Array<uint32_t>& parentIndices);
	} functions;

    /// Special case for components that cannot use the function pointer within function bundle (heap allocated, dynamic).
    /// Should only serialize/deserialize the component attributes, not owners
    virtual void InternalSerialize(const Ptr<IO::BinaryWriter>& writer);
    virtual void InternalDeserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances);

protected:
	friend class ComponentManager;

    /// Holds all events this component is subscribed to.
	Util::BitField<ComponentEvent::NumEvents> events;
	
	/// Holds all attributes that this components has available.
	/// This should be adjacent to the data/tuple the values are in.
	Util::FixedArray<Attr::AttrId> attributes;

	/// Determines whether the component manager will execute this components update methods.
	/// activation, load and such methods will still be called.
	bool enabled;

	/// name of component
	Util::StringAtom componentName;

	/// Identifier
	Util::FourCC fourcc;
};

} // namespace Game