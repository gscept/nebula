//------------------------------------------------------------------------------
//  physics/basejoint.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/physicsbody.h"
#include "physics/base/basejoint.h"

namespace Physics
{
__ImplementAbstractClass(Physics::BaseJoint, 'PJO2', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
BaseJoint::BaseJoint() : 
    type(InvalidType),
    isAttached(false)
{
    // empty
}


void
BaseJoint::SetType(JointType t)
{
	type = t;
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseJoint::SetBodies(const Ptr<PhysicsBody> & body1, const Ptr<PhysicsBody> & body2)
{
    this->rigidBody1 = body1;
    this->rigidBody2 = body2;
}

//------------------------------------------------------------------------------
/**
    Pointer to the first body to which the joint is attached to.
*/
const Ptr<PhysicsBody> &
BaseJoint::GetBody1() const
{
    return this->rigidBody1;
}

//------------------------------------------------------------------------------
/**
    Pointer to the second body to which the joint is attached to.
*/
const Ptr<PhysicsBody> &
BaseJoint::GetBody2() const
{
    return this->rigidBody2;
}

//------------------------------------------------------------------------------
/**
    Render the debug visualization of this shape.
*/
void
BaseJoint::RenderDebug()
{
    // empty
}

} // namespace Physics
