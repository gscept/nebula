#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::ExplosionAreaImpulse

    Implements an area impulse for a typical explosion. Applies an impulse
    with exponentail falloff to all rigid bodies within the range of the
    explosion witch satisfy a line-of-sight test. After Apply() is called,
    the object can be asked about all physics entities which have been
    affected.
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "physics/util/areaimpulse.h"
#include "util/array.h"
#include "physics/physicsbody.h"

//------------------------------------------------------------------------------
namespace Physics
{
class ContactPoint;
class RigidBody;

class ExplosionAreaImpulse : public AreaImpulse
{
	__DeclareClass(ExplosionAreaImpulse);
public:
    /// constructor
    ExplosionAreaImpulse();
    /// destructor
    virtual ~ExplosionAreaImpulse();
    /// apply impulse to the world
    void Apply();
    /// set position
	void SetPosition(const Math::point& p);
    /// get position
	const Math::point& GetPosition() const;
    /// set radius
    void SetRadius(float r);
    /// get radius
    float GetRadius() const;
    /// set max impulse
    void SetImpulse(float i);
    /// get max impulse
    float GetImpulse() const;
	/// set line of sight handling (only objects directly visible by the explosion will be affected
	void SetEnableLineOfSight(bool enable);
	/// get line of sight handling
	bool GetEnableLineOfSight() const;

private:
    /// apply impulse on single rigid body
    void HandleRigidBody(const Ptr<PhysicsBody> & rigidBody, const Math::point& pos);
    
	Math::point pos;
    float radius;
    float impulse;
	bool applyLineofSight;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
ExplosionAreaImpulse::SetPosition(const Math::point& p)
{
    this->pos = p;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Math::point&
ExplosionAreaImpulse::GetPosition() const
{
    return this->pos;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
ExplosionAreaImpulse::SetRadius(float r)
{
    this->radius = r;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
ExplosionAreaImpulse::GetRadius() const
{
    return this->radius;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
ExplosionAreaImpulse::SetImpulse(float i)
{
    this->impulse = i;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
ExplosionAreaImpulse::GetImpulse() const
{
    return this->impulse;
}

//------------------------------------------------------------------------------
/**
*/
inline
void 
ExplosionAreaImpulse::SetEnableLineOfSight(bool enable)
{
	this->applyLineofSight = enable;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool 
ExplosionAreaImpulse::GetEnableLineOfSight() const
{
	return this->applyLineofSight;
}

};
//------------------------------------------------------------------------------
