#pragma once
//------------------------------------------------------------------------------
/**
	Component

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "util/string.h"
#include "util/hashtable.h"
#include "math/float4.h"
#include "math/scalar.h"
#include "component/componentdata.h"
#include "game/entity.h"

// Forward declare cr.h
struct cr_plugin;

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

	bool LoadPlugin();
	bool UnloadPlugin();

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

	/// called on begin of frame
	void OnBeginFrame();
	/*
	/// called when an instance is activated.
	void OnActivate();
	/// called when an instance is deactivated.
	void OnDeactivate();
	/// called after attributes are loaded
	void OnLoad();
	/// called after OnLoad when the complete world exist
	void OnStart();
	/// called before attributes are saved back to database
	void OnSave();

	/// called before rendering happens
	void OnRender();
	/// called when game debug visualization is on
	void OnRenderDebug();
	/// called when entity looses activity
	void OnLoseActivity();
	/// called when entity gains activity
	void OnGainActivity();
	*/

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

	// the host application should initalize a plugin with a context, a plugin
	cr_plugin* pluginContext;
};

} // namespace Game