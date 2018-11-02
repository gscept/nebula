//------------------------------------------------------------------------------
//  tagcomponent.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "tagcomponent.h"

namespace Attr
{
	DefineGuid(LocalTransform, 'TFLT', Attr::ReadWrite)
	DefineStringWithDefault(WorldTransform, 'TRWT', Attr::ReadWrite, "None");
} // namespace Attr

__ImplementClass(Game::TagComponent, 'TRCM', Game::BaseComponent);

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
TagComponent::TagComponent() : 
	component_templated_t({
		Attr::Tag, 
		Attr::TagName
	})
{
	// this->events.SetBit(ComponentEvent::OnActivate);
	// this->events.SetBit(ComponentEvent::OnDeactivate);
}

//------------------------------------------------------------------------------
/**
*/
TagComponent::~TagComponent()
{
	// Empty
}

//------------------------------------------------------------------------------
/**
*/
void
TagComponent::SetupAcceptedMessages()
{
	// Empty
}

uint32_t
TagComponent::RegisterEntity(const Entity & e)
{
	auto instance = component_templated_t::RegisterEntity(e);
#if IMMEDIATE_DELETION
	Game::EntityManager::Instance()->RegisterDeletionCallback(e, this);
#endif
	return instance;
}

void TagComponent::DeregisterEntity(const Entity & e)
{
#if IMMEDIATE_DELETION
	component_templated_t::DeregisterEntityImmediate(e);
	Game::EntityManager::Instance()->DeregisterDeletionCallback(e, this);
#else
	component_templated_t::DeregisterEntity(e);
#endif
}

//------------------------------------------------------------------------------
/**
*/
SizeT
TagComponent::Optimize()
{
#if IMMEDIATE_DELETION
	return 0;
#else
	return component_templated_t::OptimizeData();
#endif
}

} // namespace Game