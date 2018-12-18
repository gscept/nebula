//------------------------------------------------------------------------------
//  transformcomponent.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "transformcomponent.h"
#include "basegamefeature/managers/componentmanager.h"
#include "basegamefeature/components/transformdata.h"

namespace Game
{

static TransformComponentAllocator data;
static Msg::UpdateTransform::MessageQueueId messageQueue;

//------------------------------------------------------------------------------
/**
	Default implementations
*/
__ImplementComponent_woSerialization(TransformComponent, data);

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::Create()
{
	data = TransformComponentAllocator();

	__SetupDefaultComponentBundle(data);
	data.functions.OnDeactivate = OnDeactivate;
	data.functions.OnInstanceMoved = OnInstanceMoved;
	data.functions.SetParents = SetParents;
	__RegisterComponent(&data, "TransformComponent"_atm);

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
	data.messageListeners.Append(__RegisterMsg(Msg::SetLocalTransform, SetLocalTransform));
	data.messageListeners.Append(__RegisterMsg(Msg::SetWorldTransform, SetWorldTransform));
	data.messageListeners.Append(__RegisterMsg(Msg::SetParent, SetParent));
	messageQueue = Msg::UpdateTransform::AllocateMessageQueue();
	
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetLocalTransform(uint32_t i, const Math::matrix44& val)
{
	data.LocalTransform(i) = val;
	
	uint32_t parent = data.Parent(i);
	uint32_t child = data.FirstChild(i);
	if (parent == InvalidIndex)
	{
		data.WorldTransform(i) = val;
		if (child == InvalidIndex)
		{
			// Early out if we don't have any children
			Msg::UpdateTransform::Send(data.GetOwner(i), val);
			return;
		}
		
		Msg::UpdateTransform::Defer(messageQueue, data.GetOwner(i), val);
	}
	else
	{
		// First of, transform this with parent transform if any
		data.WorldTransform(i) = Math::matrix44::multiply(data.LocalTransform(i), data.WorldTransform(parent));

		if (child == InvalidIndex)
		{
			// Early out if we don't have any children
			Msg::UpdateTransform::Send(data.GetOwner(i), val);
			return;
		}

		Msg::UpdateTransform::Defer(messageQueue, data.GetOwner(i), data.WorldTransform(i));
	}
	
	parent = i;
	// Transform every child and their siblings.
	while (parent != InvalidIndex)
	{
		while (child != InvalidIndex)
		{
			data.WorldTransform(child) = Math::matrix44::multiply(data.LocalTransform(child), data.WorldTransform(parent));
			Msg::UpdateTransform::Defer(messageQueue, data.GetOwner(child), data.WorldTransform(child));
			parent = child;
			child = data.FirstChild(child);
		}
		child = data.NextSibling(parent);
		parent = data.Parent(parent);
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
	uint32_t instance = data.GetInstance(entity);
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
	n_assert(data.data.Size() > instance);
	if (data.data.Size() <= instance)
	{
		return;
	}

	uint32_t parentInstance = data.Parent(instance);
	if (parentInstance != InvalidIndex)
	{
		Math::matrix44& parentWorld = data.WorldTransform(parentInstance);
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
	uint32_t instance = data.GetInstance(entity);
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
	if (instance < data.data.Size())
		return data.LocalTransform(instance);

	return Math::matrix44::identity();
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
TransformComponent::GetLocalTransform(Game::Entity entity)
{
	return GetLocalTransform(data.GetInstance(entity));
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
TransformComponent::GetWorldTransform(uint32_t instance)
{
	if (instance < data.data.Size())
		return data.WorldTransform(instance);

	return Math::matrix44::identity();
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
TransformComponent::GetWorldTransform(Game::Entity entity)
{
	return GetWorldTransform(data.GetInstance(entity));
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
		data.Parent(parentInstance) == instance
		)
	{
		return;
	}

	// Update all old nearest neighbor relationships
	{
		uint32_t prevParent = data.Parent(instance);
		uint32_t nextSibling = data.NextSibling(instance);
		uint32_t previousSibling = data.NextSibling(instance);

		if (prevParent != InvalidIndex && data.FirstChild(prevParent) == instance)
			data.FirstChild(prevParent) = nextSibling;

		if (nextSibling != InvalidIndex)
			data.PreviousSibling(nextSibling) = previousSibling;

		if (previousSibling != InvalidIndex)
			data.NextSibling(previousSibling) = nextSibling;
	}

	// Update all new nearest neighbor relationships
	data.Parent(instance) = parentInstance;

	if (parentInstance != InvalidIndex)
	{
		uint32_t child = data.FirstChild(parentInstance);
		if (child == InvalidIndex)
		{
			data.FirstChild(parentInstance) = instance;
		}
		else
		{
			// Find last child and make this a sibling to that instance
			uint32_t sibling = data.NextSibling(child);
			while (sibling != InvalidIndex)
			{
				child = sibling;
				sibling = data.NextSibling(child);
			}

			data.NextSibling(child) = instance;
			data.PreviousSibling(instance) = child;
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
	uint32_t instance = data.GetInstance(entity);
	if (instance == InvalidIndex)
		return;
	uint32_t parentInstance = data.GetInstance(parent);

	SetParent(instance, parentInstance);
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetParent(uint32_t instance, uint32_t parentInstance)
{
	n_assert(instance < data.NumRegistered() && instance != InvalidIndex);

	InternalSetParent(instance, parentInstance);
	UpdateHierarchy(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformComponent::GetParent(uint32_t instance)
{
	return data.Parent(instance);
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
TransformComponent::GetOwner(uint32_t instance)
{
	return data.GetOwner(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformComponent::GetFirstChild(uint32_t instance)
{
	return data.FirstChild(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformComponent::GetNextSibling(uint32_t instance)
{
	return data.NextSibling(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformComponent::GetPreviousSibling(uint32_t instance)
{
	return data.PreviousSibling(instance);
}

//------------------------------------------------------------------------------
/**
*/
Util::FourCC
TransformComponent::GetFourCC()
{
	return data.GetRtti()->GetFourCC();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::OnDeactivate(uint32_t instance)
{
	// update sibling relationships
	uint32_t previousSibling = data.PreviousSibling(instance);
	uint32_t nextSibling = data.NextSibling(instance);
	uint32_t child = data.FirstChild(instance);
	uint32_t parentInstance = data.Parent(instance);

	if(parentInstance != InvalidIndex && data.FirstChild(parentInstance) == instance)
		data.FirstChild(parentInstance) = child;

	if (previousSibling != InvalidIndex)
	{
		if (child != InvalidIndex)
		{
			data.PreviousSibling(child) = previousSibling;
			data.NextSibling(previousSibling) = child;
		}
		else
		{
			data.NextSibling(previousSibling) = nextSibling;
		}
	}

	// Each child needs to update their parent to this instance's parent
	uint32_t lastChild = InvalidIndex;
	while (child != InvalidIndex)
	{
		data.Parent(child) = parentInstance;
		UpdateHierarchy(child);
		lastChild = child;
		child = data.NextSibling(child);
	}

	if (nextSibling != InvalidIndex)
	{
		if (lastChild != InvalidIndex)
		{
			data.NextSibling(lastChild) = nextSibling;
			data.PreviousSibling(nextSibling) = lastChild;
		}
		else
		{
			data.PreviousSibling(nextSibling) = previousSibling;
		}
	}
}

//------------------------------------------------------------------------------
/**
	Update all instance relationships since the instance index has changed
*/
void
TransformComponent::OnInstanceMoved(uint32_t instance, uint32_t oldIndex)
{
	uint32_t parent = data.Parent(instance);
	uint32_t nextSibling = data.NextSibling(instance);
	uint32_t previousSibling = data.PreviousSibling(instance);
	uint32_t child = data.FirstChild(instance);
	
	if (parent != InvalidIndex && data.FirstChild(parent) == oldIndex)
	{
		data.FirstChild(parent) = instance;
	}

	if (nextSibling != InvalidIndex)
	{
		data.PreviousSibling(nextSibling) = instance;
	}

	if (previousSibling != InvalidIndex)
	{
		data.NextSibling(previousSibling) = instance;
	}

	while (child != InvalidIndex)
	{
		data.Parent(child) = instance;
		child = data.NextSibling(child);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::UpdateHierarchy(uint32_t instance)
{
	// TODO: There are more elegant ways for this.	
	SetLocalTransform(instance, data.LocalTransform(instance));
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::Serialize(const Ptr<IO::BinaryWriter>& writer)
{
	// Only serialize the ones we want.
	Game::Serialize(writer, data.data.GetArray<TransformComponentAllocator::LOCALTRANSFORM>());
	Game::Serialize(writer, data.data.GetArray<TransformComponentAllocator::WORLDTRANSFORM>());
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances)
{
	// Only serialize the ones we want.
	Game::Deserialize(reader, data.data.GetArray<TransformComponentAllocator::LOCALTRANSFORM>(), offset, numInstances);
	Game::Deserialize(reader, data.data.GetArray<TransformComponentAllocator::WORLDTRANSFORM>(), offset, numInstances);
}

} // namespace Game