//------------------------------------------------------------------------------
//  transformcomponent.cc
//  (C) 2018-2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "game/component/component.h"
#include "transformcomponent.h"
#include "basegamefeature/managers/componentmanager.h"

namespace Game
{

static TransformComponentAllocator* data;

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
	if (data != nullptr)
	{
		data->DestroyAll();
	}
	else
	{
        data = n_new(TransformComponentAllocator);
	}

	data->EnableEvent(Game::ComponentEvent::OnDeactivate);
	
	__SetupDefaultComponentBundle(data);
	data->functions.OnDeactivate = OnDeactivate;
	data->functions.OnInstanceMoved = OnInstanceMoved;
	data->functions.SetParents = SetParents;
	Game::ComponentManager::Instance()->RegisterComponent(data, "TransformComponent"_atm, GetFourCC());

	SetupAcceptedMessages();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::Discard()
{
	data->DestroyAll();
	// __DeregisterComponent(data);
	delete data;
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetupAcceptedMessages()
{
	// SetLocalTransform message will be handled by ::SetLocalTransform(...)
	__RegisterMsg(Msg::SetLocalTransform, SetLocalTransform);
	__RegisterMsg(Msg::SetWorldTransform, SetWorldTransform);
	__RegisterMsg(Msg::SetParent, SetParent);
	messageQueue = Msg::UpdateTransform::AllocateMessageQueue();
	
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetLocalTransform(InstanceId i, const Math::matrix44& val)
{
	data->Get<Attr::LocalTransform>(i) = val;
	
	InstanceId parent = data->Get<Attr::Parent>(i);
	InstanceId child = data->Get<Attr::FirstChild>(i);
	if (parent == InvalidIndex)
	{
		if (child == InvalidIndex)
		{
			// Early out if we don't have any children
			Msg::UpdateTransform::Send(data->GetOwner(i), val);

			// This means world transform will remain untouched
			// for entities without parents and children
			return;
		}

		data->Get<Attr::WorldTransform>(i) = val;

		Msg::UpdateTransform::Defer(messageQueue, data->GetOwner(i), val);
	}
	else
	{
		// First of, transform this with parent transform if any
		data->Get<Attr::WorldTransform>(i) = Math::matrix44::multiply(val, data->Get<Attr::WorldTransform>(parent));

		if (child == InvalidIndex)
		{
			// Early out if we don't have any children
			Msg::UpdateTransform::Send(data->GetOwner(i), val);
			return;
		}

		Msg::UpdateTransform::Defer(messageQueue, data->GetOwner(i), data->Get<Attr::WorldTransform>(i));
	}
	
	parent = i;
	// Transform every child and their siblings.
	while (parent != InvalidIndex)
	{
		while (child != InvalidIndex)
		{
			data->Get<Attr::WorldTransform>(child) = Math::matrix44::multiply(data->Get<Attr::LocalTransform>(child), data->Get<Attr::WorldTransform>(parent));
			Msg::UpdateTransform::Defer(messageQueue, data->GetOwner(child), data->Get<Attr::WorldTransform>(child));
			parent = child;
			child = data->Get<Attr::FirstChild>(child);
		}
		child = data->Get<Attr::NextSibling>(parent);
		parent = data->Get<Attr::Parent>(parent);
	}
	// Dispatch all world transform update messages sequentially at the end of the method.
	// Keeps it cache friendly(er)
	/**
		@todo	Is this really faster? this requires us to copy all
				the transforms temporarily. Might be faster to just
				ignore the cache misses we'll likely hit when updating
				the hierarchy.
				(try replacing all the defer calls to regular sends).
	*/
	Msg::UpdateTransform::DispatchMessageQueue(messageQueue);
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetLocalTransform(Game::Entity entity, const Math::matrix44& val)
{
	InstanceId instance = data->GetInstance(entity);
	if (instance != InvalidIndex)
	{
		SetLocalTransform(instance, val);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetWorldTransform(InstanceId instance, const Math::matrix44& val)
{
	n_assert(data->data.Size() > instance);
	if (data->data.Size() <= instance)
	{
		return;
	}

	InstanceId parentInstance = data->Get<Attr::Parent>(instance);
	if (parentInstance != InvalidIndex)
	{
		Math::matrix44& parentWorld = data->Get<Attr::WorldTransform>(parentInstance);
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
	InstanceId instance = data->GetInstance(entity);
	if (instance != InvalidIndex)
	{
		SetWorldTransform(instance, val);
	}
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
TransformComponent::GetLocalTransform(InstanceId instance)
{
	if (instance < data->data.Size())
		return data->Get<Attr::LocalTransform>(instance);

	return Math::matrix44::identity();
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
TransformComponent::GetLocalTransform(Game::Entity entity)
{
	return GetLocalTransform(data->GetInstance(entity));
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
TransformComponent::GetWorldTransform(InstanceId instance)
{
	if (instance < data->data.Size())
	{
		// If we don't have any parent, we use the local transform.
		if (data->Get<Attr::Parent>(instance) == InvalidIndex)
		{
			return data->Get<Attr::LocalTransform>(instance);
		}

		return data->Get<Attr::WorldTransform>(instance);
	}

	return Math::matrix44::identity();
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
TransformComponent::GetWorldTransform(Game::Entity entity)
{
	return GetWorldTransform(data->GetInstance(entity));
}


//------------------------------------------------------------------------------
/**
*/
void
InternalSetParent(InstanceId instance, InstanceId parentInstance)
{
	if (instance == InvalidIndex ||
		instance == parentInstance)
	{
		return;
	}

	if (parentInstance != InvalidIndex &&
		data->Get<Attr::Parent>(parentInstance) == instance)
	{
		return;
	}

	// Update all old nearest neighbor relationships
	{
		InstanceId prevParent = data->Get<Attr::Parent>(instance);
		InstanceId nextSibling = data->Get<Attr::NextSibling>(instance);
		InstanceId previousSibling = data->Get<Attr::PreviousSibling>(instance);

		if (prevParent != InvalidIndex && data->Get<Attr::FirstChild>(prevParent) == instance)
			data->Get<Attr::FirstChild>(prevParent) = nextSibling;

		if (nextSibling != InvalidIndex)
			data->Get<Attr::PreviousSibling>(nextSibling) = previousSibling;

		if (previousSibling != InvalidIndex)
			data->Get<Attr::NextSibling>(previousSibling) = nextSibling;
	}

	// Update all new nearest neighbor relationships
	data->Get<Attr::Parent>(instance) = parentInstance;

	if (parentInstance != InvalidIndex)
	{
		InstanceId child = data->Get<Attr::FirstChild>(parentInstance);
		if (child == InvalidIndex)
		{
			data->Get<Attr::FirstChild>(parentInstance) = instance;
		}
		else
		{
			// Find last child and make this a sibling to that instance
			InstanceId sibling = data->Get<Attr::NextSibling>(child);
			while (sibling != InvalidIndex)
			{
				child = sibling;
				sibling = data->Get<Attr::NextSibling>(child);
			}

			data->Get<Attr::NextSibling>(child) = instance;
			data->Get<Attr::PreviousSibling>(instance) = child;
		}
	}
}


//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetParents(InstanceId start, InstanceId end, const Util::Array<Entity>& entities, const Util::Array<uint32_t>& parentIndices)
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
	InstanceId instance = data->GetInstance(entity);
	if (instance == InvalidIndex)
		return;
	InstanceId parentInstance = data->GetInstance(parent);

	SetParent(instance, parentInstance);
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetParent(InstanceId instance, InstanceId parentInstance)
{
	n_assert(instance < data->NumRegistered() && instance != InvalidIndex);

	InternalSetParent(instance, parentInstance);
	UpdateHierarchy(instance);
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
TransformComponent::GetParent(InstanceId instance)
{
	return data->Get<Attr::Parent>(instance);
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
TransformComponent::GetOwner(InstanceId instance)
{
	return data->GetOwner(instance);
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
TransformComponent::GetFirstChild(InstanceId instance)
{
	return data->Get<Attr::FirstChild>(instance);
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
TransformComponent::GetNextSibling(InstanceId instance)
{
	return data->Get<Attr::NextSibling>(instance);
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
TransformComponent::GetPreviousSibling(InstanceId instance)
{
	return data->Get<Attr::PreviousSibling>(instance);
}

//------------------------------------------------------------------------------
/**
*/
Util::FourCC
TransformComponent::GetFourCC()
{
	return 'trsf';
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::OnDeactivate(InstanceId instance)
{
	// update sibling relationships
	InstanceId previousSibling = data->Get<Attr::PreviousSibling>(instance);
	InstanceId nextSibling = data->Get<Attr::NextSibling>(instance);
	InstanceId child = data->Get<Attr::FirstChild>(instance);
	InstanceId parentInstance = data->Get<Attr::Parent>(instance);

	if(parentInstance != InvalidIndex && data->Get<Attr::FirstChild>(parentInstance) == instance)
		data->Get<Attr::FirstChild>(parentInstance) = child;

	if (previousSibling != InvalidIndex)
	{
		if (child != InvalidIndex)
		{
			data->Get<Attr::PreviousSibling>(child) = previousSibling;
			data->Get<Attr::NextSibling>(previousSibling) = child;
		}
		else
		{
			data->Get<Attr::NextSibling>(previousSibling) = nextSibling;
		}
	}

	// Each child needs to update their parent to this instance's parent
	InstanceId lastChild = InvalidIndex;
	while (child != InvalidIndex)
	{
		data->Get<Attr::Parent>(child) = parentInstance;
		UpdateHierarchy(child);
		lastChild = child;
		child = data->Get<Attr::NextSibling>(child);
	}

	if (nextSibling != InvalidIndex)
	{
		if (lastChild != InvalidIndex)
		{
			data->Get<Attr::NextSibling>(lastChild) = nextSibling;
			data->Get<Attr::PreviousSibling>(nextSibling) = lastChild;
		}
		else
		{
			data->Get<Attr::PreviousSibling>(nextSibling) = previousSibling;
		}
	}
}

//------------------------------------------------------------------------------
/**
	Update all instance relationships since the instance index has changed
*/
void
TransformComponent::OnInstanceMoved(InstanceId instance, InstanceId oldIndex)
{
	InstanceId parent = data->Get<Attr::Parent>(instance);
	InstanceId nextSibling = data->Get<Attr::NextSibling>(instance);
	InstanceId previousSibling = data->Get<Attr::PreviousSibling>(instance);
	InstanceId child = data->Get<Attr::FirstChild>(instance);
	
	if (parent != InvalidIndex && data->Get<Attr::FirstChild>(parent) == oldIndex)
	{
		data->Get<Attr::FirstChild>(parent) = instance;
	}

	if (nextSibling != InvalidIndex)
	{
		data->Get<Attr::PreviousSibling>(nextSibling) = instance;
	}

	if (previousSibling != InvalidIndex)
	{
		data->Get<Attr::NextSibling>(previousSibling) = instance;
	}

	while (child != InvalidIndex)
	{
		data->Get<Attr::Parent>(child) = instance;
		child = data->Get<Attr::NextSibling>(child);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::UpdateHierarchy(InstanceId instance)
{
	// TODO: There are more elegant ways for this.	
	SetLocalTransform(instance, data->Get<Attr::LocalTransform>(instance));
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::Serialize(const Ptr<IO::BinaryWriter>& writer)
{
	// Only serialize the ones we want.
	Game::Serialize(writer, data->data.GetArray<ComponentData::GetAttributeIndex<Attr::LocalTransform>()>());
	Game::Serialize(writer, data->data.GetArray<ComponentData::GetAttributeIndex<Attr::WorldTransform>()>());
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances)
{
	// Only serialize the ones we want.
	Game::Deserialize(reader, data->data.GetArray<ComponentData::GetAttributeIndex<Attr::LocalTransform>()>(), offset, numInstances);
	Game::Deserialize(reader, data->data.GetArray<ComponentData::GetAttributeIndex<Attr::WorldTransform>()>(), offset, numInstances);
}

} // namespace Game