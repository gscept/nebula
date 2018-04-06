//------------------------------------------------------------------------------
//  physxcharacter.cc
//  (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/physx/physxcharacter.h"
#include "characterkinematic/PxCapsuleController.h"
#include "physics/physx/physxscene.h"
#include "characterkinematic/PxControllerManager.h"
#include "physxphysicsserver.h"
#include "PxShape.h"
#include "PxRigidDynamic.h"
#include "PxSceneLock.h"
#include "physxutils.h"
#include "PxQueryFiltering.h"
#include "math/scalar.h"

using namespace Physics;
using namespace Math;
using namespace physx;



namespace PhysX
{
	
//------------------------------------------------------------------------------
/**
*/
class CharacterHitReport : public PxUserControllerHitReport
{
public:
	CharacterHitReport(PhysX::PhysXCharacter * character) { this->character = character; }
	///
	virtual void onShapeHit(const PxControllerShapeHit& hit);
	///
	virtual void onControllerHit(const PxControllersHit& hit) {}
	///
	virtual void onObstacleHit(const PxControllerObstacleHit& hit) {}
	PhysX::PhysXCharacter * character;
};

static void addForceAtPosInternal(PxRigidBody& body, const PxVec3& force, const PxVec3& pos, PxForceMode::Enum mode, bool wakeup)
{	
	const PxTransform globalPose = body.getGlobalPose();
	const PxVec3 centerOfMass = globalPose.transform(body.getCMassLocalPose().p);

	const PxVec3 torque = (pos - centerOfMass).cross(force);
	body.addForce(force, mode, wakeup);
	body.addTorque(torque, mode, wakeup);
}

static void addForceAtLocalPos(PxRigidBody& body, const PxVec3& force, const PxVec3& pos, PxForceMode::Enum mode, bool wakeup = true)
{
	//transform pos to world space
	const PxVec3 globalForcePos = body.getGlobalPose().transform(pos);

	addForceAtPosInternal(body, force, globalForcePos, mode, wakeup);
}

//------------------------------------------------------------------------------
/**
*/
void
CharacterHitReport::onShapeHit(const PxControllerShapeHit& hit)
{
	PxRigidDynamic* actor = hit.shape->getActor()->is<PxRigidDynamic>();
	if (actor)
	{
		if (actor->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC)
			return;

		// We only allow horizontal pushes. Vertical pushes when we stand on dynamic objects creates
		// useless stress on the solver. It would be possible to enable/disable vertical pushes on
		// particular objects, if the gameplay requires it.
		const PxVec3 upVector = hit.controller->getUpDirection();
		const PxF32 dp = hit.dir.dot(upVector);
		
		if (fabsf(dp) < 1e-3f)
			//		if(hit.dir.y==0.0f)
		{
			const PxTransform globalPose = actor->getGlobalPose();
			const PxVec3 localPos = globalPose.transformInv(toVec3(hit.worldPos));
			addForceAtLocalPos(*actor, hit.dir*hit.length*this->character->mass, localPos, PxForceMode::eACCELERATION);
		}
	}
}

__ImplementClass(PhysX::PhysXCharacter, 'PXCK', Physics::BaseCharacter);

//------------------------------------------------------------------------------
/**
*/
PhysXCharacter::PhysXCharacter():
	controller(NULL),
	jumpHeight(1.0f),	
	onGround(true)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
PhysXCharacter::~PhysXCharacter()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::Attach(Physics::BaseScene* world)
{
	this->hitReport = new CharacterHitReport(this);
	PxCapsuleControllerDesc desc;
	desc.height = this->height;
	desc.radius = this->radius;	
	desc.stepOffset = 0.0f;//this->jumpHeight;
	desc.material = PhysXServer::Instance()->GetMaterial(InvalidMaterial);
	desc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
	desc.reportCallback = this->hitReport;
	desc.density = this->mass;
	desc.userData = this;
	this->controller = ((PhysXScene*)world)->controllerManager->createController(desc);
	this->controller->setUserData(this);
	PxRigidDynamic* actor = this->controller->getActor();
	actor->userData = this;		
	this->filters = n_new(physx::PxControllerFilters());	
	this->lastFrame = PhysXServer::Instance()->GetTime();	
	this->SetTransform(this->transform);
	this->SetCollideCategory(Physics::Characters);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::Detach()
{
	this->controller->release();
	n_delete(this->filters);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::SetRadius(float radius)
{
	this->radius = radius;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::SetShape(CharacterShape shape)
{

}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::SetMotionVector(const Math::vector& movement)
{
	this->motionVector = movement;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::SetMaxTraversableSlopeAngle(float angle)
{

}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::SetFallSpeed(float fallSpeed)
{

}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::Jump()
{
	this->jump.Jump(this->jumpForce);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::SetJumpSpeed(float jumpSpeed)
{
	this->jumpForce = jumpSpeed;
}

//------------------------------------------------------------------------------
/**
*/
bool
PhysXCharacter::OnGround()
{
	return this->onGround;
}

//------------------------------------------------------------------------------
/**
*/
Math::vector
PhysXCharacter::GetLinearVelocity()
{
	return Math::vector(0, 0, 0);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::SetMaxJumpHeight(float maxJumpHeight)
{
	this->jumpHeight = maxJumpHeight;	
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::SetCrouching(bool enable)
{
	if (this->crouching == enable)
	{
		return;
	}
	this->crouching = enable;
	if (this->crouching)
	{
		this->controller->resize(this->crouchingHeight);
	}
	else
	{
		this->standup = true;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::SetMovementSpeed(float speed)
{
	this->moveSpeed = speed;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::SetMaxLinearAcceleration(float acceleration)
{

}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::SetVelocityGain(float gain, float airGain)
{

}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::OnFrameAfter()
{	
	Timing::Time now = PhysXServer::Instance()->GetTime();
	Timing::Time delta = now - this->lastFrame;
	float fdelta = (float)delta;
	this->lastFrame = now;
	if (this->standup)
	{
		this->TryStandup();
	}

	float heightDelta = this->jump.GetCurrentHeight(fdelta);
	if (heightDelta == 0.0f)
	{
		// apply normal gravity since we might be falling
		heightDelta = - this->controller->getScene()->getGravity().magnitude() * fdelta;
	}
	Math::vector move = this->motionVector;
	//move *= fdelta;
	move += Math::vector(0.0f, heightDelta, 0.0f);

	PxControllerCollisionFlags res = this->controller->move(Neb2PxVec(move), 0.0f, fdelta, *this->filters);
	if (res & PxControllerCollisionFlag::eCOLLISION_DOWN)
	{
		this->jump.Stop();
		this->onGround = true;
	}
	else
	{
		this->onGround = false;
	}
	PxExtendedVec3 vec = this->controller->getFootPosition();
	Math::point pos((float)vec.x, (float)vec.y, (float)vec.z);
	this->transform.set_position(pos);

}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::TryStandup()
{
	PxScene* scene = this->controller->getScene();
	PxSceneReadLock scopedLock(*scene);

	PxCapsuleController* capsuleCtrl = static_cast<PxCapsuleController*>(this->controller);

	PxReal r = capsuleCtrl->getRadius();
    n_assert2(this->crouchingHeight < this->height, "crouching height cant be larger than height");
        
	PxReal dh = this->height - this->crouchingHeight - 2 * r;
	PxCapsuleGeometry geom(r, dh*.5f);

	PxExtendedVec3 position = this->controller->getPosition();
	PxVec3 pos((float)position.x, (float)position.y + this->height*.5f + r, (float)position.z);
	PxQuat orientation(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f));
	PxQueryFilterData qd(PxQueryFlag::eANY_HIT | PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC);
	qd.data.word0 = Physics::Default | Physics::Static ;
	PxOverlapBuffer hit;
	if (scene->overlap(geom, PxTransform(pos, orientation), hit, qd))
		return;
	// if no hit, we can stand up
	this->controller->resize(this->height);

	this->standup = false;
	this->crouching = false;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::SetTransform(const Math::matrix44 & trans)
{
	this->transform = trans;
	if (this->controller)
	{
		Math::float4 f(trans.get_position());
		PxExtendedVec3 vec(f.x(), f.y(), f.z());
		this->controller->setFootPosition(vec);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::SetCollideCategory(Physics::CollideCategory coll)
{
	BaseCharacter::SetCollideCategory(coll);
	PhysXScene::SetCollideCategory(this->controller->getActor(), coll);
}

//------------------------------------------------------------------------------
/**
*/
PhysXCharacter::JumpController::JumpController():
	inJump(false),
	jumpGravity(20.0f)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
float
PhysXCharacter::JumpController::GetCurrentHeight(float delta)
{
	if (!this->inJump)
	{
		return 0.0f;
	}
	
	this->jumpStart += delta;
	float height = -this->jumpGravity * this->jumpStart * this->jumpStart + this->jumpForce * this->jumpStart;
	height *= delta;	
	return height;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::JumpController::Jump(float force)
{
	if (this->inJump)
	{
		return;
	}
	this->inJump = true;
	this->jumpStart = 0.0f;
	this->jumpForce = force;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXCharacter::JumpController::Stop()
{
	this->inJump = false;
}

} // namespace PhysX