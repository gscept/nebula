//------------------------------------------------------------------------------
//  component.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "component.h"

namespace Game {

//------------------------------------------------------------------------------
/**
*/
void
ComponentContainer::RegisterEntity(const Entity& entity)
{
	this->componentData.RegisterEntity(entity);
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentContainer::UnregisterEntity(const Entity& entity)
{
	this->componentData.DeregisterEntity(entity);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
ComponentContainer::GetInstance(const Entity& entity) const
{
	return this->componentData.GetInstance(entity);
}

//------------------------------------------------------------------------------
/**
*/
Component*
ComponentContainer::GetInstanceData(const uint32_t& instance)
{
	return &this->componentData[instance];
}

//------------------------------------------------------------------------------
/**
*/
Entity
ComponentContainer::GetOwner(const uint32_t& instance) const
{
	return this->componentData[instance].owner;
}

//------------------------------------------------------------------------------
/**
*/
bool
ComponentContainer::IsActive(const uint32_t& instance) const
{
	return this->componentData[instance].isActive;
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentContainer::OnBeginFrame()
{
	this->componentData.Optimize();

	// TODO: DLL OnBeginFrameFunction should be called here.
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentContainer::SetDescriptor(const uint32_t& instance, const Util::String & str)
{
	this->componentData[instance].descriptor = str;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
ComponentContainer::GetDescriptor(const uint32_t& instance) const
{
	return this->componentData[instance].descriptor;
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentContainer::SetMass(const uint32_t& instance, const Math::scalar& newMass)
{
	this->componentData[instance].mass = newMass;
}

//------------------------------------------------------------------------------
/**
*/
Math::scalar
ComponentContainer::GetMass(const uint32_t& instance) const
{
	return this->componentData[instance].mass;
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentContainer::SetVelocity(const uint32_t& instance, const Math::float4& vel)
{
	this->componentData[instance].vel = vel;
}

//------------------------------------------------------------------------------
/**
*/
Math::float4
ComponentContainer::GetVelocity(const uint32_t& instance) const
{
	return this->componentData[instance].vel;
}

} // namespace Game