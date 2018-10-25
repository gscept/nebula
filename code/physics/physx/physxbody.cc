//------------------------------------------------------------------------------
//  physxbody.cc
//  (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/physx/physxbody.h"
#include "physics/collider.h"
#include "PxRigidDynamic.h"
#include "physxutils.h"
#include "PxPhysics.h"
#include "PxMaterial.h"
#include "extensions/PxRigidBodyExt.h"
#include "physxphysicsserver.h"
#include "physxcollider.h"
#include "physics/physicsbody.h"
#include "extensions/PxDefaultSimulationFilterShader.h"

using namespace Physics;
using namespace physx;

namespace PhysX
{
__ImplementClass(PhysX::PhysXBody, 'PXRB', Physics::BaseRigidBody);


//------------------------------------------------------------------------------
/**
*/
PhysXBody::PhysXBody():
    body(NULL),
    scene(NULL)
{
    this->common.type = Physics::PhysicsBody::RTTI.GetFourCC();
}

//------------------------------------------------------------------------------
/**
*/
PhysXBody::~PhysXBody()
{
	if (this->body)
	{
		this->body->release();
		this->body = NULL;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysXBody::SetupFromTemplate(const PhysicsCommon & templ)
{
	BaseRigidBody::SetupFromTemplate(templ);    
    Math::quaternion rotation;
    Math::float4 pos;
    templ.startTransform.decompose(this->scale, rotation, pos);
	
	PxTransform pxStartTrans(Neb2PxVec(pos), Neb2PxQuat(rotation));
	this->body = PhysXServer::Instance()->physics->createRigidDynamic(pxStartTrans);    
	if (templ.bodyFlags & Physics::Kinematic)
	{
		this->SetKinematic(true);
	}
    this->SetMass(templ.mass);
    PxMaterial * mat;
    if(templ.material == InvalidMaterial && templ.friction >= 0.0f)
    { 
        mat = PhysXServer::Instance()->physics->createMaterial(templ.friction, templ.friction, templ.restitution);
    }
    else
    {
        mat = PhysXServer::Instance()->GetMaterial(templ.material);
    }
    templ.collider.cast<PhysXCollider>()->CreateInstance(this->body, this->scale, *mat);
	this->SetCollideCategory(Physics::Default);
	
    this->body->userData = this;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXBody::RenderDebug()
{
	
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysXBody::SetLinearVelocity(const Math::vector& v)
{
	this->body->setLinearVelocity(Neb2PxVec(v));
}

//------------------------------------------------------------------------------
/**
*/
Math::vector 
PhysXBody::GetLinearVelocity() const
{		
	return Px2NebVec(this->body->getLinearVelocity());	
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysXBody::SetAngularVelocity(const Math::vector& v)
{
	this->body->setAngularVelocity(Neb2PxVec(v));
}

//------------------------------------------------------------------------------
/**
*/
Math::vector 
PhysXBody::GetAngularVelocity() const
{
	return Px2NebVec(this->body->getAngularVelocity());
}

//------------------------------------------------------------------------------
/**
*/
Math::vector 
PhysXBody::GetCenterOfMassLocal()
{
	return Px2NebVec(this->body->getCMassLocalPose().p);	
}

//------------------------------------------------------------------------------
/**
*/
Math::vector 
PhysXBody::GetCenterOfMassWorld()
{
	n_error("BaseRigidBody::GetCenterOfMassWorld: Not implemented");
	return -1;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXBody::Attach(Physics::BaseScene * world)
{
	this->scene = ((PhysXScene*)world)->scene;
	this->scene->addActor(*this->body);
	this->attached = true;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXBody::Detach()
{
	n_assert(this->attached);
	this->scene->removeActor(*this->body);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXBody::SetMass(float m)
{
    if (m > 0.0f)
    {
        PxRigidBodyExt::setMassAndUpdateInertia(*this->body, m);
    }
    else
    {
        this->body->setMass(0.0f);
    }
}

//------------------------------------------------------------------------------
/**
*/
float
PhysXBody::GetMass() const
{
	return this->body->getMass();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXBody::SetEnableGravity(bool enable)
{
	this->body->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !enable);
}

//------------------------------------------------------------------------------
/**
*/
bool
PhysXBody::GetEnableGravity() const
{
	return !this->body->getActorFlags().isSet(PxActorFlag::eDISABLE_GRAVITY);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXBody::SetKinematic(bool enable)
{
	this->body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, enable);
}

//------------------------------------------------------------------------------
/**
*/
bool
PhysXBody::GetKinematic()
{
	return this->body->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXBody::ApplyImpulseAtPos(const Math::vector& impulse, const Math::point& pos, bool multByMass /*= false*/)
{
	Math::matrix44 m = Math::matrix44::inverse(this->GetTransform());
	Math::vector rpos = Math::matrix44::transform(pos, m);
	PxRigidBodyExt::addForceAtPos(*this->body, Neb2PxVec(impulse), Neb2PxVec(rpos), PxForceMode::eIMPULSE);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXBody::SetAngularDamping(float f)
{
	this->body->setAngularDamping(f);
}

//------------------------------------------------------------------------------
/**
*/
float
PhysXBody::GetAngularDamping() const
{
	return this->body->getAngularDamping();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXBody::SetLinearDamping(float f)
{
	this->body->setLinearDamping(f);
}

//------------------------------------------------------------------------------
/**
*/
float
PhysXBody::GetLinearDamping() const
{
	return this->body->getLinearDamping();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXBody::SetSleeping(bool sleeping)
{    
	if (sleeping)
	{
        this->body->getActorFlags().set(PxActorFlag::eDISABLE_SIMULATION);
	}
	else
	{
        this->body->getActorFlags().clear(PxActorFlag::eDISABLE_SIMULATION);
		this->body->wakeUp();
	}
}

//------------------------------------------------------------------------------
/**
*/
bool
PhysXBody::GetSleeping()
{
	return this->body->isSleeping();
}

//------------------------------------------------------------------------------
/**
*/
bool
PhysXBody::HasTransformChanged()
{
	return !this->body->isSleeping();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXBody::SetCollideCategory(CollideCategory coll)
{
	BaseRigidBody::SetCollideCategory(coll);
	PhysXScene::SetCollideCategory(this->body, (CollideCategory)coll);
}

//------------------------------------------------------------------------------
/**
*/
unsigned int
PhysXBody::GetCollideCategory() const
{
	return PxGetGroup(*this->body);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXBody::OnFrameBefore()
{
	if (this->HasTransformChanged())
	{
		PxTransform trans = this->body->getGlobalPose();
		Math::point origin;
		this->transform = Math::matrix44::transformation(origin, origin, this->scale, origin, Px2NebQuat(trans.q), Px2NebPoint(trans.p));
	}
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXBody::SetEnableCollisionCallback(bool enable)
{
	BaseRigidBody::SetEnableCollisionCallback(enable);
	for (unsigned int i = 0;i < this->body->getNbShapes();i++)
	{
		PxShape * shape;
		this->body->getShapes(&shape, 1, i);
		PxFilterData fd = shape->getSimulationFilterData();
		if (enable)
		{
			fd.word1 = CollisionSingle;
		}
		else
		{
			fd.word1 = 0;
		}
		shape->setSimulationFilterData(fd);
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXBody::SetTransform(const Math::matrix44 & trans)
{
    Math::quaternion q;
    Math::float4 scale;
    Math::float4 pos;
    trans.decompose(scale, q, pos);
	BaseRigidBody::SetTransform(trans);
    // we have to remove any possible scaling
    PxTransform pose(Neb2PxVec(pos), Neb2PxQuat(q));
	this->body->setGlobalPose(pose);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXBody::SetMaterialType(Physics::MaterialType t)
{
	BaseRigidBody::SetMaterialType(t);
	PxMaterial * mat = PhysXServer::Instance()->GetMaterial(t);
	for (unsigned int i = 0; i < this->body->getNbShapes(); i++)
	{
		PxShape * shape;
		this->body->getShapes(&shape, 1, i);
		shape->setMaterials(&mat, 1);
	}
}

} // namespace PhysX