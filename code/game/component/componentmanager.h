#pragma once
//------------------------------------------------------------------------------
/**
	ComponentManager

	Holds components and acts as interface agains other systems.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "basecomponent.h"
#include "util/delegate.h"
#include "util/bitfield.h"
#include "util/dictionary.h"

namespace Game
{

class ComponentManager
{
public:
	ComponentManager();
	~ComponentManager();

	/// Register a component and setup all event delegates for it.
	void RegisterComponent(const Ptr<BaseComponent>& component);

	// TODO: implement this.
	// void DeregisterComponent(const Ptr<BaseComponent>& component);

	/// Execute all OnBeginFrame events
	void OnBeginFrame();

	/// Execute all OnRender events
	void OnRender();

	/// Execute all OnRenderDebug events
	void OnRenderDebug();

private:
	Util::Array<Ptr<BaseComponent>> registry;

	/// All arrays containing the delegates for different events
	Util::Array<Util::Delegate<>> delegates_OnBeginFrame;
	Util::Array<Util::Delegate<>> delegates_OnRender;
	Util::Array<Util::Delegate<>> delegates_OnRenderDebug;

};

} // namespace Game
