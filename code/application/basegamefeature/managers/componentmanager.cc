//------------------------------------------------------------------------------
//  componentmanager.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "game/component/componentinterface.h"
#include "componentmanager.h"

namespace Game
{

__ImplementClass(Game::ComponentManager, 'COMC', Game::Manager);
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
ComponentManager::RegisterComponent(ComponentInterface* component, const Util::StringAtom& name, Util::FourCC fourcc)
{
	// Check if each event is actually setup.
	auto events = component->SubscribedEvents();

#define CHECKEVENT(EVENT) if (events.IsSet(ComponentEvent::EVENT)) {\
							n_assert2(component->functions.EVENT != nullptr, #EVENT " event callback has not been defined in component function bundle!"); \
						  } else {\
							n_assert2(component->functions.EVENT == nullptr, #EVENT " event callback has been defined in bundle, but not set up in NIDL file!"); \
						  }

	CHECKEVENT(OnBeginFrame);
	CHECKEVENT(OnRender);
	CHECKEVENT(OnEndFrame);	
	CHECKEVENT(OnRenderDebug);
	CHECKEVENT(OnActivate);
	CHECKEVENT(OnDeactivate);
	CHECKEVENT(OnLoad);
	CHECKEVENT(OnSave);

	component->componentName = name;
	component->fourcc = fourcc;

	this->components.Append(component);
	this->componentByFourcc.Add(fourcc, component);
	this->componentByName.Add(name, component);
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentManager::DeregisterComponent(ComponentInterface * component)
{
	IndexT index = this->components.FindIndex(component);
	if (index != InvalidIndex)
	{
		this->components.EraseIndex(index);
	}

	auto it = this->componentByFourcc.Begin();
	while (true)
	{
		if (*it.val == component)
		{
			this->componentByFourcc.Erase(*it.key);
			break;
		}

		if (it == this->componentByFourcc.End())
			break;

		it++;
	}

	auto itName = this->componentByName.Begin();
	while (true)
	{
		if (*itName.val == component)
		{
			this->componentByName.Erase(*itName.key);
			break;
		}

		if (itName == this->componentByName.End())
			break;

		itName++;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentManager::DeregisterAll()
{
	this->componentByFourcc.Clear();
	this->componentByName.Clear();
	this->components.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentManager::ClearAll()
{
	for (SizeT i = 0; i < this->components.Size(); ++i)
	{
		if (this->components[i]->functions.DestroyAll != nullptr)
			this->components[i]->functions.DestroyAll();
	}
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
ComponentInterface*
ComponentManager::GetComponentAtIndex(IndexT index)
{
	return this->components[index];
}

//------------------------------------------------------------------------------
/**
*/
ComponentInterface*
ComponentManager::GetComponentByFourCC(const Util::FourCC & fourcc)
{
	IndexT idx = this->componentByFourcc.FindIndex(fourcc);
	if (idx != InvalidIndex)
	{
		return this->componentByFourcc.ValueAtIndex(fourcc, idx);
	}
	return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
ComponentInterface*
ComponentManager::GetComponentByName(const Util::StringAtom & str)
{
	IndexT idx = this->componentByName.FindIndex(str);
	if (idx != InvalidIndex)
	{
		return this->componentByName.ValueAtIndex(str, idx);
	}
	return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentManager::EnableComponent(const Util::FourCC & fourcc)
{
	auto component = GetComponentByFourCC(fourcc);
	if (component != nullptr)
		component->enabled = true;
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentManager::DisableComponent(const Util::FourCC & fourcc)
{
	auto component = GetComponentByFourCC(fourcc);
	if (component != nullptr)
		component->enabled = false;
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentManager::OnBeginFrame()
{
	// We need to clean up any erased components, no matter if they're registered to this event.
	// TODO: Can we do this in a better way?
	SizeT size = this->components.Size();
	for (SizeT i = 0; i < size; ++i)
	{
		this->components[i]->Optimize();
	}

	ComponentInterface* component;
	for (SizeT i = 0; i < size; ++i)
	{
		component = this->components[i];
		if (component->Enabled() == true && component->functions.OnBeginFrame != nullptr)
			component->functions.OnBeginFrame();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentManager::OnRender()
{
	SizeT length = this->components.Size();
	ComponentInterface* component;
	for (SizeT i = 0; i < length; ++i)
	{
		component = this->components[i];
		if (component->Enabled() && component->functions.OnRender != nullptr)
			component->functions.OnRender();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentManager::OnEndFrame()
{
	SizeT length = this->components.Size();
	ComponentInterface* component;
	for (SizeT i = 0; i < length; ++i)
	{
		component = this->components[i];
		if (component->Enabled() && component->functions.OnEndFrame != nullptr)
			component->functions.OnEndFrame();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentManager::OnRenderDebug()
{
	SizeT length = this->components.Size();
	for (SizeT i = 0; i < length; ++i)
	{
		if (this->components[i]->functions.OnRenderDebug != nullptr)
			this->components[i]->functions.OnRenderDebug();
	}
}

} // namespace Game
