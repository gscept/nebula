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
TransformComponent::TransformComponent()
{
	this->attributeIds.SetSize(7);
	this->attributeIds[0] = Attr::Owner;
	this->attributeIds[1] = Attr::LocalTransform;
	this->attributeIds[2] = Attr::WorldTransform;
	this->attributeIds[3] = Attr::Parent;
	this->attributeIds[4] = Attr::FirstChild;
	this->attributeIds[5] = Attr::NextSibling;
	this->attributeIds[6] = Attr::PreviousSibling;
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
		this->WorldTransform(i) = Math::matrix44::multiply(this->LocalTransform(i), transform);
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
			parent = child;
			child = this->FirstChild(child);
		}
		child = this->NextSibling(parent);
		parent = this->Parent(parent);
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
		this->Parent(i) = this->GetInstance(entities[parentIndices[i]]);
		// TODO: Implement this entire function. Needs to recalculate relationships and transforms.
		i++;
	}
}


//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::RegisterEntity(const Entity& entity)
{
	this->data.RegisterEntity(entity);
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::DeregisterEntity(const Entity& entity)
{
	uint32_t index = this->data.GetInstance(entity);
	if (index != InvalidIndex)
	{
		this->data.DeregisterEntity(entity);
		return;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::DeregisterAll()
{
	this->data.DeregisterAll();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::DeregisterAllDead()
{
	this->data.DeregisterAllInactive();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::CleanData()
{
	this->data.Clean();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::DestroyAll()
{
	this->data.DestroyAll();
}

//------------------------------------------------------------------------------
/**
*/
bool
TransformComponent::IsRegistered(const Entity& entity) const
{
	return this->data.GetInstance(entity) != InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformComponent::GetInstance(const Entity& entity) const
{
	return this->data.GetInstance(entity);
}

//------------------------------------------------------------------------------
/**
*/
Entity
TransformComponent::GetOwner(const uint32_t& instance) const
{
	return this->data.GetOwner(instance);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
TransformComponent::Optimize()
{
	// TODO: We need to update relationships.
	return this->data.Optimize();
}

//------------------------------------------------------------------------------
/**
*/
Util::Variant
TransformComponent::GetAttributeValue(uint32_t instance, IndexT attributeIndex) const
{
	switch (attributeIndex)
	{
	case 0:
		return Util::Variant(this->data.data.Get<0>(instance).id);
	case 1:
		return Util::Variant(this->data.data.Get<1>(instance));
	case 2:
		return Util::Variant(this->data.data.Get<2>(instance));
	case 3:
		return Util::Variant(this->data.data.Get<3>(instance));
	case 4:
		return Util::Variant(this->data.data.Get<4>(instance));
	case 5:
		return Util::Variant(this->data.data.Get<5>(instance));
	case 6:
		return Util::Variant(this->data.data.Get<6>(instance));
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
		return Util::Variant(this->data.data.Get<0>(instance).id);
	}
	else if (attributeId == Attr::LocalTransform)
	{
		return Util::Variant(this->data.data.Get<1>(instance));
	}
	else if (attributeId == Attr::WorldTransform)
	{
		return Util::Variant(this->data.data.Get<2>(instance));
	}
	else if (attributeId == Attr::Parent)
	{
		return Util::Variant(this->data.data.Get<3>(instance));
	}
	else if (attributeId == Attr::FirstChild)
	{
		return Util::Variant(this->data.data.Get<4>(instance));
	}
	else if (attributeId == Attr::NextSibling)
	{
		return Util::Variant(this->data.data.Get<5>(instance));
	}
	else if (attributeId == Attr::PreviousSibling)
	{
		return Util::Variant(this->data.data.Get<6>(instance));
	}

	n_assert2(false, "Component doesn't contain this attribute!\n");
	return Util::Variant();
}


//------------------------------------------------------------------------------
/**
*/
Math::matrix44&
TransformComponent::LocalTransform(const uint32_t& instance)
{
	return this->data.data.Get<1>(instance);
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44&
TransformComponent::WorldTransform(const uint32_t& instance)
{
	return this->data.data.Get<2>(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t&
TransformComponent::Parent(const uint32_t& instance)
{
	return this->data.data.Get<3>(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t&
TransformComponent::FirstChild(const uint32_t& instance)
{
	return this->data.data.Get<4>(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t&
TransformComponent::NextSibling(const uint32_t& instance)
{
	return this->data.data.Get<5>(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t&
TransformComponent::PrevSibling(const uint32_t& instance)
{
	return this->data.data.Get<6>(instance);
}

} // namespace Game