#pragma once
//------------------------------------------------------------------------------
/**
	ComponentManager

	Holds components and acts as interface agains other systems.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/singleton.h"
#include "game/manager.h"
#include "util/delegate.h"
#include "util/bitfield.h"
#include "util/dictionary.h"
#include "ids/id.h"
#include "util/fourcc.h"

namespace Game
{
class ComponentInterface;

class ComponentManager : public Game::Manager
{
	__DeclareClass(ComponentManager)
	__DeclareSingleton(ComponentManager)
public:
	ComponentManager();
	~ComponentManager();

	/// Register a component and setup all event delegates for it.
	void RegisterComponent(ComponentInterface* component);

	/// Deregister all components. Removes all event delegates too.
	void DeregisterAll();

	/// Destroys all component instances of each component
	void ClearAll();

	/// Returns the number of components registered.
	SizeT GetNumComponents() const;

	ComponentInterface* GetComponentAtIndex(IndexT index);

	ComponentInterface* GetComponentByFourCC(const Util::FourCC& fourcc);

	/// Execute all OnBeginFrame events
	void OnBeginFrame();

	/// Execute all OnFrame events
	void OnRender();

	/// Execute all OnEndFrame events
	void OnEndFrame();

	/// Execute all OnRenderDebug events
	void OnRenderDebug();

private:
	Util::Array<ComponentInterface*> components;
};

} // namespace Game
