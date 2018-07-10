//------------------------------------------------------------------------------
//  basecomponent.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "basecomponent.h"

__ImplementClass(Game::BaseComponent, 'BaCo', Core::RefCounted)

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
BaseComponent::BaseComponent()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
BaseComponent::~BaseComponent()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
const Util::BitField<ComponentEvent::NumEvents>&
BaseComponent::SubscribedEvents() const
{
	return this->events;
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::RegisterEntity(const Entity& entity)
{
	// Override in subclass
	n_assert2(false, "Method has not been overridden!");
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::DeregisterEntity(const Entity& entity)
{
	// Override in subclass
	n_assert2(false, "Method has not been overridden!");
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
BaseComponent::GetInstance(const Entity & entity) const
{
	// Override in subclass
	n_assert2(false, "Method has not been overridden!");
	return uint32_t();
}

//------------------------------------------------------------------------------
/**
*/
Entity
BaseComponent::GetOwner(const uint32_t & instance) const
{
	// Override in subclass
	n_assert2(false, "Method has not been overridden!");
	return Entity();
}

//------------------------------------------------------------------------------
/**
*/
SizeT
BaseComponent::Optimize()
{
	// Override in subclass if neccessary.
	return 0;
}

//------------------------------------------------------------------------------
/**
*/
Util::Variant
BaseComponent::GetAttributeValue(uint32_t instance, IndexT attributeIndex) const
{
	// Override in subclass
	n_error("Not implemented!");
	return Util::Variant();
}

//------------------------------------------------------------------------------
/**
*/
Util::Variant
BaseComponent::GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const
{
	// Override in subclass
	n_error("NOT IMPLEMENTED\n");
	return Util::Variant();
}

//------------------------------------------------------------------------------
/**
*/
const Attr::AttrId&
BaseComponent::GetAttributeId(IndexT index) const
{
	return this->attributes[index];
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<Attr::AttrId>&
BaseComponent::GetAttributeIds() const
{
	return this->attributes;
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::Activate(const Entity & entity)
{
	// Override this.
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::Deactivate(const Entity & entity)
{
	// Override this.
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::OnActivate(const uint32_t& instance)
{
	// Override in subclass if neccessary.
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::OnDeactivate(const uint32_t& instance)
{
	// Override in subclass if neccessary.
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::OnBeginFrame()
{
	// Override in subclass if neccessary.
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::OnRender()
{
	// Override in subclass if neccessary.
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseComponent::OnEndFrame()
{
	// Override in subclass if neccessary.
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::OnRenderDebug()
{
	// Override in subclass if neccessary.
}

} // namespace Game