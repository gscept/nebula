#pragma once
//------------------------------------------------------------------------------
/**
    @class PhysX::PhysXJoint

    A joint (also known as constraint) connects two Actors    

    (C) 2016-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "physics/base/basejoint.h"

namespace physx
{
	class PxJoint;
	class PxFixedJoint;
	class PxDistanceJoint;
	class PxRevoluteJoint;
	class PxSphericalJoint;
	class PxD6Joint;
	class PxPrismaticJoint;
}

union PhysXJointType
{
	physx::PxJoint* joint;
	physx::PxFixedJoint* fixed;
	physx::PxDistanceJoint* distance;
	physx::PxRevoluteJoint* revolute;
	physx::PxSphericalJoint* spherical;
	physx::PxD6Joint* universal;
	physx::PxPrismaticJoint* prismatic;
};


//------------------------------------------------------------------------------
namespace PhysX
{

class PhysXJoint : public Physics::BaseJoint
{
    __DeclareClass(PhysXJoint);
public:
    

    /// default constructor
	PhysXJoint();
	///
	~PhysXJoint();
       
    /// update position and orientation
    virtual void UpdateTransform(const Math::matrix44& m){}
    /// render debug visualization
	virtual void RenderDebug();

	/// attach to scene
	virtual void Attach(Physics::BaseScene * world);
	/// detach from scene
	virtual void Detach();
	/// set enable
	virtual void SetEnabled(bool b){}

	///
	virtual void SetBreakThreshold(float threshold){}
	///
	virtual float GetBreakThreshold() { return -1.0f; }

	///
	virtual void SetERP(float ERP, int axis = 0) {}
	///
	virtual void SetCFM(float CFM, int axis = 0) {}
	///
	virtual void SetStoppingERP(float ERP, int axis = 0) {}
	///
	virtual void SetStoppingCFM(float CFM, int axis = 0) {}

	///
	virtual float GetERP(int axis = 0) { return -1.0f; }
	///
	virtual float GetCFM(int axis = 0) { return -1.0f; }

	///
	virtual float GetStoppingERP(int axis = 0) { return -1.0f; }
	///
	virtual float GetStoppingCFM(int axis = 0) { return -1.0f; }

	/// 
	virtual physx::PxJoint * GetPxJoint();

protected:
	PhysXJointType joint;	
};


//------------------------------------------------------------------------------
/**

*/
inline
physx::PxJoint * 
PhysXJoint::GetPxJoint()
{
	return this->joint.joint;
}


}; // namespace PhysX
//------------------------------------------------------------------------------
