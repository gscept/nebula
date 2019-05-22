//------------------------------------------------------------------------------
//  actor.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "PxPhysicsAPI.h"
#include "physicsinterface.h"
#include "physics/actor.h"
#include "physics/utils.h"

using namespace physx;
using namespace Physics;

namespace Physics
{
    
//------------------------------------------------------------------------------
/**
*/
static inline 
PxRigidActor* CreateActor(PxPhysics* physics, bool dynamic, Math::matrix44 const & transform)
{
    if (dynamic)
    {
        return physics->createRigidDynamic(Neb2PxTrans(transform));
    }
    else
    {
        return physics->createRigidStatic(Neb2PxTrans(transform));
    }
}
//------------------------------------------------------------------------------
/**
*/
Actor* 
Actor::CreateBox(Math::vector const& extends, IndexT materialId, bool dynamic, Math::matrix44 const & transform, IndexT sceneId)
{
    Scene & scene = Physics::GetScene(sceneId);        

    PxRigidActor* newActor = CreateActor(scene.physics, dynamic, transform);

    Material const & material = Physics::GetMaterial(materialId);
    PxShape * shape = PxRigidActorExt::createExclusiveShape(*newActor, PxBoxGeometry(Neb2PxVec(extends)), *material.material);
    if (dynamic)
    {
        PxRigidBodyExt::updateMassAndInertia(* static_cast<PxRigidDynamic*>(newActor), material.density);
    }
    scene.scene->addActor(*newActor);
}


//------------------------------------------------------------------------------
/**
*/
Actor*
Actor::CreateSphere(float radius, IndexT material, bool dynamic, Math::matrix44 const & transform, IndexT scene)
{

}



}

