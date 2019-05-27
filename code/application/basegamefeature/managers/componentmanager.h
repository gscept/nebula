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
#include "util/stringatom.h"
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
	void RegisterComponent(ComponentInterface* component, const Util::StringAtom& name, Util::FourCC fourcc);
	
	/// Deregister a component and all event delegates for it.
	void DeregisterComponent(ComponentInterface* component);

	/// Deregister all components. Removes all event delegates too.
	void DeregisterAll();

	/// Destroys all component instances of each component
	void ClearAll();

	/// Returns the number of components registered.
	SizeT GetNumComponents() const;

	/// Linear access to components
	ComponentInterface* GetComponentAtIndex(IndexT index);

	/// Retrieve a component by fourcc
	ComponentInterface* GetComponentByFourCC(const Util::FourCC& fourcc);
	
	/// Retrieve a component by name
	ComponentInterface* GetComponentByName(const Util::StringAtom& str);

	/// Enable a component with the given fourcc
	void EnableComponent(const Util::FourCC& fourcc);

	/// Disable a component with the given fourcc
	void DisableComponent(const Util::FourCC& fourcc);

	/// Execute all OnBeginFrame events
	void OnBeginFrame();

	/// Execute all OnFrame events
	void OnRender();

	/// Execute all OnEndFrame events
	void OnEndFrame();

	/// Execute all OnRenderDebug events
	void OnRenderDebug();

private:
	// We can afford double hashtables here because there'll never be THAT many components.
	Util::HashTable<Util::StringAtom, ComponentInterface*, 64> componentByName;
	Util::HashTable<Util::FourCC, ComponentInterface*, 64> componentByFourcc;
	Util::Array<ComponentInterface*> components;
};

} // namespace Game
