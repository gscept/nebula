#include "foundation/stdneb.h"
#include "physics/actorcontext.h"
#include "physics/physxstate.h"
#include "physics/utils.h"

#define GET_ACTOR(i) ActorContext::actors[Ids::Index(i.id)]
#define GET_DYNAMIC(i) static_cast<PxRigidDynamic*>(ActorContext::actors[Ids::Index(i.id)].actor)

using namespace physx;

namespace Physics
{

Util::Array<Actor> ActorContext::actors;
Ids::IdGenerationPool ActorContext::actorPool;

//------------------------------------------------------------------------------
/**
*/
ActorId
ActorContext::AllocateActorId(PxRigidActor* pxActor)
{
    ActorId id;
    bool reused = ActorContext::actorPool.Allocate(id.id);
    Ids::Id24 idx = Ids::Index(id.id);
    if (!reused)
    {
        ActorContext::actors.Append(Physics::Actor());
        n_assert(idx == ActorContext::actors.Size() - 1);
    }

    Actor& actor = ActorContext::actors[idx];
    actor.id = id;
    actor.actor = pxActor;
#pragma warning(push)
#pragma warning(disable: 4312)
    pxActor->userData = (void*)id.id;
#pragma warning(pop)
    return id;
}

//------------------------------------------------------------------------------
/**
*/
ActorId
ActorContext::CreateBox(Math::vector const& extends, IndexT materialId, bool dynamic, Math::matrix44 const & transform, IndexT sceneId)
{
    Scene & scene = Physics::GetScene(sceneId);

    PxRigidActor* newActor = state.CreateActor(dynamic, transform);

    Material const & material = Physics::GetMaterial(materialId);
    PxShape * shape = PxRigidActorExt::createExclusiveShape(*newActor, PxBoxGeometry(Neb2PxVec(extends)), *material.material);
    if (dynamic)
    {
        PxRigidBodyExt::updateMassAndInertia(*static_cast<PxRigidDynamic*>(newActor), material.density);
    }
    scene.scene->addActor(*newActor);
    return AllocateActorId(newActor);
}


//------------------------------------------------------------------------------
/**
*/
ActorId
ActorContext::CreateSphere(float radius, IndexT materialId, bool dynamic, Math::matrix44 const & transform, IndexT sceneId)
{
    Scene & scene = Physics::GetScene(sceneId);

    PxRigidActor* newActor = state.CreateActor(dynamic, transform);

    Material const & material = Physics::GetMaterial(materialId);
    PxShape * shape = PxRigidActorExt::createExclusiveShape(*newActor, PxSphereGeometry(radius), *material.material);
    if (dynamic)
    {
        PxRigidBodyExt::updateMassAndInertia(*static_cast<PxRigidDynamic*>(newActor), material.density);
    }
    scene.scene->addActor(*newActor);
    return AllocateActorId(newActor);
}


//------------------------------------------------------------------------------
/**
*/
ActorId
ActorContext::CreateCapsule(float radius, float halfHeight, IndexT materialId, bool dynamic, Math::matrix44 const & transform, IndexT sceneId)
{
    Scene & scene = Physics::GetScene(sceneId);

    PxRigidActor* newActor = state.CreateActor(dynamic, transform);

    Material const & material = Physics::GetMaterial(materialId);
    PxShape * shape = PxRigidActorExt::createExclusiveShape(*newActor, PxCapsuleGeometry(radius, halfHeight), *material.material);
    if (dynamic)
    {
        PxRigidBodyExt::updateMassAndInertia(*static_cast<PxRigidDynamic*>(newActor), material.density);
    }
    scene.scene->addActor(*newActor);
    return AllocateActorId(newActor);
}

//------------------------------------------------------------------------------
/**
*/
ActorId
ActorContext::CreatePlane(Math::plane const& plane, IndexT materialId, IndexT sceneId)
{
    Scene & scene = Physics::GetScene(sceneId);

    PxPlane pxPlane(Neb2PxVec(plane.get_point()), Neb2PxVec(plane.get_normal()));
    PxTransform transform = PxTransformFromPlaneEquation(pxPlane);

    PxRigidActor* newActor = scene.physics->createRigidStatic(transform);

    Material const & material = Physics::GetMaterial(materialId);

    PxShape * shape = PxRigidActorExt::createExclusiveShape(*newActor, PxPlaneGeometry(), *material.material);

    scene.scene->addActor(*newActor);
    return AllocateActorId(newActor);
}


//------------------------------------------------------------------------------
/**
*/
Actor&
ActorContext::GetActor(ActorId id)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    return ActorContext::actors[Ids::Index(id.id)];
}

//------------------------------------------------------------------------------
/**
*/
void ActorContext::SetTransform(ActorId id, Math::matrix44 const & transform)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    GET_DYNAMIC(id)->setGlobalPose(Neb2PxTrans(transform));
}

Math::matrix44 ActorContext::GetTransform(ActorId id)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    return Px2NebMat(GET_DYNAMIC(id)->getGlobalPose());
}

physx::PxRigidActor * ActorContext::GetPxActor(ActorId id)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    return static_cast<physx::PxRigidActor*>(GET_ACTOR(id).actor);
}

}
