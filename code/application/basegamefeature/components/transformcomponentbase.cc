//------------------------------------------------------------------------------
//  component.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "transformcomponentbase.h"

__ImplementClass(Game::TransformComponentBase, 'TrCo', Game::BaseComponent)

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
TransformComponentBase::TransformComponentBase()
{
	this->events.SetBit(ComponentEvent::OnBeginFrame);

	this->attributes.SetSize(7);
	this->attributes[0] = Attr::Owner;
	this->attributes[1] = Attr::LocalTransform;
	this->attributes[2] = Attr::WorldTransform;
	this->attributes[3] = Attr::Parent;
	this->attributes[4] = Attr::FirstChild;
	this->attributes[5] = Attr::NextSibling;
	this->attributes[6] = Attr::PreviousSibling;
}

//------------------------------------------------------------------------------
/**
*/
TransformComponentBase::~TransformComponentBase()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponentBase::RegisterEntity(const Entity& entity)
{
	/*n_assert2(!this->idMap.Contains(entity.id), "ID has already been registered.");
	uint32_t index = this->data.AllocObject();
	this->data.Get<0>(index) = entity;
	this->idMap.Add(entity.id, index);*/
	this->data.RegisterEntity(entity);
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponentBase::UnregisterEntity(const Entity& entity)
{
	/*n_assert2(this->idMap.Contains(entity.id), "Tried to remove an ID that had not been registered.");
	SizeT index = this->idMap[entity.id];
	this->data.DeallocObject(index);
	this->idMap.Erase(entity.id);*/
	this->data.DeregisterEntity(entity);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformComponentBase::GetInstance(const Entity& entity) const
{
	//return this->idMap[entity.id];
	return this->data.GetInstance(entity);
}

//------------------------------------------------------------------------------
/**
*/
Entity
TransformComponentBase::GetOwner(const uint32_t& instance) const
{
	//return this->data.Get<0>(instance);
	return this->data.data[instance].owner;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
TransformComponentBase::Optimize()
{
	return this->data.Optimize();
}

//------------------------------------------------------------------------------
/**
	@todo	Implement
*/
Util::Variant
TransformComponentBase::GetAttributeValue(uint32_t instance, IndexT attributeIndex) const
{
	switch (attributeIndex)
	{
	case 0:
		// return Util::Variant(this->data.data[instance].owner);
		n_error("Util::Variant Entity not implemented yet!");
	case 1:
		return Util::Variant(this->data.data[instance].localTransform);
	case 2:
		return Util::Variant(this->data.data[instance].worldTransform);
	case 3:
		return Util::Variant(this->data.data[instance].parent);
	case 4:
		return Util::Variant(this->data.data[instance].firstChild);
	case 5:
		return Util::Variant(this->data.data[instance].nextSibling);
	case 6:
		return Util::Variant(this->data.data[instance].prevSibling);
	default:
		n_assert2(false, "Component doesn't contain this attribute!\n");
		return Util::Variant();
	}
}

//------------------------------------------------------------------------------
/**
	@todo	Implement
*/
Util::Variant
TransformComponentBase::GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const
{
	if (attributeId == Attr::Owner)
	{
		// return Util::Variant(this->data.data[instance].owner);
		n_error("Util::Variant Entity not implemented yet!");
	}
	else if (attributeId == Attr::LocalTransform)
	{
		return Util::Variant(this->data.data[instance].localTransform);
	}
	else if (attributeId == Attr::WorldTransform)
	{
		return Util::Variant(this->data.data[instance].worldTransform);
	}
	else if (attributeId == Attr::Parent)
	{
		return Util::Variant(this->data.data[instance].parent);
	}
	else if (attributeId == Attr::FirstChild)
	{
		return Util::Variant(this->data.data[instance].firstChild);
	}
	else if (attributeId == Attr::NextSibling)
	{
		return Util::Variant(this->data.data[instance].nextSibling);
	}
	else if (attributeId == Attr::PreviousSibling)
	{
		return Util::Variant(this->data.data[instance].prevSibling);
	}
	
	n_assert2(false, "Component doesn't contain this attribute!\n");
	return Util::Variant();
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44& 
TransformComponentBase::LocalTransform(const uint32_t& instance)
{
	//return this->data.Get<1>(instance);
	return this->data.data[instance].localTransform;
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44& 
TransformComponentBase::WorldTransform(const uint32_t& instance)
{
	//return this->data.Get<2>(instance);
	return this->data.data[instance].worldTransform;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t& 
TransformComponentBase::Parent(const uint32_t& instance)
{
	//return this->data.Get<3>(instance);
	return this->data.data[instance].parent;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t& 
TransformComponentBase::FirstChild(const uint32_t& instance)
{
	//return this->data.Get<4>(instance);
	return this->data.data[instance].firstChild;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t& 
TransformComponentBase::NextSibling(const uint32_t& instance)
{
	//return this->data.Get<5>(instance);
	return this->data.data[instance].nextSibling;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t& 
TransformComponentBase::PrevSibling(const uint32_t& instance)
{
	//return this->data.Get<6>(instance);
	return this->data.data[instance].prevSibling;
}
	


} // namespace Game