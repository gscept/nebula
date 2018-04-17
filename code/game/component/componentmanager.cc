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
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentManager::OnRenderDebug()
{
}

} // namespace Game
