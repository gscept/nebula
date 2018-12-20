#pragma once
//------------------------------------------------------------------------------
/**
	TransformComponent

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/component/component.h"
#include "basegamefeature/messages/basegameprotocol.h"

namespace Game
{

class TransformComponent
{
	__DeclareComponent(TransformComponent)
public:
	/// Setup callbacks.
	static void SetupAcceptedMessages();

	/// Set the local transform of instance. Will update hierarchy
	static void SetLocalTransform(uint32_t instance, const Math::matrix44& val);
	static void SetLocalTransform(Game::Entity entity, const Math::matrix44& val);
	
	/// Set the world transform of an instance. This will update the hierarchy and also set the local transform.
	static void SetWorldTransform(uint32_t instance, const Math::matrix44& val);
	static void SetWorldTransform(Game::Entity entity, const Math::matrix44& val);

	/// Return the local transform of an instance.
	static Math::matrix44 GetLocalTransform(uint32_t instance);
	static Math::matrix44 GetLocalTransform(Game::Entity entity);

	/// Return the world transform of an instance.
	static Math::matrix44 GetWorldTransform(uint32_t instance);
	static Math::matrix44 GetWorldTransform(Game::Entity entity);

	/// Update relationships
	static void SetParents(uint32_t start, uint32_t end, const Util::Array<Entity>& entities, const Util::Array<uint32_t>& parentIndices);

	/// Set parent of specific entity
	static void SetParent(Game::Entity entity, Game::Entity parent);
	static void SetParent(uint32_t instance, uint32_t parentInstance);

	/// Get parent of instance
	static uint32_t GetParent(uint32_t instance);
	
	/// Get owner of instance
	static Game::Entity GetOwner(uint32_t instance);

	/// Get first child of instance
	static uint32_t GetFirstChild(uint32_t instance);

	/// Get the first sibling of this instance
	static uint32_t GetNextSibling(uint32_t instance);

	/// Get the previous sibling of this instance
	static uint32_t GetPreviousSibling(uint32_t instance);

	/// Return this components fourcc
	static Util::FourCC GetFourCC();

private:
	/// Updates relationships
	static void OnDeactivate(uint32_t instance);

	/// Special case if an instance has changed index because of garbage collection
	static void OnInstanceMoved(uint32_t instance, uint32_t oldIndex);

	/// Update the transform hierarchy of this instance and all its children
	static void UpdateHierarchy(uint32_t instance);
};

} // namespace Game