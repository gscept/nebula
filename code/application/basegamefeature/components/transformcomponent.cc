//------------------------------------------------------------------------------
//  transformcomponent.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "transformcomponent.h"
#include "basegamefeature/managers/componentmanager.h"
#include "transformdata.h"

namespace Game
{

static TransformComponentData component;
static Msg::UpdateTransform::MessageQueueId messageQueue;

using AttrIndex = TransformComponentData::AttributeIndex;

__ImplementComponent(TransformComponent, component)

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::Create()
{
	component = TransformComponentData();

	component.functions.DestroyAll = DestroyAll;
	component.functions.Serialize = Serialize;
	component.functions.Deserialize = Deserialize;
	component.functions.SetParents = SetParents;
	__RegisterComponent(&component);

	SetupAcceptedMessages();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::Discard()
{

}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetupAcceptedMessages()
{
	// SetLocalTransform message will be handled by ::SetLocalTransform(...)
	component.messageListeners.Append(__RegisterMsg(Msg::SetLocalTransform, SetLocalTransform));
	component.messageListeners.Append(__RegisterMsg(Msg::SetParent, SetParent));
	messageQueue = Msg::UpdateTransform::AllocateMessageQueue();
	
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetLocalTransform(const uint32_t& i, const Math::matrix44& val)
{
	component.data.Get<AttrIndex::LOCALTRANSFORM>(i) = val;
	uint32_t parent = component.data.Get<AttrIndex::PARENT>(i);
	uint32_t child;
	Math::matrix44 transform;
	if (parent == InvalidIndex)
	{
		component.data.Get<AttrIndex::WORLDTRANSFORM>(i) = val;
		Msg::UpdateTransform::Defer(messageQueue, component.GetOwner(i), val);
	}
	else
	{
		// First of, transform this with parent transform if any
		transform = component.data.Get<AttrIndex::WORLDTRANSFORM>(parent);
		component.data.Get<AttrIndex::WORLDTRANSFORM>(i) = Math::matrix44::multiply(component.data.Get<AttrIndex::LOCALTRANSFORM>(i), transform);
		Msg::UpdateTransform::Defer(messageQueue, component.GetOwner(i), component.data.Get<AttrIndex::WORLDTRANSFORM>(i));
	}
	
	child = component.data.Get<AttrIndex::FIRSTCHILD>(i);
	parent = i;
	// Transform every child and their siblings.
	while (parent != InvalidIndex)
	{
		while (child != InvalidIndex)
		{
			transform = component.data.Get<AttrIndex::WORLDTRANSFORM>(parent);
			component.data.Get<AttrIndex::WORLDTRANSFORM>(child) = Math::matrix44::multiply(component.data.Get<AttrIndex::LOCALTRANSFORM>(child), transform);
			Msg::UpdateTransform::Defer(messageQueue, component.GetOwner(child), component.data.Get<AttrIndex::WORLDTRANSFORM>(child));
			parent = child;
			child = component.data.Get<AttrIndex::FIRSTCHILD>(child);
		}
		child = component.data.Get<AttrIndex::NEXTSIBLING>(parent);
		parent = component.data.Get<AttrIndex::PARENT>(parent);
	}

	// Dispatch all world transform update messages sequentially at the end of the method.
	// Keeps it cache friendly(er)
	Msg::UpdateTransform::DispatchMessageQueue(messageQueue);
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetLocalTransform(const Game::Entity& entity, const Math::matrix44& val)
{
	uint32_t instance = component.GetInstance(entity);
	if (instance != InvalidIndex)
	{
		SetLocalTransform(instance, val);
	}
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
TransformComponent::GetLocalTransform(const uint32_t & instance)
{
	if (instance < component.data.Size())
		return component.data.Get<AttrIndex::LOCALTRANSFORM>(instance);

	return Math::matrix44::identity();
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
TransformComponent::GetLocalTransform(const Game::Entity & entity)
{
	return GetLocalTransform(component.GetInstance(entity));
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
TransformComponent::GetWorldTransform(const uint32_t & instance)
{
	if (instance < component.data.Size())
		return component.data.Get<AttrIndex::WORLDTRANSFORM>(instance);

	return Math::matrix44::identity();
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
TransformComponent::GetWorldTransform(const Game::Entity & entity)
{
	return GetWorldTransform(component.GetInstance(entity));
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetParents(const uint32_t & start, const uint32_t & end, const Util::Array<Entity>& entities, const Util::Array<uint32_t>& parentIndices)
{
	SizeT i = 0;
	for (SizeT instance = start; instance < end; instance++)
	{
		SetParent(instance, GetInstance(entities[parentIndices[i]]));
		i++;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetParent(const Game::Entity& entity, const Game::Entity& parent)
{
	uint32_t instance = component.GetInstance(entity);
	uint32_t parentInstance = component.GetInstance(parent);
	SetParent(instance, parentInstance);
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetParent(const uint32_t& instance, const uint32_t& parentInstance)
{
	if (instance == InvalidIndex ||
		parentInstance == InvalidIndex ||
		instance == parentInstance ||
		component.data.Get<AttrIndex::PARENT>(parentInstance) == instance ||
		component.data.Get<AttrIndex::PARENT>(instance) == parentInstance
		)
	{
		return;
	}
	
	component.data.Get<AttrIndex::PARENT>(instance) = parentInstance;

	uint32_t child = component.data.Get<AttrIndex::FIRSTCHILD>(parentInstance);
	if (child == InvalidIndex)
	{
		component.data.Get<AttrIndex::FIRSTCHILD>(parentInstance) = instance;
	}
	else
	{
		// Find last child and make this a sibling to that instance
		uint32_t sibling = component.data.Get<AttrIndex::NEXTSIBLING>(child);
		while (sibling != InvalidIndex)
		{
			child = sibling;
			sibling = component.data.Get<AttrIndex::NEXTSIBLING>(child);
		}

		component.data.Get<AttrIndex::NEXTSIBLING>(child) = instance;
		component.data.Get<AttrIndex::PREVIOUSSIBLING>(instance) = child;
	}

	UpdateHierarchy(instance);
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::UpdateHierarchy(uint32_t instance)
{
	// TODO: There are more elegant ways for this.
	uint32_t parent = component.data.Get<AttrIndex::PARENT>(instance);
	if (parent != InvalidIndex)
		SetLocalTransform(parent, component.data.Get<AttrIndex::LOCALTRANSFORM>(parent));
}

} // namespace Game