//------------------------------------------------------------------------------
//  basecharacter.cc
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/base/basecharacter.h"

namespace Physics
{
__ImplementAbstractClass(Physics::BaseCharacter, 'APEN', Physics::PhysicsObject);

//------------------------------------------------------------------------------
/**
*/
BaseCharacter::BaseCharacter() : 
	radius(1.0f), 
	height(4.0f),
	crouchingHeight(height * 0.55f),
	mass(10.0f),
	shape(Capsule)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
BaseCharacter::~BaseCharacter()
{
	// empty
}

} // namespace Physics