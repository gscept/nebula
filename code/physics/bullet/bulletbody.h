#pragma once
//------------------------------------------------------------------------------
/**
	@class Bullet::BulletBody

	(C) 2012-2018 Individual contributors, see AUTHORS file
*/
#include "physics/base/basebody.h"


class btRigidBody;
class btDynamicsWorld;

namespace Bullet
{
class Scene;
class BulletScene;
class Collider;
class BulletCollider;
class NebulaMotionState;

class BulletBody : public Physics::BaseRigidBody
{
	__DeclareClass(BulletBody);

public:
	/// constructor
	BulletBody();
	/// destructor
	virtual ~BulletBody();

	/// set the body's world space transform
	virtual void SetTransform(const Math::matrix44& m);
	/// get the body's world space transform
	virtual const Math::matrix44& GetTransform();

	/// set the body's linear velocity
	virtual void SetLinearVelocity(const Math::vector& v);
	/// get the body's linear velocity
	virtual Math::vector GetLinearVelocity() const;
	/// set the body's angular velocity
	virtual void SetAngularVelocity(const Math::vector& v);
	/// get the body's angular velocity
	Math::vector GetAngularVelocity() const;

	/// set the angular factor on the rigid body
	virtual void SetAngularFactor(const Math::vector& v);
	/// get the angular factor on the rigid body
	virtual Math::vector GetAngularFactor() const;

	virtual void SetMass(float);		
	/// reset the force and torque accumulators
	void Reset();

	void SetSleeping(bool sleeping);
	bool GetSleeping();	

	/// enable/disable the rigid body
	virtual void SetEnabled(bool b);
	
	/// apply a global impulse vector at the next time step at a global position
	virtual void ApplyImpulseAtPos(const Math::vector& impulse, const Math::point& pos, bool multByMass = false);

	/// Returns friction ratio of the physics body
	virtual float GetFriction();
	/// Sets friction ratio of the physics body
	virtual void SetFriction(float ratio);

	/// Returns restitution ratio of the physics body
	virtual float GetRestitution();
	/// Sets restitution ratio of the physics body
	virtual void SetRestitution(float ratio);


	/// called before simulation step is taken
	virtual void OnStepBefore(){}
	/// called after simulation step is taken
	virtual void OnStepAfter(){}
	/// called before simulation takes place
	virtual void OnFrameBefore();
	/// called after simulation takes place
	virtual void OnFrameAfter(){}

	/// enable/disable collision for connected bodies
	void SetConnectedCollision(bool b);
	/// get connected collision flag
	bool GetConnectedCollision() const;
	/// set the damping flag
	void SetDampingActive(bool active);
	/// get the damping flag
	bool GetDampingActive() const;
	/// set angular damping factor (0.0f..1.0f)
	void SetAngularDamping(float f);
	/// get angular damping factor (0.0f..1.0f)
	float GetAngularDamping() const;
	/// set linear damping factor
	void SetLinearDamping(float f);
	/// get linear damping factor
	float GetLinearDamping() const;

	/// set whether body is influenced by gravity
	void SetEnableGravity(bool enable);
	/// get whether body is influenced by gravity
	bool GetEnableGravity() const ;

	void SetKinematic(bool);

	virtual void SetMaterialType(Physics::MaterialType t);

	void SetCollideCategory(Physics::CollideCategory coll);
	void SetCollideFilter(uint mask);

	btRigidBody * GetBulletBody();


private:
	
	friend class NebulaMotionState;
	friend class Scene;	

	/// attach the rigid body to the world and initialize its position and mark it as trigger/kinematic/debris
	void Attach(Physics::BaseScene * world);
	/// detach the rigid body from the world
	void Detach();
	/// apply a material value from the common object.
	void ApplyMaterial();

	void UpdateTransform(const Math::matrix44 & trans);
	btDynamicsWorld * world;
	btRigidBody * body;    	
	NebulaMotionState * motion;	
	bool hasScale;
	Math::float4 scale;
};

inline 
btRigidBody *
BulletBody::GetBulletBody()
{
	return body;
}

}