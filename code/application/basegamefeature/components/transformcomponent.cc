//------------------------------------------------------------------------------
//  transformcomponent.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "transformcomponent.h"

namespace Attr
{
	DefineMatrix44WithDefault(LocalTransform, 'TFLT', Attr::ReadWrite, Math::matrix44::identity())
	DefineMatrix44WithDefault(WorldTransform, 'TRWT', Attr::ReadWrite, Math::matrix44::identity());
	DefineUIntWithDefault(Parent, 'TRPT', Attr::ReadOnly, uint(-1));
	DefineUIntWithDefault(FirstChild, 'TRFC', Attr::ReadOnly, uint(-1));
	DefineUIntWithDefault(NextSibling, 'TRNS', Attr::ReadOnly, uint(-1));
	DefineUIntWithDefault(PreviousSibling, 'TRPS', Attr::ReadOnly, uint(-1));
} // namespace Attr

__ImplementClass(Game::TransformComponent, 'TRCM', Game::BaseComponent);

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
TransformComponent::TransformComponent() :
	component_templated_t({
		Attr::LocalTransform,
		Attr::WorldTransform,
		Attr::Parent,
		Attr::FirstChild,
		Attr::NextSibling,
		Attr::PreviousSibling
	})
{
	
}

//------------------------------------------------------------------------------
/**
*/
TransformComponent::~TransformComponent()
{
	// Empty
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetupAcceptedMessages()
{
	// SetLocalTransform message will be handled by this->SetLocalTransform(...)
	__RegisterMsg(Msg::SetLocalTransform, SetLocalTransform);
	__RegisterMsg(Msg::SetParent, SetParent);
	messageQueue = Msg::UpdateTransform::AllocateMessageQueue();
	
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetLocalTransform(const uint32_t& i, const Math::matrix44& val)
{
	this->LocalTransform(i) = val;
	uint32_t parent = this->Parent(i);
	uint32_t child;
	Math::matrix44 transform;
	// First of, transform this with parent transform if any
	if (parent != InvalidIndex)
	{
		transform = this->WorldTransform(parent);
		auto local = this->LocalTransform(i);
		this->WorldTransform(i) = Math::matrix44::multiply(this->LocalTransform(i), transform);
		Msg::UpdateTransform::Defer(messageQueue, this->GetOwner(i), this->WorldTransform(i));
	}
	child = this->FirstChild(i);
	parent = i;
	// Transform every child and their siblings.
	while (parent != InvalidIndex)
	{
		while (child != InvalidIndex)
		{
			transform = this->WorldTransform(parent);
			this->WorldTransform(child) = Math::matrix44::multiply(this->LocalTransform(parent), transform);
			Msg::UpdateTransform::Defer(messageQueue, this->GetOwner(child), this->WorldTransform(child));
			parent = child;
			child = this->FirstChild(child);
		}
		child = this->NextSibling(parent);
		parent = this->Parent(parent);
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
	uint32_t instance = this->GetInstance(entity);
	if (instance != InvalidIndex)
	{
		this->SetLocalTransform(instance, val);
	}
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
		// TODO: Implement this entire function. Needs to recalculate relationships and transforms.
		// this->Parent(i) = this->GetInstance(entities[parentIndices[i]]);
		i++;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetParent(const Game::Entity& entity, const Game::Entity& parent)
{
	uint32_t instance = this->GetInstance(entity);
	this->Parent(instance) = this->GetInstance(parent);

	this->UpdateHierarchy(instance);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
TransformComponent::Optimize()
{
	// TODO: We need to update relationships.
	SizeT numDeleted = component_templated_t::Optimize();
	return numDeleted;
}

//------------------------------------------------------------------------------
/**
*/
Util::Variant
TransformComponent::GetAttributeValue(uint32_t instance, IndexT attributeIndex) const
{
	switch (attributeIndex)
	{
	case OWNER:
		return Util::Variant(this->data.Get<OWNER>(instance).id);
	case LOCALTRANSFORM:
		return Util::Variant(this->data.Get<LOCALTRANSFORM>(instance));
	case WORLDTRANSFORM:
		return Util::Variant(this->data.Get<WORLDTRANSFORM>(instance));
	case PARENT:
		return Util::Variant(this->data.Get<PARENT>(instance));
	case FIRSTCHILD:
		return Util::Variant(this->data.Get<FIRSTCHILD>(instance));
	case NEXTSIBLING:
		return Util::Variant(this->data.Get<NEXTSIBLING>(instance));
	case PREVIOUSSIBLING:
		return Util::Variant(this->data.Get<PREVIOUSSIBLING>(instance));
	default:
		n_assert2(false, "Component doesn't contain this attribute!\n");
		return Util::Variant();
	}
}

//------------------------------------------------------------------------------
/**
*/
Util::Variant
TransformComponent::GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const
{
	if (attributeId == Attr::Owner)
	{
		return Util::Variant(this->data.Get<OWNER>(instance).id);
	}
	else if (attributeId == Attr::LocalTransform)
	{
		return Util::Variant(this->data.Get<LOCALTRANSFORM>(instance));
	}
	else if (attributeId == Attr::WorldTransform)
	{
		return Util::Variant(this->data.Get<WORLDTRANSFORM>(instance));
	}
	else if (attributeId == Attr::Parent)
	{
		return Util::Variant(this->data.Get<PARENT>(instance));
	}
	else if (attributeId == Attr::FirstChild)
	{
		return Util::Variant(this->data.Get<FIRSTCHILD>(instance));
	}
	else if (attributeId == Attr::NextSibling)
	{
		return Util::Variant(this->data.Get<NEXTSIBLING>(instance));
	}
	else if (attributeId == Attr::PreviousSibling)
	{
		return Util::Variant(this->data.Get<PREVIOUSSIBLING>(instance));
	}

	n_assert2(false, "Component doesn't contain this attribute!\n");
	return Util::Variant();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetAttributeValue(uint32_t instance, IndexT index, Util::Variant value)
{
	switch (index)
	{
	case LOCALTRANSFORM:
		this->SetLocalTransform(instance, value.GetMatrix44());
		break;
	}
}


//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetAttributeValue(uint32_t instance, Attr::AttrId attributeId, Util::Variant value)
{
	if (attributeId == Attr::LocalTransform)
		this->SetLocalTransform(instance, value.GetMatrix44());
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::UpdateHierarchy(uint32_t instance)
{
	// TODO: There are more elegant ways for this.
	uint32_t parent = this->Parent(instance);
	if (parent != InvalidIndex)
		this->SetLocalTransform(parent, this->LocalTransform(parent));
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44&
TransformComponent::LocalTransform(const uint32_t& instance)
{
	return this->data.Get<LOCALTRANSFORM>(instance);
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44&
TransformComponent::WorldTransform(const uint32_t& instance)
{
	return this->data.Get<WORLDTRANSFORM>(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t&
TransformComponent::Parent(const uint32_t& instance)
{
	return this->data.Get<PARENT>(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t&
TransformComponent::FirstChild(const uint32_t& instance)
{
	return this->data.Get<FIRSTCHILD>(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t&
TransformComponent::NextSibling(const uint32_t& instance)
{
	return this->data.Get<NEXTSIBLING>(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t&
TransformComponent::PrevSibling(const uint32_t& instance)
{
	return this->data.Get<PREVIOUSSIBLING>(instance);
}

} // namespace Game