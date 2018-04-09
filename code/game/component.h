#pragma once
//------------------------------------------------------------------------------
/**
	Component

	This is an example component.
	Components that use reloadable shared libraries be automatically generated later.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "util/string.h"
#include "util/hashtable.h"
#include "math/float4.h"
#include "math/scalar.h"
#include "component/componentdata.h"
#include "component/plugins/cplugin.h"
#include "game/entity.h"

namespace Game
{

typedef struct
{
	// general component attributes
	Entity owner; // Owner entity
	bool isActive;

	// specific component attributes
	Util::String descriptor;
	Math::float4 vel;
	Math::scalar mass;
} Component;

class ComponentContainer
{
public:
	ComponentContainer();
	~ComponentContainer();

	/// Registers an entity to this component. Will try to reuse old datablocks (clearing them first) 
	/// within componentData and create new data if no free id is available.
	void RegisterEntity(const Entity& entity);
	/// Unregisters an entity from this component. The data will still exist in the buffer until Optimize() is called.
	/// Note that even though this keeps the data intact, registering the same entity again won't result in the same data being used.
	void UnregisterEntity(const Entity& entity);

	/// returns an index to the instance data within the data buffer.
	uint32_t GetInstance(const Entity& entity) const;
	/// returns a instances owner entity id
	Entity GetOwner(const uint32_t& instance) const;
	/// return true if property is currently active
	bool IsActive(const uint32_t& instance) const;

	/// Access to component plugin interface.
	ComponentPlugin* Plugin();

	/// called on begin of frame
	void OnBeginFrame();

	//-----------------------------------------------------
	// Specific attributes getters and setters.

	void SetDescriptor(const uint32_t& instance, const Util::String& str);
	Util::String GetDescriptor(const uint32_t& instance) const;
	void SetVelocity(const uint32_t& instance, const Math::float4& vel);
	Math::float4 GetVelocity(const uint32_t& instance) const;
	void SetMass(const uint32_t& instance, const Math::scalar& scalar);
	Math::scalar GetMass(const uint32_t& instance) const;
	
private:
	/// returns a pointer to the instance data for a given index.
	Component* GetInstanceData(const uint32_t& instance);

	ComponentData<Component> componentData;

	CPlugin plugin;
};

} // namespace Game