#pragma once
//------------------------------------------------------------------------------
/**
	TransformComponent

	Manages transforms and parent - child relationships for entities.

	Each instance contains:
		* Local transform, with respect to the entity's
		  parent's world transform.
		* World transform.
		* Parent, first child and nearest sibling instances

	Internally, the system might decide to use local transform as the entity's
	world transform if it has no parent or children. This is however never
	reflected outward since if you ask for the world transform it will return
	the correct one that is used internally no matter what.

	(C) 2018-2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/component/component.h"
#include "basegamefeature/messages/basegameprotocol.h"
#include "basegamefeature/components/transformdata.h"

namespace Game
{

class TransformComponent
{
	__DeclareComponent(TransformComponent)
public:
	/// Setup callbacks.
	static void SetupAcceptedMessages();

	/// Set the local transform of instance. Will update hierarchy.
	static void SetLocalTransform(InstanceId instance, const Math::matrix44& val);
	static void SetLocalTransform(Game::Entity entity, const Math::matrix44& val);
	
	/// Set the world transform of an instance. This will update the hierarchy and also set the local transform.
	/// Note: this is MUCH slower than updating the local transform since we need to calculate the local transform
	///		  from the inverse of the entity's parent.
	static void SetWorldTransform(InstanceId instance, const Math::matrix44& val);
	static void SetWorldTransform(Game::Entity entity, const Math::matrix44& val);

	/// Return the local transform of an instance.
	static Math::matrix44 GetLocalTransform(InstanceId instance);
	static Math::matrix44 GetLocalTransform(Game::Entity entity);

	/// Return the world transform of an instance.
	static Math::matrix44 GetWorldTransform(InstanceId instance);
	static Math::matrix44 GetWorldTransform(Game::Entity entity);

	/// Update relationships
	static void SetParents(InstanceId start, InstanceId end, const Util::Array<Entity>& entities, const Util::Array<uint32_t>& parentIndices);

	/// Set parent of specific entity
	static void SetParent(Game::Entity entity, Game::Entity parent);
	static void SetParent(InstanceId instance, InstanceId parentInstance);

	/// Get parent of instance
	static InstanceId GetParent(InstanceId instance);
	
	/// Get owner of instance
	static Game::Entity GetOwner(InstanceId instance);

	/// Get first child of instance
	static InstanceId GetFirstChild(InstanceId instance);

	/// Get the first sibling of this instance
	static InstanceId GetNextSibling(InstanceId instance);

	/// Get the previous sibling of this instance
	static InstanceId GetPreviousSibling(InstanceId instance);

	/// Return this components fourcc
	static Util::FourCC GetFourCC();

private:
	/// Updates relationships
	static void OnDeactivate(InstanceId instance);

	/// Special case if an instance has changed index because of garbage collection
	static void OnInstanceMoved(InstanceId instance, InstanceId oldIndex);

	/// Update the transform hierarchy of this instance and all its children
	static void UpdateHierarchy(InstanceId instance);
};

} // namespace Game