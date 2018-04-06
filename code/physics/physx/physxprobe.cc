//------------------------------------------------------------------------------
//  physxprobe.cc
//  (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/physicsprobe.h"
#include "physics/collider.h"
#include "PxSimulationEventCallback.h"
#include "foundation/PxTransform.h"
#include "physxphysicsserver.h"
#include "PxMaterial.h"
#include "PxShape.h"
#include "PxScene.h"
#include "PxPhysics.h"
#include "physics/physx/physxbody.h"
#include "PxRigidStatic.h"
#include "physxscene.h"
#include "physxutils.h"
#include "physxcharacter.h"

using namespace Physics;
using namespace Math;
using namespace physx;

namespace PhysX
{
__ImplementClass(PhysX::PhysXProbe, 'PXPR', Physics::BaseProbe);

//------------------------------------------------------------------------------
/**
*/

PhysXProbe::PhysXProbe()
{
    this->common.type = Physics::PhysicsProbe::RTTI.GetFourCC();
    this->common.category = SensorTrigger;
    this->common.material = InvalidMaterial;    
}

//------------------------------------------------------------------------------
/**
*/

PhysXProbe::~PhysXProbe()
{
	this->overlap.Clear();
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Ptr<Core::RefCounted>>
PhysXProbe::GetOverlappingObjects()
{
	return this->overlap;
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysXProbe::Init(const Ptr<Physics::Collider> & coll, const Math::matrix44 & trans)
{
    this->common.collider = coll;
    this->common.startTransform = trans;

    Math::vector scale;
    Math::quaternion rotation;
    Math::float4 pos;
    trans.decompose(scale, rotation, pos);

    PxTransform pxStartTrans(Neb2PxVec(pos), Neb2PxQuat(rotation));
    this->body = PhysXServer::Instance()->physics->createRigidStatic(pxStartTrans);
    PxMaterial * mat;    
    mat = PhysXServer::Instance()->GetMaterial(InvalidMaterial);
    
    this->common.collider.cast<PhysXCollider>()->CreateInstance(this->body, scale, *mat);
    PxShape * shape;
    this->body->getShapes(&shape, 1);
    shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
    shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
	PxFilterData fd;
	fd.word0 = Physics::SensorTrigger;
	shape->setQueryFilterData(fd);
    this->body->userData = this;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXProbe::Attach(Physics::BaseScene * world)
{
    this->scene = ((PhysXScene*)world)->scene;
    this->scene->addActor(*this->body);
    this->attached = true;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXProbe::Detach()
{
    n_assert(this->attached);
    this->scene->removeActor(*this->body);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXProbe::OnTriggerEvent(PxPairFlag::Enum eventType, physx::PxActor * other)
{	
	Core::RefCounted* obj = (Core::RefCounted*)other->userData;
	if (obj->IsA(PhysX::PhysXBody::RTTI) || obj->IsA(PhysX::PhysXCharacter::RTTI))
	{
		Physics::PhysicsObject * otherBody = (Physics::PhysicsObject*)other->userData;
		if (otherBody->GetUserData() && otherBody->GetUserData()->object.isvalid())
		{
			Ptr<Core::RefCounted> obj = otherBody->GetUserData()->object;
			switch (eventType)
			{			
			case PxPairFlag::eNOTIFY_TOUCH_FOUND:
			{
				// if we do multiple simulation steps we might get called multiple times	
				if (this->overlap.BinarySearchIndex(obj) == InvalidIndex)
				{
					this->overlap.InsertSorted(obj);
				}
			}
			break;			
			case PxPairFlag::eNOTIFY_TOUCH_LOST:				
			{
				IndexT idx = this->overlap.BinarySearchIndex(obj);
				if (idx != InvalidIndex)
				{
					this->overlap.EraseIndex(idx);
				}
			}
			break;
			default:
				break;
			}			
		}
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXProbe::SetTransform(const Math::matrix44 & trans)
{
	PhysicsObject::SetTransform(trans);
	// physx does not allow rescaling of a trigger, ignore scale
	Math::vector scale;
	Math::quaternion rotation;
	Math::float4 pos;
	trans.decompose(scale, rotation, pos);

	PxTransform ptrans(Neb2PxVec(pos), Neb2PxQuat(rotation));	
	this->body->setGlobalPose(ptrans);
}

}