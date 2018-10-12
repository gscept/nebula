//------------------------------------------------------------------------------
//  bulletcharacter.cc
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/bullet/bulletcharacter.h"
#include "physics/bullet/bulletscene.h"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "BulletDynamics/Character/btKinematicCharacterController.h"
#include "physics/bullet/conversion.h"

namespace Bullet
{
__ImplementClass(Bullet::BulletCharacter, 'BTCH', Physics::BaseCharacter);

using namespace Math;
using namespace Physics;

class NBKinematicCharacterController : public btKinematicCharacterController
{
public:
    NBKinematicCharacterController (btPairCachingGhostObject* ghostObject,btConvexShape* convexShape,btScalar stepHeight, int upAxis = 1)
        :btKinematicCharacterController(ghostObject,convexShape,stepHeight,upAxis)
    {

    }
    void SetGhostObject(btPairCachingGhostObject * ghost, btConvexShape * capsule)
    {
        this->m_ghostObject = ghost;       
        this->m_convexShape = capsule;
    }

};



//------------------------------------------------------------------------------
/**
*/
BulletCharacter::BulletCharacter() :
	ghostObject(NULL),
    crouchingGhostObject(NULL),
	characterController(NULL),
	world(NULL),
    crouching(false),
    maxJumpHeight(0.0f)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
BulletCharacter::~BulletCharacter()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCharacter::Attach( Physics::BaseScene* world )
{
	//PhysicsObject::Attach(world);

	// set world pointer
	this->world = ((BulletScene*)world)->GetWorld();
		
	// create collision capsule
	switch(this->shape)
	{
		case Capsule:
		{
			this->capsule = new btCapsuleShape(this->radius, this->height);
			this->crouchingCapsule = new btCapsuleShape(this->radius, this->crouchingHeight);
            // create capsule offset transforms
            this->capsuleOffset.translate(vector(0,this->height*0.5f+this->radius,0));;
            this->inverseCapsuleOffset.translate(vector(0,-this->height*0.5f-this->radius,0));

            // create crouching capsule offset transforms
            this->crouchingCapsuleOffset.translate(vector(0,this->crouchingHeight*0.5f+this->radius,0));;
            this->inverseCrouchingCapsuleOffset.translate(vector(0,-this->crouchingHeight*0.5f-this->radius,0));
		}
		break;
		case Cylinder:
		{
			this->capsule = new btCylinderShape(btVector3(this->radius, this->height, 0.0f));
			this->crouchingCapsule = new btCylinderShape(btVector3(this->radius, this->crouchingHeight, 0.0f));
            // create cylinder offset transforms
            this->capsuleOffset.translate(vector(0,this->height,0));;
            this->inverseCapsuleOffset.translate(vector(0,-this->height,0));

            // create crouching cylinder offset transforms
            this->crouchingCapsuleOffset.translate(vector(0,this->crouchingHeight,0));
            this->inverseCrouchingCapsuleOffset.translate(vector(0,-this->crouchingHeight,0));
		}
		break;
	}
    // create ghost object
    this->ghostObject = new btPairCachingGhostObject();
    this->ghostObject->setWorldTransform(Neb2BtM44Transform(Math::matrix44::multiply(this->transform,this->capsuleOffset)));

	this->ghostObject->setCollisionShape(this->capsule);
	this->ghostObject->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
	this->ghostObject->setUserPointer(this);

    // create ghost object
    this->crouchingGhostObject = new btPairCachingGhostObject();
    this->crouchingGhostObject->setWorldTransform(Neb2BtM44Transform(Math::matrix44::multiply(this->transform,this->crouchingCapsuleOffset)));

    // create collision capsule
    
    this->crouchingGhostObject->setCollisionShape(this->crouchingCapsule);
    this->crouchingGhostObject->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
    this->crouchingGhostObject->setUserPointer(this);

	// create character controller
	this->characterController = new NBKinematicCharacterController(this->ghostObject, this->capsule, 0.35f);
    this->characterController->setMaxJumpHeight(this->maxJumpHeight);

	PhysicsObject::SetCollideCategory(Physics::Characters);
	this->world->addCollisionObject(this->ghostObject, btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::AllFilter);
	this->world->addAction(this->characterController);
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCharacter::Detach()
{
	//PhysicsObject::Detach();

	// remove from dynamics world
    if(this->crouching)
    {
        this->world->removeCollisionObject(this->crouchingGhostObject);
    }
    else
    {
	    this->world->removeCollisionObject(this->ghostObject);
    }
	this->world->removeAction(this->characterController);

	// delete pointers
	delete this->ghostObject;
    delete this->crouchingGhostObject;
	delete this->characterController;

	this->ghostObject = 0;
    this->crouchingGhostObject = 0;
	this->characterController = 0;
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCharacter::SetTransform( const Math::matrix44 & trans )
{
    if(this->crouching)
    {
	    if (this->crouchingGhostObject)
	    {
    		this->crouchingGhostObject->setWorldTransform(Neb2BtM44Transform(Math::matrix44::multiply(trans, this->crouchingCapsuleOffset)));
    	}
    }
    else
    {
        if (this->ghostObject)
        {
            this->ghostObject->setWorldTransform(Neb2BtM44Transform(Math::matrix44::multiply(trans, this->capsuleOffset)));
        }
    }

	// run base class
	PhysicsObject::SetTransform(trans);
}

//------------------------------------------------------------------------------
/**
*/
const Math::matrix44& 
BulletCharacter::GetTransform()
{
    if(this->crouching)
    {
	    if (this->crouchingGhostObject)
	    {
    		this->transform = Math::matrix44::multiply(this->inverseCrouchingCapsuleOffset, Bt2NebTransform(this->crouchingGhostObject->getWorldTransform()));
    	}
    }
    else
    {
        if (this->ghostObject)
        {
            this->transform = Math::matrix44::multiply(this->inverseCapsuleOffset, Bt2NebTransform(this->ghostObject->getWorldTransform()));
        }
    }

	return this->transform;
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCharacter::SetFallSpeed( float fallSpeed )
{
	this->characterController->setFallSpeed(fallSpeed);
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCharacter::SetMotionVector( const Math::vector& movement )
{
	this->characterController->setWalkDirection(Neb2BtVector(movement));
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCharacter::Jump()
{
	this->characterController->jump();
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCharacter::SetJumpSpeed( float jumpSpeed )
{
	this->characterController->setJumpSpeed(jumpSpeed);
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCharacter::SetMaxJumpHeight( float maxJumpHeight )
{    
    if (this->characterController)
    {
        this->characterController->setMaxJumpHeight(maxJumpHeight);
    }	
    this->maxJumpHeight = maxJumpHeight;
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCharacter::SetMaxTraversableSlopeAngle( float angle )
{
	this->characterController->setMaxSlope(angle);
}

//------------------------------------------------------------------------------
/**
*/
bool 
BulletCharacter::OnGround()
{
	return this->characterController->onGround();
}

//------------------------------------------------------------------------------
/**
*/
Math::vector 
BulletCharacter::GetLinearVelocity()
{
    if(this->crouching)
    {
        return Bt2NebVector(this->crouchingGhostObject->getInterpolationLinearVelocity());
    }
    else
    {
        return Bt2NebVector(this->ghostObject->getInterpolationLinearVelocity());
    }    
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCharacter::SetCrouching(bool enable)
{
    if(this->crouching == enable)
    {
        return;
    }
    
    matrix44 oldTrans = this->GetTransform();

    this->crouching = enable;    
    if(this->crouching)
    {
        this->world->addCollisionObject(this->crouchingGhostObject, btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::AllFilter);
        this->world->removeCollisionObject(this->ghostObject);
        this->characterController->SetGhostObject(this->crouchingGhostObject, this->crouchingCapsule);
    }
    else
    {
        this->world->addCollisionObject(this->ghostObject, btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::AllFilter);
        this->world->removeCollisionObject(this->crouchingGhostObject);
        this->characterController->SetGhostObject(this->ghostObject, this->capsule);
    }
    this->SetTransform(oldTrans);
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCharacter::SetCollideCategory(CollideCategory coll)
{
	PhysicsObject::SetCollideCategory(coll);

	n_assert2(this->ghostObject != NULL, "No ghost object! Cannot set collision category before character is attached to physics scene");

	this->ghostObject->getBroadphaseHandle()->m_collisionFilterGroup = coll;
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCharacter::SetCollideFilter(uint mask)
{
	PhysicsObject::SetCollideFilter(mask);

	n_assert2(this->ghostObject != NULL, "No ghost object! Cannot set collision filter before character is attached to physics scene");

	this->ghostObject->getBroadphaseHandle()->m_collisionFilterMask = mask;
}

} // namespace Bullet