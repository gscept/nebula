#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::Actor

    A physics entity for a controllable actor.

    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "physics/physicsobject.h"

//------------------------------------------------------------------------------

namespace Physics
{
class BaseCharacter : public Physics::PhysicsObject
{
	__DeclareClass(BaseCharacter);
public:

	enum CharacterShape
	{
		Capsule = 0,
		Cylinder
	};

    /// constructor
    BaseCharacter();
    /// destructor
    virtual ~BaseCharacter();

	/// get the height of the character
	float GetHeight() const;
	/// set the height of the character
	virtual void SetHeight(float height);
    /// Sets crouching height
    virtual void SetCrouchingHeight(float height);
    /// Gets crouching height
    virtual float GetCrouchingHeight() const;
	/// get the radius of the character
	float GetRadius() const;
	/// set the radius of the character
	virtual void SetRadius(float radius);
	/// get the shape of the character
	CharacterShape GetShape() const;
	// set the shape of the character
	virtual void SetShape(CharacterShape shape);

	/// set movement direction
	virtual void SetMotionVector(const Math::vector& movement);
	/// sets biggest angle allowed for traversal
	virtual void SetMaxTraversableSlopeAngle(float angle);
	/// sets fall speed
	virtual void SetFallSpeed(float fallSpeed);
	/// jumps character
	virtual void Jump();
	/// sets speed of jumping
	virtual void SetJumpSpeed(float jumpSpeed);
	/// sets maximal jumping height
	virtual void SetMaxJumpHeight(float maxJumpHeight);
	/// returns true if character is on the ground
	virtual bool OnGround();
	/// returns linear velocity
	virtual Math::vector GetLinearVelocity() = 0;

    /// set crouching
    virtual void SetCrouching(bool enable);

	/// set movement speed
	virtual void SetMovementSpeed(float speed);
	/// set linear acceleration
	virtual void SetMaxLinearAcceleration(float acceleration);
	/// set velocity gain, between 0 and 1
	virtual void SetVelocityGain(float gain, float airGain);
	/// set characters weight
	virtual void SetWeight(float mass);

protected:

	CharacterShape shape; //< defaults to Capsule
	float radius;
	float height;
	float mass;
    float crouchingHeight;
};

//------------------------------------------------------------------------------
/**
*/
inline float 
BaseCharacter::GetHeight() const
{
	return height;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
BaseCharacter::SetHeight( float height )
{
	this->height = height;
}

//------------------------------------------------------------------------------
/**
*/
inline void
BaseCharacter::SetWeight(float mass)
{
	this->mass = mass;
}

//------------------------------------------------------------------------------
/**
*/
inline float 
BaseCharacter::GetCrouchingHeight() const
{
    return crouchingHeight;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
BaseCharacter::SetCrouchingHeight( float height )
{
    this->crouchingHeight = height;
}

//------------------------------------------------------------------------------
/**
*/
inline float 
BaseCharacter::GetRadius() const
{
	return radius;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
BaseCharacter::SetRadius( float radius )
{
	this->radius = radius;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
BaseCharacter::SetMaxTraversableSlopeAngle( float angle )
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
*/
inline void 
BaseCharacter::SetMotionVector( const Math::vector& movement )
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
*/
inline void 
BaseCharacter::SetFallSpeed( float fallSpeed )
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
*/
inline void 
BaseCharacter::Jump()
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
*/
inline void 
BaseCharacter::SetJumpSpeed( float jumpSpeed )
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
*/
inline void 
BaseCharacter::SetMaxJumpHeight( float maxJumpHeight )
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
BaseCharacter::OnGround()
{
	// override in subclass
	return false;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
BaseCharacter::SetMovementSpeed(float speed)
{
	// empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
inline void 
BaseCharacter::SetMaxLinearAcceleration(float acceleration)
{
	// empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
inline void 
BaseCharacter::SetVelocityGain(float gain, float airGain)
{
	// empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
inline void 
BaseCharacter::SetCrouching(bool enable)
{
    // override in subclass
}

//------------------------------------------------------------------------------
/**
*/
inline BaseCharacter::CharacterShape 
BaseCharacter::GetShape() const
{
	return this->shape;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
BaseCharacter::SetShape(CharacterShape shape)
{
	this->shape = shape;
}

}