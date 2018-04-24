//------------------------------------------------------------------------------
//  component.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "transformcomponent.h"

__ImplementClass(Game::TransformComponent, 'TrCo', Game::BaseComponent)

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
TransformComponent::TransformComponent()
{
	this->events.SetBit(ComponentEvent::OnBeginFrame);
}

//------------------------------------------------------------------------------
/**
*/
TransformComponent::~TransformComponent()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
TestBeginFrame(TransformComponentInstance* i)
{
	i->localTransform = Math::matrix44::transpose(i->localTransform);
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::OnBeginFrame()
{
	__PerInstance(TransformComponentInstance, TestBeginFrame);
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetLocalTransform(const uint32_t& i, const Math::matrix44& val)
{
	this->data[i].localTransform = val;
	uint32_t parent = this->data[i].parent;
	uint32_t child;
	Math::matrix44 transform;
	// First of, transform this with parent transform if any
	if (parent != InvalidIndex)
	{
		transform = this->data[parent].worldTransform;
		this->data[i].worldTransform = Math::matrix44::multiply(this->data[i].localTransform, transform);
	}
	child = this->data[i].firstChild;
	parent = i;
	// Transform every child and their siblings.
	while (parent != InvalidIndex)
	{
		while (child != InvalidIndex)
		{
			transform = this->data[parent].worldTransform;
			this->data[child].worldTransform = Math::matrix44::multiply(this->data[parent].localTransform, transform);
			parent = child;
			child = this->data[child].firstChild;
		}
		child = this->data[parent].nextSibling;
		parent = this->data[parent].parent;
	}
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
TransformComponent::GetLocalTransform(const uint32_t& instance) const
{
	return this->data[instance].localTransform;
}

//------------------------------------------------------------------------------
/**
	@todo	Implement me!
*/
void
TransformComponent::SetWorldTransform(const uint32_t& instance, const Math::matrix44& val)
{
	this->data[instance].worldTransform = val;
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
TransformComponent::GetWorldTransform(const uint32_t& instance) const
{
	return this->data[instance].worldTransform;
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetParent(const uint32_t& instance, const uint32_t& val)
{
	this->data[instance].parent = val;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformComponent::GetParent(const uint32_t& instance) const
{
	return this->data[instance].parent;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformComponent::GetFirstChild(const uint32_t& instance) const
{
	return this->data[instance].firstChild;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformComponent::GetNextSibling(const uint32_t& instance) const
{
	return this->data[instance].nextSibling;
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformComponent::GetPrevSibling(const uint32_t& instance) const
{
	return this->data[instance].prevSibling;
}

} // namespace Game