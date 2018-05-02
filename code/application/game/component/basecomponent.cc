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
BaseComponent::UnregisterEntity(const Entity& entity)
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
BaseComponent::OnRenderDebug()
{
	// Override in subclass if neccessary.
}

} // namespace Game