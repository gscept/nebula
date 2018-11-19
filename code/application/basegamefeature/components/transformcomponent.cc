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


//------------------------------------------------------------------------------
/**
	Default implementations
*/
uint32_t TransformComponent::RegisterEntity(Game::Entity entity) { return component.RegisterEntity(entity); }
void TransformComponent::DeregisterEntity(Game::Entity entity) { component.DeregisterEntity(entity); }
void TransformComponent::DestroyAll() { component.DestroyAll(); }
SizeT TransformComponent::NumRegistered() { return component.NumRegistered(); }
uint32_t TransformComponent::GetInstance(Game::Entity entity) { return component.GetInstance(entity); }

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
	component.messageListeners.Append(__RegisterMsg(Msg::SetWorldTransform, SetWorldTransform));
	component.messageListeners.Append(__RegisterMsg(Msg::SetParent, SetParent));
	messageQueue = Msg::UpdateTransform::AllocateMessageQueue();
	
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetLocalTransform(uint32_t i, const Math::matrix44& val)
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
TransformComponent::SetLocalTransform(Game::Entity entity, const Math::matrix44& val)
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
void
TransformComponent::SetWorldTransform(uint32_t instance, const Math::matrix44& val)
{
	n_assert(component.data.Size() > instance);
	if (component.data.Size() <= instance)
	{
		return;
	}

	uint32_t parentInstance = component.data.Get<AttrIndex::PARENT>(instance);
	if (parentInstance != InvalidIndex)
	{
		Math::matrix44& parentWorld = component.data.Get<AttrIndex::WORLDTRANSFORM>(parentInstance);
		Math::matrix44 parentInverse = Math::matrix44::inverse(parentWorld);
		Math::matrix44 local = Math::matrix44::multiply(val, parentInverse);
		SetLocalTransform(instance, local);
	}
	else
	{
		// world transform is same as local if we have no parent
		SetLocalTransform(instance, val);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetWorldTransform(Game::Entity entity, const Math::matrix44& val)
{
	uint32_t instance = component.GetInstance(entity);
	if (instance != InvalidIndex)
	{
		SetWorldTransform(instance, val);
	}
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
TransformComponent::GetLocalTransform(uint32_t instance)
{
	if (instance < component.data.Size())
		return component.data.Get<AttrIndex::LOCALTRANSFORM>(instance);

	return Math::matrix44::identity();
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
TransformComponent::GetLocalTransform(Game::Entity entity)
{
	return GetLocalTransform(component.GetInstance(entity));
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
TransformComponent::GetWorldTransform(uint32_t instance)
{
	if (instance < component.data.Size())
		return component.data.Get<AttrIndex::WORLDTRANSFORM>(instance);

	return Math::matrix44::identity();
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
TransformComponent::GetWorldTransform(Game::Entity entity)
{
	return GetWorldTransform(component.GetInstance(entity));
}


//------------------------------------------------------------------------------
/**
*/
void
InternalSetParent(uint32_t instance, uint32_t parentInstance)
{
	if (instance == InvalidIndex ||
		instance == parentInstance
		)
	{
		return;
	}

	if (parentInstance != InvalidIndex &&
		component.data.Get<AttrIndex::PARENT>(parentInstance) == instance
		)
	{
		return;
	}

	component.data.Get<AttrIndex::PARENT>(instance) = parentInstance;

	if (parentInstance != InvalidIndex)
	{
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
	}
}


//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetParents(uint32_t start, uint32_t end, const Util::Array<Entity>& entities, const Util::Array<uint32_t>& parentIndices)
{
	SizeT i = 0;
	for (SizeT instance = start; instance < end; instance++)
	{
		if (parentIndices[i] == InvalidIndex)
		{
			InternalSetParent(instance, -1); // Instance has no parent :'(
		}
		else
		{
			auto e = entities[parentIndices[i]];
			auto p = GetInstance(entities[parentIndices[i]]);
			InternalSetParent(instance, GetInstance(entities[parentIndices[i]]));
		}
		i++;
	}

	// Update transforms after updating all parent, child and sibling indices.
	for (SizeT instance = start; instance < end; instance++)
	{
		// Only need to update from root
		if (GetParent(instance) == InvalidIndex)
		{
			UpdateHierarchy(instance);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetParent(Game::Entity entity, Game::Entity parent)
{
	uint32_t instance = component.GetInstance(entity);
	uint32_t parentInstance = component.GetInstance(parent);
	SetParent(instance, parentInstance);
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetParent(uint32_t instance, uint32_t parentInstance)
{
	InternalSetParent(instance, parentInstance);
	UpdateHierarchy(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformComponent::GetParent(uint32_t instance)
{
	return component.data.Get<AttrIndex::PARENT>(instance);
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
TransformComponent::GetOwner(uint32_t instance)
{
	return component.GetOwner(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformComponent::GetFirstChild(uint32_t instance)
{
	return component.data.Get<AttrIndex::FIRSTCHILD>(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformComponent::GetNextSibling(uint32_t instance)
{
	return component.data.Get<AttrIndex::NEXTSIBLING>(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformComponent::GetPreviousSibling(uint32_t instance)
{
	return component.data.Get<AttrIndex::PREVIOUSSIBLING>(instance);
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::UpdateHierarchy(uint32_t instance)
{
	// TODO: There are more elegant ways for this.	
	SetLocalTransform(instance, component.data.Get<AttrIndex::LOCALTRANSFORM>(instance));
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::Serialize(const Ptr<IO::BinaryWriter>& writer)
{
	// Only serialize the ones we want.
	Game::Serialize(writer, component.data.GetArray<AttrIndex::LOCALTRANSFORM>());
	Game::Serialize(writer, component.data.GetArray<AttrIndex::WORLDTRANSFORM>());
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances)
{
	// Only serialize the ones we want.
	Game::Deserialize(reader, component.data.GetArray<AttrIndex::LOCALTRANSFORM>(), offset, numInstances);
	Game::Deserialize(reader, component.data.GetArray<AttrIndex::WORLDTRANSFORM>(), offset, numInstances);
}

} // namespace Game