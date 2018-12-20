#pragma once
//------------------------------------------------------------------------------
/**
    @class PhysX::PhysXCollider
    
    
    (C) 2016-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "math/bbox.h"
#include "physics/model/templates.h"
#include "physics/base/basecollider.h"
#include "foundation/PxTransform.h"

namespace physx
{
	class PxRigidActor;
    class PxMaterial;
}

namespace Physics
{
	class ManagedPhysicsMesh;
}

namespace PhysX
{
class PhysXCollider : public Physics::BaseCollider
{
    __DeclareClass(PhysXCollider);
public:
	   
    /// default constructor
	PhysXCollider();
    /// destructor
    virtual ~PhysXCollider();
    /// render debug visualization
	virtual void RenderDebug(const Math::matrix44& t);

    /// 
    void CreateInstance(physx::PxRigidActor * target, Math::vector & scale, const physx::PxMaterial& material);    
protected:    

    ///
    void CreateSphere(physx::PxRigidActor * target, const Physics::ColliderDescription & desc, const Math::float4 &scale, const Math::float4 &trans, const physx::PxMaterial& material);
    ///
    void CreateCapsule(physx::PxRigidActor * target, const Physics::ColliderDescription & desc, const Math::float4 &scale, const Math::quaternion & quat, const Math::float4 &trans, const physx::PxMaterial& material);
    ///
    void CreateBox(physx::PxRigidActor * target, const Physics::ColliderDescription & desc, const Math::float4 &scale, const Math::quaternion & quat, const Math::float4 &trans, const physx::PxMaterial& material);
    ///
    void CreatePlane(physx::PxRigidActor * target, const Physics::ColliderDescription & desc, const Math::float4 &scale, const Math::quaternion & quat, const Math::float4 &trans, const physx::PxMaterial& material);
    //
    void CreatePhysicsMesh(physx::PxRigidActor * target, const Physics::ColliderDescription & desc, const Math::float4 &scale, const Math::quaternion & quat, const Math::float4 &trans, const physx::PxMaterial& material);
};

}; // namespace PhysX


    