#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::BaseRagdollJoint
    
    Base class for ragdoll joints, can use motors to rotate the bodies to a 
	specific target orientation.
    
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "physics/joint.h"

//------------------------------------------------------------------------------
namespace Physics
{
class BaseRagdollJoint : public Joint
{
	__DeclareAbstractClass(BaseRagdollJoint);
public:
	/// constructor
	BaseRagdollJoint();
	/// destructor
	virtual ~BaseRagdollJoint();

	/// set up the joint
	virtual void Setup(bool worldSpacePivot, const Ptr<PhysicsBody>& bodyA, const Ptr<PhysicsBody>& bodyB, const Math::vector& pivot, const Math::vector& twistAxis, const Math::vector& planeAxis) {n_error("Not Implemented"); };
	/// set up the motor
	virtual void SetupMotors();

	enum MotorSelection
	{
		Twist = 0,
		Plane = 1,
		Cone = 2,
		All = 3
	};

	/// attach to scene
	virtual void Attach(Physics::BaseScene * world) {n_error("Not Implemented"); };
	/// detach from scene
	virtual void Detach() {n_error("Not Implemented"); };

	virtual float GetConeAngularLimit() const			{n_error("Not Implemented"); return 0;};
	virtual void  SetConeAngularLimit(float limit)		{n_error("Not Implemented"); };
	virtual float GetPlaneMinAngularLimit() const		{n_error("Not Implemented"); return 0;};
	virtual void  SetPlaneMinAngularLimit(float limit)	{n_error("Not Implemented"); };
	virtual float GetPlaneMaxAngularLimit() const		{n_error("Not Implemented"); return 0;};
	virtual void  SetPlaneMaxAngularLimit(float limit)	{n_error("Not Implemented"); };
	virtual float GetTwistMinAngularLimit() const		{n_error("Not Implemented"); return 0;};
	virtual void  SetTwistMinAngularLimit(float limit)	{n_error("Not Implemented"); };
	virtual float GetTwistMaxAngularLimit() const		{n_error("Not Implemented"); return 0;};
	virtual void  SetTwistMaxAngularLimit(float limit)	{n_error("Not Implemented"); };

	virtual float GetMotorStiffness(MotorSelection s) const	/*< tau */						{n_error("Not implemented"); return 0;};
	virtual void  SetMotorStiffness(float stiffness, MotorSelection s)						{n_error("Not implemented"); };
	virtual float GetMotorDamping(MotorSelection s) const									{n_error("Not implemented"); return 0;};
	virtual void  SetMotorDamping(float damping,MotorSelection s)							{n_error("Not implemented"); };
	virtual float GetMotorMaxForce(MotorSelection s) const									{n_error("Not implemented"); return 0;};
	virtual void  SetMotorMaxForce(float maxforce, MotorSelection s)						{n_error("Not implemented"); };
	virtual float GetMotorMinForce(MotorSelection s) const									{n_error("Not implemented"); return 0;};
	virtual void  SetMotorMinForce(float minforce, MotorSelection s)						{n_error("Not implemented"); };
	/// set min and max value to -absValue and absValue respectively
	virtual void  SetMotorMinMaxForce(float absValue, MotorSelection s)						{n_error("Not implemented"); };
	virtual float GetMotorConstantRecoverlyVelocity(MotorSelection s) const					{n_error("Not implemented"); return 0;};
	virtual void  SetMotorConstantRecoverlyVelocity(float vel, MotorSelection s)			{n_error("Not implemented"); };
	virtual void  SetTargetRotation(const Math::quaternion& rot)			{n_error("Not implemented"); };
	virtual void  SetTargetRelativeRotation(const Math::quaternion& rot)	{n_error("Not implemented"); };

protected:

	bool initialized;
	bool motorInitialized;
}; 
} 
// namespace Physics
//------------------------------------------------------------------------------