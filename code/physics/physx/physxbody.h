#pragma once
//------------------------------------------------------------------------------
/**
    @class PhysX::PhysXBody
    
    
    (C) 2016-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "math/matrix44.h"
#include "math/vector.h"
#include "math/plane.h"
#include "physics/scene.h"
#include "util/fixedarray.h"
#include "physics/base/basebody.h"
#include "physics/materialtable.h"

namespace physx
{
	class PxRigidDynamic;
}

namespace PhysX
{

class Scene;
class Collider;
class PhysicsMesh;

class PhysXBody : public Physics::BaseRigidBody
{
	__DeclareClass(PhysXBody);
public:     

    /// constructor
	PhysXBody();
    /// destructor
    virtual ~PhysXBody();

	/// set transform
	void SetTransform(const Math::matrix44 & trans);

    /// set the body's linear velocity
	virtual void SetLinearVelocity(const Math::vector& v);
    /// get the body's linear velocity
	virtual Math::vector GetLinearVelocity() const;
    /// set the body's angular velocity
	virtual void SetAngularVelocity(const Math::vector& v);
    /// get the body's angular velocity
	virtual Math::vector GetAngularVelocity() const;

	/// set angular damping factor (0.0f..1.0f)
	void SetAngularDamping(float f);
	/// get angular damping factor (0.0f..1.0f)
	float GetAngularDamping() const;
	/// set linear damping factor
	void SetLinearDamping(float f);
	/// get linear damping factor
	float GetLinearDamping() const;


	/// set the angular factor on the rigid body
	virtual void SetAngularFactor(const Math::vector& v) {};
	/// get the angular factor on the rigid body
	virtual Math::vector GetAngularFactor() const { return Math::vector(0); }

	/// set the mass of the body
	virtual void SetMass(float);
    /// get the mass of the body
	virtual float GetMass() const;
	/// get the center of mass in local space
	virtual Math::vector GetCenterOfMassLocal();
	/// get the center of mass in world space
	virtual Math::vector GetCenterOfMassWorld();

    /// reset the force and torque accumulators
	void Reset();

    /// deactivate body
	void SetSleeping(bool sleeping);
    /// return body state
	bool GetSleeping();
	
	///
	void SetEnableGravity(bool enable);
	///
	bool GetEnableGravity() const;

	///
	void SetEnableCollisionCallback(bool enable);

    /// transform a global point into the body's local space
	Math::vector GlobalToLocalPoint(const Math::vector& p) const;
    /// transform a body-local point into global space
	Math::vector LocalToGlobalPoint(const Math::vector& p) const;

	/// apply a global impulse vector at the next time step at a global position
	void ApplyImpulseAtPos(const Math::vector& impulse, const Math::point& pos, bool multByMass = false);

    /// called before simulation step is taken
	void OnStepBefore(){}
    /// called after simulation step is taken
	void OnStepAfter(){}
    /// called before simulation takes place
	void OnFrameBefore();
    /// called after simulation takes place
	void OnFrameAfter(){}
    
    /// render the debug shapes
	virtual void RenderDebug();    

	///
	virtual void SetCollideCategory(Physics::CollideCategory coll);
	///
	unsigned int GetCollideCategory() const;
	///
	virtual void SetKinematic(bool);
	///
	bool GetKinematic();
    ///
	bool HasTransformChanged();	
	/// set material
	virtual void SetMaterialType(Physics::MaterialType t);

	///
	physx::PxRigidDynamic* GetBody();

protected:        		
    ///
	void SetupFromTemplate(const Physics::PhysicsCommon & templ);
    ///
	void Attach(Physics::BaseScene * world);
	///
	void Detach();
	
	physx::PxRigidDynamic *body;
	physx::PxScene *scene;
	Math::vector scale;
};


//------------------------------------------------------------------------------
/**

*/
inline
physx::PxRigidDynamic*
PhysXBody::GetBody()
{
	return this->body;
}

}; // namespace PhysX

//------------------------------------------------------------------------------
