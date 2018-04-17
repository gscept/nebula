#pragma once
//------------------------------------------------------------------------------
/**
	BaseComponent

	Components derive from this class.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/entity.h"
#include "componentmacros.h"
#include "util/bitfield.h"

namespace Game
{

enum ComponentEvent
{
	OnBeginFrame	= 1,
	OnRender		= 2,
	OnRenderDebug	= 3,
	NumEvents		= 4
};

class BaseComponent : public Core::RefCounted
{
	__DeclareClass(BaseComponent)
public:
	BaseComponent();
	~BaseComponent();

	/// Returns a bitfield containing the events this component is subscribed to.
	const Util::BitField<ComponentEvent::NumEvents>& SubscribedEvents() const;

	/// Registers an entity to this component. Will try to reuse old datablocks (clearing them first)
	/// within componentData and create new data if no free id is available.
	virtual void RegisterEntity(const Entity& entity);

	/// Unregisters an entity from this component. The data will still exist in the buffer until Optimize() is called.
	/// Note that even though this keeps the data intact, registering the same entity again won't result in the same data being used.
	virtual void UnregisterEntity(const Entity& entity);

	/// returns an index to the instance data within the data buffer.
	virtual uint32_t GetInstance(const Entity& entity) const;

	/// returns a instances owner entity id
	virtual Entity GetOwner(const uint32_t& instance) const;

	/// perform garbage collection. Returns number of erased instances.
	virtual SizeT Optimize();

	/// called at beginning of frame
	virtual void OnBeginFrame();

	/// called before rendering happens
	virtual void OnRender();

	/// called when game debug visualization is on
	virtual void OnRenderDebug();
	
protected:
	Util::BitField<ComponentEvent::NumEvents> events;

	// ComponentData<ComponentDataInstance> componentData;
	// ComponentData<TransformComponentInstance> inactiveComponents;
};

} // namespace Game