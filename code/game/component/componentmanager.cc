//------------------------------------------------------------------------------
//  componentmanager.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "componentmanager.h"

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
ComponentManager::ComponentManager()
{
}

//------------------------------------------------------------------------------
/**
*/
ComponentManager::~ComponentManager()
{
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentManager::RegisterComponent(const Ptr<BaseComponent>& component)
{
	this->registry.InsertSorted(component);
	auto bitField = component->SubscribedEvents();
	if (bitField.IsSet(ComponentEvent::OnBeginFrame))
	{
		auto d = Util::Delegate<>::FromMethod<BaseComponent, &BaseComponent::OnBeginFrame>(component);
		this->delegates_OnBeginFrame.Append(d);
	}
	if (bitField.IsSet(ComponentEvent::OnRender))
	{
		auto d = Util::Delegate<>::FromMethod<BaseComponent, &BaseComponent::OnRender>(component);
		this->delegates_OnBeginFrame.Append(d);
	}
	if (bitField.IsSet(ComponentEvent::OnRenderDebug))
	{
		auto d = Util::Delegate<>::FromMethod<BaseComponent, &BaseComponent::OnRenderDebug>(component);
		this->delegates_OnBeginFrame.Append(d);
	}
}

//------------------------------------------------------------------------------
/**
*/
//void
//ComponentManager::DeregisterComponent(const Ptr<BaseComponent>& component)
//{
//	IndexT i = this->registry.FindIndex(component);
//	this->registry.EraseIndex(i);
//
//	
//}

//------------------------------------------------------------------------------
/**
*/
void
ComponentManager::OnBeginFrame()
{
	// We need to clean up any erased components, no matter if they're registered to this event.
	for (SizeT i = 0; i < this->registry.Size(); ++i)
	{
		this->registry[i]->Optimize();
	}

	// Run all event delegates
	for (SizeT i = 0; i < this->delegates_OnBeginFrame.Size(); ++i)
	{
		this->delegates_OnBeginFrame[i]();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentManager::OnRender()
{
	for (SizeT i = 0; i < this->delegates_OnRender.Size(); ++i)
	{
		this->delegates_OnRender[i]();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentManager::OnRenderDebug()
{
	for (SizeT i = 0; i < this->delegates_OnRenderDebug.Size(); ++i)
	{
		this->delegates_OnRenderDebug[i]();
	}
}

} // namespace Game
