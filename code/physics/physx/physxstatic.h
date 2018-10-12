#pragma once
//------------------------------------------------------------------------------
/**
    @class PhysX::PhysXStatic

    (C) 2016-2018 Individual contributors, see AUTHORS file
*/
#include "physics/base/basestatic.h"
#include "physics/collider.h"
#include "math/matrix44.h"
#include "physics/materialtable.h"

namespace physx
{
    class PxRigidStatic;
    class PxScene;
}

namespace PhysX
{


class PhysXStatic : public Physics::BaseStatic
{
__DeclareClass(PhysXStatic);

public:
    ///
    PhysXStatic();
    ///
    ~PhysXStatic();
       

	/// set transform
	void SetTransform(const Math::matrix44 & trans);
	///
	virtual void SetCollideCategory(Physics::CollideCategory coll);
	/// set material
	virtual void SetMaterialType(Physics::MaterialType t);
	
protected:	
	friend class PhysicsObject;
    ///
	virtual void SetupFromTemplate(const Physics::PhysicsCommon & templ);
    ///
    void Attach(Physics::BaseScene * world);
    ///
    void Detach();

    physx::PxRigidStatic * body;
    physx::PxScene *scene;
};
}