//------------------------------------------------------------------------------
//  componentmanager.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "game/component/basecomponent.h"
#include "componentmanager.h"

namespace Game
{

__ImplementClass(Game::ComponentManager, 'COMA', Game::Manager);
__ImplementSingleton(Game::ComponentManager);

//------------------------------------------------------------------------------
/**
*/
ComponentManager::ComponentManager()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ComponentManager::~ComponentManager()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentManager::RegisterComponent(const Ptr<BaseComponent>& component)
{
	n_assert2(!this->registry.Contains(component->GetRtti()->GetFourCC()), "Component already registered!");

	this->components.InsertSorted(component);
	this->registry.Add(component->GetRtti()->GetFourCC(), component);

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
	if (bitField.IsSet(ComponentEvent::OnEndFrame))
	{
		auto d = Util::Delegate<>::FromMethod<BaseComponent, &BaseComponent::OnEndFrame>(component);
		this->delegates_OnEndFrame.Append(d);
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
void
ComponentManager::DeregisterComponent(const Ptr<BaseComponent>& component)
{
	IndexT i = this->components.FindIndex(component);
	this->components[i]->DestroyAll();
	this->registry.Erase(this->components[i]->GetRtti()->GetFourCC());
	this->components.EraseIndex(i);

	auto bitField = component->SubscribedEvents();
	if (bitField.IsSet(ComponentEvent::OnBeginFrame))
	{
		IndexT i = this->FindDelegateIndex(this->delegates_OnBeginFrame, component);
		if (i != InvalidIndex) this->delegates_OnBeginFrame.EraseIndex(i);
	}
	if (bitField.IsSet(ComponentEvent::OnRender))
	{
		IndexT i = this->FindDelegateIndex(this->delegates_OnRender, component);
		if (i != InvalidIndex) this->delegates_OnRender.EraseIndex(i);
	}
	if (bitField.IsSet(ComponentEvent::OnEndFrame))
	{
		IndexT i = this->FindDelegateIndex(this->delegates_OnEndFrame, component);
		if (i != InvalidIndex) this->delegates_OnEndFrame.EraseIndex(i);
	}
	if (bitField.IsSet(ComponentEvent::OnRenderDebug))
	{
		IndexT i = this->FindDelegateIndex(this->delegates_OnRenderDebug, component);
		if (i != InvalidIndex) this->delegates_OnRenderDebug.EraseIndex(i);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentManager::DeregisterAll()
{
	this->registry.Clear();
	// Destroy all component data
	for (SizeT i = 0; i < this->components.Size(); ++i)
	{
		this->components[i]->DestroyAll();
	}
	this->components.Clear();

	this->delegates_OnBeginFrame.Clear();
	this->delegates_OnEndFrame.Clear();
	this->delegates_OnRender.Clear();
	this->delegates_OnRenderDebug.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<class T>
const Ptr<T>& ComponentManager::GetComponent()
{
	const Core::Rtti rtti = T::RTTI;
	n_assert2(this->registry.Contains(rtti.GetFourCC()), "Component not registered to componentmanager!");
	return this->registry[rtti].cast<T>();
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ComponentManager::GetNumComponents() const
{
	return this->components.Size();
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<BaseComponent>&
ComponentManager::GetComponentAtIndex(IndexT index)
{
	return this->components[index];
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<BaseComponent>&
ComponentManager::ComponentByFourCC(const Util::FourCC & fourcc)
{
	n_assert2(this->registry.Contains(fourcc), "Component not registered to componentmanager!");
	return this->registry[fourcc];
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentManager::OnBeginFrame()
{
	// We need to clean up any erased components, no matter if they're registered to this event.
	// TODO: Can we do this in a better way?
	for (SizeT i = 0; i < this->components.Size(); ++i)
	{
		this->components[i]->Optimize();
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
ComponentManager::OnEndFrame()
{
	for (SizeT i = 0; i < this->delegates_OnEndFrame.Size(); ++i)
	{
		this->delegates_OnEndFrame[i]();
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

//------------------------------------------------------------------------------
/**
*/
IndexT
ComponentManager::FindDelegateIndex(const Util::Array<Util::Delegate<>>& delegateArray, const Ptr<BaseComponent>& component)
{
	for (SizeT i = 0; i < delegateArray.Size(); i++)
	{
		if (delegateArray[i].GetObject<BaseComponent>() == component)
		{
			return i;
		}
	}
	return InvalidIndex;
	
}

} // namespace Game
