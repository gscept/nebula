//------------------------------------------------------------------------------
//  physics/explosionareaimpulse.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/util/explosionareaimpulse.h"
#include "physics/physicsserver.h"
#include "physics/physicsbody.h"
#include "../filterset.h"

namespace Physics
{
__ImplementClass(Physics::ExplosionAreaImpulse, 'PEAI', Physics::AreaImpulse);

using namespace Math;
//------------------------------------------------------------------------------
/**
*/
ExplosionAreaImpulse::ExplosionAreaImpulse() :
    radius(1.0f),
    impulse(1.0f),
	applyLineofSight(true)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ExplosionAreaImpulse::~ExplosionAreaImpulse()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
ExplosionAreaImpulse::Apply()
{

	Util::Array<Ptr<Physics::PhysicsObject> > objs;
	FilterSet dummy;	
	PhysicsServer::Instance()->GetScene()->GetObjectsInSphere(this->pos,this->radius,dummy,objs);

	for(int i=0;i<objs.Size();i++)
	{
		// ignore non rigid bodies
		if(!objs[i]->IsA(PhysicsBody::RTTI))
			continue;
		// FIXME this should use the contact of the object with the sphere instead of its center
		this->HandleRigidBody(objs[i].cast<PhysicsBody>(),objs[i]->GetTransform().get_position());
	}    
}

//------------------------------------------------------------------------------
/**
    Applies impulse on single rigid body. Does line of sight test on
    the center of the rigid body (FIXME: check all corners of the 
    bounding box??).
*/
void 
ExplosionAreaImpulse::HandleRigidBody(const Ptr<PhysicsBody> & rigidBody, const Math::point& pos)
{
    // do line of sight check to position of body
    FilterSet excludeSet;
    excludeSet.AddRigidBodyId(rigidBody->GetUniqueId());
    Math::vector dirVec = pos - this->pos;

	bool apply = true;
	if(applyLineofSight)
	{
		Ptr<Contact> contact = PhysicsServer::Instance()->GetScene()->GetClosestContactAlongRay(this->pos,dirVec,excludeSet);

		// something in the way?
		if(contact.isvalid())
			apply = false;

	}
	
	if(apply)
	{
        // free line of sight, apply impulse
		float dist = dirVec.length();
		dirVec = float4::normalize(dirVec);

		// scale impulse by distance
		float attenuate = 1.0f - n_saturate(dist / this->radius);
		Math::vector impulse = dirVec * this->impulse * attenuate;

		rigidBody->ApplyImpulseAtPos(impulse, pos);		
	}   
}

}; // namespace Physics