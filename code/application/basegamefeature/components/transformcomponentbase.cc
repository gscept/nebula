//------------------------------------------------------------------------------
//  transformcomponentbase.cc
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
	//this->events.SetBit(ComponentEvent::OnBeginFrame);

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
	this->inactiveData.RegisterEntity(entity);
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponentBase::DeregisterEntity(const Entity& entity)
{
	uint32_t index = this->data.GetInstance(entity);
	if (index != InvalidIndex)
	{
		this->data.DeregisterEntity(entity);
		return;
	}

	index = this->inactiveData.GetInstance(entity);
	if (index != InvalidIndex)
	{
		this->inactiveData.DeregisterEntityImmediate(entity, index);
		return;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponentBase::DeregisterAll()
{
	this->data.DeregisterAll();
	this->inactiveData.DeregisterAll();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponentBase::DeregisterAllDead()
{
	this->data.DeregisterAllInactive();
	this->inactiveData.DeregisterAllInactive();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponentBase::CleanData()
{
	this->data.Clean();
	this->inactiveData.Clean();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponentBase::DestroyAll()
{
	this->data.DestroyAll();
	this->inactiveData.DestroyAll();
}

//------------------------------------------------------------------------------
/**
*/
bool
TransformComponentBase::IsRegistered(const Entity& entity) const
{
	return this->data.GetInstance(entity) != InvalidIndex || this->inactiveData.GetInstance(entity) != InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponentBase::Activate(const Entity& entity)
{
	n_assert2(this->IsRegistered(entity), "Cannot activate component for an entity that is not registered!");

	if (this->data.GetInstance(entity) != InvalidIndex || !EntityManager::Instance()->IsAlive(entity)) return;

	auto inactiveInstance = this->inactiveData.GetInstance(entity);

	if (inactiveInstance != InvalidIndex)
	{
		uint32_t newInstance = this->data.RegisterEntity(entity);
		this->data.SetInstanceData(newInstance, this->inactiveData.data.Get<1>(inactiveInstance));

		// NOTE: If this would be registered to OnActivate event, we would call OnActivate(newInstance) here.

		this->inactiveData.DeregisterEntityImmediate(entity, inactiveInstance);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponentBase::Deactivate(const Entity& entity)
{
	n_assert2(this->IsRegistered(entity), "Cannot Deactivate component for an entity that is not even registered!");

	if (!this->data.GetInstance(entity) || !EntityManager::Instance()->IsAlive(entity)) return;

	auto instance = this->data.GetInstance(entity);

	if (instance != InvalidIndex)
	{
		// NOTE: If this would be registered to OnDeactivate event, we would call OnDeactivate(instance) here.

		uint32_t inactiveInstance = this->inactiveData.RegisterEntity(entity);
		this->inactiveData.SetInstanceData(inactiveInstance, this->data.data.Get<1>(instance));
		this->data.DeregisterEntityImmediate(entity, instance);
	}
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformComponentBase::GetInstance(const Entity& entity) const
{
	return this->data.GetInstance(entity);
}

//------------------------------------------------------------------------------
/**
*/
Entity
TransformComponentBase::GetOwner(const uint32_t& instance) const
{
	return this->data.GetOwner(instance);
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
*/
Util::Variant
TransformComponentBase::GetAttributeValue(uint32_t instance, IndexT attributeIndex) const
{
	switch (attributeIndex)
	{
	case 0:
		return Util::Variant(this->data.data.Get<0>(instance).id);
	case 1:
		return Util::Variant(this->data.data.Get<1>(instance).localTransform);
	case 2:
		return Util::Variant(this->data.data.Get<1>(instance).worldTransform);
	case 3:
		return Util::Variant(this->data.data.Get<1>(instance).parent);
	case 4:
		return Util::Variant(this->data.data.Get<1>(instance).firstChild);
	case 5:
		return Util::Variant(this->data.data.Get<1>(instance).nextSibling);
	case 6:
		return Util::Variant(this->data.data.Get<1>(instance).prevSibling);
	default:
		n_assert2(false, "Component doesn't contain this attribute!\n");
		return Util::Variant();
	}
}

//------------------------------------------------------------------------------
/**
*/
Util::Variant
TransformComponentBase::GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const
{
	if (attributeId == Attr::Owner)
	{
		return Util::Variant(this->data.data.Get<0>(instance).id);
	}
	else if (attributeId == Attr::LocalTransform)
	{
		return Util::Variant(this->data.data.Get<1>(instance).localTransform);
	}
	else if (attributeId == Attr::WorldTransform)
	{
		return Util::Variant(this->data.data.Get<1>(instance).worldTransform);
	}
	else if (attributeId == Attr::Parent)
	{
		return Util::Variant(this->data.data.Get<1>(instance).parent);
	}
	else if (attributeId == Attr::FirstChild)
	{
		return Util::Variant(this->data.data.Get<1>(instance).firstChild);
	}
	else if (attributeId == Attr::NextSibling)
	{
		return Util::Variant(this->data.data.Get<1>(instance).nextSibling);
	}
	else if (attributeId == Attr::PreviousSibling)
	{
		return Util::Variant(this->data.data.Get<1>(instance).prevSibling);
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
	return this->data.data.Get<1>(instance).localTransform;
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44& 
TransformComponentBase::WorldTransform(const uint32_t& instance)
{
	return this->data.data.Get<1>(instance).worldTransform;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t& 
TransformComponentBase::Parent(const uint32_t& instance)
{
	return this->data.data.Get<1>(instance).parent;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t& 
TransformComponentBase::FirstChild(const uint32_t& instance)
{
	return this->data.data.Get<1>(instance).firstChild;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t& 
TransformComponentBase::NextSibling(const uint32_t& instance)
{
	return this->data.data.Get<1>(instance).nextSibling;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t& 
TransformComponentBase::PrevSibling(const uint32_t& instance)
{
	return this->data.data.Get<1>(instance).prevSibling;
}
	


} // namespace Game