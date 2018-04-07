#pragma once
//------------------------------------------------------------------------------
/**
    @class Bullet::BulletCharacter
    
    Implements a bullet-specific character
    
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "physics/base/basecharacter.h"

class btKinematicCharacterController;
class btPairCachingGhostObject;
class btDynamicsWorld;
class btConvexShape;

namespace Bullet
{

class NBKinematicCharacterController;

class BulletCharacter : public Physics::BaseCharacter
{
	__DeclareClass(BulletCharacter);
public:
	/// constructor
	BulletCharacter();
	/// destructor
	virtual ~BulletCharacter();

	/// overrides setting transform
	void SetTransform(const Math::matrix44 & trans);
	/// overrides getting transform
	const Math::matrix44& GetTransform();

	/// sets fall speed
	void SetFallSpeed(float fallSpeed);
	/// sets motion vector
	void SetMotionVector(const Math::vector& movement);
	/// jumps character
	void Jump();
	/// sets jump speed
	void SetJumpSpeed(float jumpSpeed);
	/// sets jump height
	void SetMaxJumpHeight(float maxJumpHeight);
	/// sets largest traversable angle
	void SetMaxTraversableSlopeAngle(float angle);
	/// returns true if character is on the ground
	bool OnGround();
    /// returns linear velocity
    Math::vector GetLinearVelocity();
    /// set crouching
    void SetCrouching(bool enable);
	
	/// set collide category
	void SetCollideCategory(Physics::CollideCategory coll);
	/// set collide filter
	void SetCollideFilter(uint mask);

protected:
	/// called when character gets attached
	void Attach(Physics::BaseScene* world);
	/// called when character gets detached
	void Detach();
    
	btDynamicsWorld* world;
	NBKinematicCharacterController* characterController;
	btPairCachingGhostObject* ghostObject;
    btConvexShape* capsule;
    btConvexShape* crouchingCapsule;
    btPairCachingGhostObject* crouchingGhostObject;
	Math::matrix44 capsuleOffset, inverseCapsuleOffset;
    Math::matrix44 crouchingCapsuleOffset, inverseCrouchingCapsuleOffset;
    bool crouching;
    float maxJumpHeight;
};
} // namespace Bullet
//------------------------------------------------------------------------------