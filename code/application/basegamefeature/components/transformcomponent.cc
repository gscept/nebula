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
		// TODO: Implement this entire function. Needs to recalculate relationships and transforms.
		// this->Parent(i) = this->GetInstance(entities[parentIndices[i]]);
		i++;
	}
}


//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::RegisterEntity(const Entity& entity)
{
	uint32_t instance = this->data.RegisterEntity(entity);
	
	// Set default values
	this->LocalTransform(instance) = Math::matrix44::identity();
	this->WorldTransform(instance) = Math::matrix44::identity();
	this->Parent(instance) = uint32_t(-1);
	this->FirstChild(instance) = uint32_t(-1);
	this->NextSibling(instance) = uint32_t(-1);
	this->PrevSibling(instance) = uint32_t(-1);
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
uint32_t
TransformComponent::NumRegistered() const
{
	return this->data.Size();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::Allocate(uint num)
{
	SizeT first = this->data.data.Size();
	this->data.data.Reserve(first + num);
	
	this->data.data.GetArray<OWNER>().SetSize(first + num);

	this->data.data.GetArray<WORLDTRANSFORM>().Fill(first, num, Math::matrix44::identity());
	this->data.data.GetArray<LOCALTRANSFORM>().Fill(first, num, Math::matrix44::identity());
	this->data.data.GetArray<PARENT>().Fill(first, num, uint(-1));
	this->data.data.GetArray<FIRSTCHILD>().Fill(first, num, uint(-1));
	this->data.data.GetArray<NEXTSIBLING>().Fill(first, num, uint(-1));
	this->data.data.GetArray<PREVIOUSSIBLING>().Fill(first, num, uint(-1));
	this->data.data.UpdateSize();
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
void
TransformComponent::SetOwner(const uint32_t & i, const Game::Entity & entity)
{
	this->data.SetOwner(i, entity);
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
	case OWNER:
		return Util::Variant(this->data.data.Get<OWNER>(instance).id);
	case LOCALTRANSFORM:
		return Util::Variant(this->data.data.Get<LOCALTRANSFORM>(instance));
	case WORLDTRANSFORM:
		return Util::Variant(this->data.data.Get<WORLDTRANSFORM>(instance));
	case PARENT:
		return Util::Variant(this->data.data.Get<PARENT>(instance));
	case FIRSTCHILD:
		return Util::Variant(this->data.data.Get<FIRSTCHILD>(instance));
	case NEXTSIBLING:
		return Util::Variant(this->data.data.Get<NEXTSIBLING>(instance));
	case PREVIOUSSIBLING:
		return Util::Variant(this->data.data.Get<PREVIOUSSIBLING>(instance));
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
TransformComponent::Serialize(const Ptr<IO::BinaryWriter>& writer) const
{
	return this->data.Serialize(writer);
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances)
{
	this->data.Deserialize(reader, offset, numInstances);
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