//------------------------------------------------------------------------------
//  basecomponent.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "basecomponent.h"

__ImplementClass(Game::BaseComponent, 'BaCo', Core::RefCounted)

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
BaseComponent::BaseComponent()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
BaseComponent::~BaseComponent()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
const Util::BitField<ComponentEvent::NumEvents>&
BaseComponent::SubscribedEvents() const
{
	return this->events;
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::RegisterEntity(const Entity& entity)
{
	// Override in subclass
	n_assert2(false, "Method has not been overridden!");
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::DeregisterEntity(const Entity& entity)
{
	// Override in subclass
	n_assert2(false, "Method has not been overridden!");
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::OnEntityDeleted(Entity entity)
{
	// Override in subclass
	n_assert2(false, "Method has not been overridden!");
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::DeregisterAll()
{
	// Override in subclass
	n_assert2(false, "Method has not been overridden!");
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::DeregisterAllDead()
{
	// Override in subclass
	n_assert2(false, "Method has not been overridden!");
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::CleanData()
{
	// Override in subclass
	n_assert2(false, "Method has not been overridden!");
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::DestroyAll()
{
	// Override in subclass
	n_assert2(false, "Method has not been overridden!");
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
BaseComponent::GetNumInstances() const
{
	n_assert2(false, "Method has not been overridden!");
	return 0;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
BaseComponent::GetInstance(const Entity & entity) const
{
	// Override in subclass
	n_assert2(false, "Method has not been overridden!");
	return uint32_t();
}

//------------------------------------------------------------------------------
/**
*/
Entity
BaseComponent::GetOwner(const uint32_t & instance) const
{
	// Override in subclass
	n_assert2(false, "Method has not been overridden!");
	return Entity();
}

//------------------------------------------------------------------------------
/**
*/
SizeT
BaseComponent::Optimize()
{
	// Override in subclass if neccessary.
	return 0;
}

//------------------------------------------------------------------------------
/**
*/
Util::Variant
BaseComponent::GetAttributeValue(uint32_t instance, IndexT attributeIndex) const
{
	// Override in subclass
	n_error("Not implemented!");
	return Util::Variant();
}

//------------------------------------------------------------------------------
/**
*/
Util::Variant
BaseComponent::GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const
{
	// Override in subclass
	n_error("NOT IMPLEMENTED\n");
	return Util::Variant();
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::SetAttributeValue(uint32_t instance, IndexT attributeIndex, Util::Variant value)
{
	// Override in subclass if needed
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::SetAttributeValue(uint32_t instance, Attr::AttrId attributeId, Util::Variant value)
{
	// Override in subclass if needed
}

//------------------------------------------------------------------------------
/**
*/
const Attr::AttrId&
BaseComponent::GetAttributeId(IndexT index) const
{
	return this->attributeIds[index];
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<Attr::AttrId>&
BaseComponent::GetAttributeIds() const
{
	return this->attributeIds;
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::AllocInstances(uint num)
{
	n_assert2(false, "Method has not been overridden!");
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::SetDataFromBlobs(uint from, uint to, const Util::Array<Util::Blob>& data)
{
	n_assert2(false, "Method has not been overridden!");
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Util::Blob>
BaseComponent::GetDataAsBlobs()
{
	n_assert2(false, "Method has not been overridden!");
	return Util::Array<Util::Blob>();
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Util::Array<Entity>*>
BaseComponent::GetEntityAttributes()
{
	n_assert2(false, "Method has not been overridden!");
	return Util::Array<Util::Array<Entity>*>();
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::SetParents(const uint32_t& start, const uint32_t& end, const Util::Array<Entity>& entities, const Util::Array<uint32_t>& parentIndices)
{
	// Override in subclass if neccessary.
}


//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::OnActivate(const uint32_t& instance)
{
	// Override in subclass if neccessary.
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::OnDeactivate(const uint32_t& instance)
{
	// Override in subclass if neccessary.
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::OnBeginFrame()
{
	// Override in subclass if neccessary.
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::OnRender()
{
	// Override in subclass if neccessary.
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseComponent::OnEndFrame()
{
	// Override in subclass if neccessary.
}

//------------------------------------------------------------------------------
/**
*/
void
BaseComponent::OnRenderDebug()
{
	// Override in subclass if neccessary.
}

} // namespace Game