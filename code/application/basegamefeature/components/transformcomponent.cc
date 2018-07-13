//------------------------------------------------------------------------------
//  transformcomponent.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "transformcomponent.h"

__ImplementClass(Game::TransformComponent, 'TRCM', Game::TransformComponentBase);

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
TransformComponent::TransformComponent()
{
	// Empty
}

//------------------------------------------------------------------------------
/**
*/
TransformComponent::~TransformComponent()
{
	// Empty
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponent::SetLocalTransform(const uint32_t& i, const Math::matrix44& val)
{
	this->LocalTransform(i) = val;
	uint32_t parent = this->Parent(i);
	uint32_t child;
	Math::matrix44 transform;
	// First of, transform this with parent transform if any
	if (parent != InvalidIndex)
	{
		transform = this->WorldTransform(parent);
		this->WorldTransform(i) = Math::matrix44::multiply(this->LocalTransform(i), transform);
	}
	child = this->FirstChild(i);
	parent = i;
	// Transform every child and their siblings.
	while (parent != InvalidIndex)
	{
		while (child != InvalidIndex)
		{
			transform = this->WorldTransform(parent);
			this->WorldTransform(child) = Math::matrix44::multiply(this->LocalTransform(parent), transform);
			parent = child;
			child = this->FirstChild(child);
		}
		child = this->NextSibling(parent);
		parent = this->Parent(parent);
	}
}

} // namespace Game