#pragma once
//------------------------------------------------------------------------------
/**
	@class Bullet::BulletJoint

	(C) 2012-2018 Individual contributors, see AUTHORS file
*/
#include "physics/base/basejoint.h"
#include "physics/base/basescene.h"

class btRigidBody;
class btDynamicsWorld;
class btTypedConstraint;

namespace Bullet
{
class Scene;
class BulletScene;
class Collider;
class BulletCollider;
class BulletBody;
class NebulaMotionState;

class BulletJoint : public Physics::BaseJoint
{
	__DeclareClass(BulletJoint);

public:
	/// default constructor
	BulletJoint();
	/// destructor
	virtual ~BulletJoint();
	/// initialize the joint (attach to world)
	virtual void Attach(Physics::BaseScene * world);
	/// uninitialize the joint (detach from world)
	virtual void Detach();		
	/// update position and orientation
	virtual void UpdateTransform(const Math::matrix44& m);
	/// render debug visualization
	virtual void RenderDebug();

	/// enable joint
	void SetEnabled(bool b);

	/// is enabled
	bool IsEnabled() const;

	/// setup joint object from description data
	bool Setup(const Physics::JointDescription & desc);

	/// set breaking threshhold
	void SetBreakThreshold(float threshold);
	/// get breaking threshold
	float GetBreakThreshold();

	///
	void SetERP(float ERP, int axis = 0 );
	///
	void SetCFM(float CFM, int axis = 0);

	///
	void SetStoppingERP(float ERP, int axis = 0);
	///
	void SetStoppingCFM(float CFM, int axis = 0);

	///
	float GetERP(int axis = 0);
	///
	float GetCFM(int axis = 0);

	///
	float GetStoppingERP(int axis = 0);
	///
	float GetStoppingCFM(int axis = 0);


protected:		
		
	btTypedConstraint *constraint;
			
	friend Physics::PhysicsBody;
};

}