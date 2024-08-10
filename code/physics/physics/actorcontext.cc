//------------------------------------------------------------------------------
//  actorcontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "PxConfig.h"
#include "physics/actorcontext.h"
#include "physics/physxstate.h"
#include "physics/utils.h"
#include "math/transform.h"

#define GET_ACTOR(i) ActorContext::actors[Ids::Index(i.id)]
#define GET_DYNAMIC(i) static_cast<PxRigidDynamic*>(ActorContext::actors[Ids::Index(i.id)].actor)

using namespace physx;

namespace Physics
{

Util::Array<Actor> ActorContext::actors;
Ids::IdGenerationPool ActorContext::actorPool;

Util::Array<Aggregate> AggregateContext::aggregates;
Ids::IdGenerationPool AggregateContext::aggPool;

//------------------------------------------------------------------------------
/**
*/
ActorId
ActorContext::AllocateActorId(PxRigidActor* pxActor)
{
    return ActorContext::AllocateActorId(pxActor, ActorResourceId::Invalid());
}

//------------------------------------------------------------------------------
/**
*/
ActorId
ActorContext::AllocateActorId(PxRigidActor* pxActor, ActorResourceId res)
{
    ActorId id;
    bool const allocated = ActorContext::actorPool.Allocate(id.id);
    Ids::Id24 idx = Ids::Index(id.id);
    if (allocated)
    {
        ActorContext::actors.Append(Physics::Actor());
        n_assert(idx == ActorContext::actors.Size() - 1);
    }

    Actor& actor = ActorContext::actors[idx];
    actor.id = id;
    actor.actor = pxActor;
    actor.res = res;
#pragma warning(push)
#pragma warning(disable: 4312)
    pxActor->userData = (void*)(uintptr_t)id.id;
#pragma warning(pop)
    return id;
}

//------------------------------------------------------------------------------
/**
*/
void 
ActorContext::DiscardActor(ActorId id)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    Actor& actor = GET_ACTOR(id);
    auto scene = actor.actor->getScene();
    if (scene)
    {
        scene->removeActor(*actor.actor);
    }
    state.DiscardActor(id);
    actor.actor->release();    
    ActorContext::actorPool.Deallocate(id.id);
}

//------------------------------------------------------------------------------
/**
*/
ActorId
ActorContext::CreateBox(Math::vector const& extends, IndexT materialId, ActorType type, Math::mat4 const & transform, IndexT sceneId)
{
    Scene & scene = Physics::GetScene(sceneId);

    PxRigidActor* newActor = state.CreateActor(type, transform);

    Material const & material = Physics::GetMaterial(materialId);
    PxShape * shape = PxRigidActorExt::createExclusiveShape(*newActor, PxBoxGeometry(Neb2PxVec(extends)), *material.material);
    if (type != ActorType::Static)
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
ActorContext::CreateSphere(float radius, IndexT materialId, ActorType type, Math::mat4 const & transform, IndexT sceneId)
{
    Scene & scene = Physics::GetScene(sceneId);

    PxRigidActor* newActor = state.CreateActor(type, transform);

    Material const & material = Physics::GetMaterial(materialId);
    PxShape * shape = PxRigidActorExt::createExclusiveShape(*newActor, PxSphereGeometry(radius), *material.material);
    if (type != ActorType::Static)
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
ActorContext::CreateCapsule(float radius, float halfHeight, IndexT materialId, ActorType type, Math::mat4 const & transform, IndexT sceneId)
{
    Scene & scene = Physics::GetScene(sceneId);

    PxRigidActor* newActor = state.CreateActor(type, transform);

    Material const & material = Physics::GetMaterial(materialId);
    PxShape * shape = PxRigidActorExt::createExclusiveShape(*newActor, PxCapsuleGeometry(radius, halfHeight), *material.material);
    if (type != ActorType::Static)
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

    PxPlane pxPlane(Neb2PxPnt(get_point(plane)), Neb2PxVec(get_normal(plane)));
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
void ActorContext::SetTransform(ActorId id, Math::mat4 const & transform)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    auto* actor = GET_DYNAMIC(id);
    if (actor->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
    {
        actor->setKinematicTarget(Neb2PxTrans(transform));
    }
    else
    {
        actor->setGlobalPose(Neb2PxTrans(transform));
    }
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
ActorContext::GetTransform(ActorId id)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    return Px2NebMat(GET_DYNAMIC(id)->getGlobalPose());
}

//------------------------------------------------------------------------------
/**
*/
void
ActorContext::SetPositionOrientation(ActorId id, Math::vec3 const& position, Math::quat const& orientation)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    auto* actor = GET_DYNAMIC(id);
    if (actor->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
    {
        actor->setKinematicTarget(Neb2PxTrans(position, orientation));
    }
    else
    {
        actor->setGlobalPose(Neb2PxTrans(position, orientation));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ActorContext::GetPositionOrientation(ActorId id, Math::vec3& pos, Math::quat& orientation)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    auto const pxTrans = GET_DYNAMIC(id)->getGlobalPose();
    pos = Px2NebVec(pxTrans.p);
    orientation = Px2NebQuat(pxTrans.q);
}

//------------------------------------------------------------------------------
/**
*/
void
ActorContext::SetLinearVelocity(ActorId id, Math::vector speed)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    GET_DYNAMIC(id)->setLinearVelocity(Neb2PxVec(speed));
}

//------------------------------------------------------------------------------
/**
*/
Math::vector 
ActorContext::GetLinearVelocity(ActorId id)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    return Px2NebVec(GET_DYNAMIC(id)->getLinearVelocity());
}

//------------------------------------------------------------------------------
/**
*/
void
ActorContext::SetAngularVelocity(ActorId id, Math::vector speed)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    GET_DYNAMIC(id)->setAngularVelocity(Neb2PxVec(speed));
}

//------------------------------------------------------------------------------
/**
*/
Math::vector
ActorContext::GetAngularVelocity(ActorId id)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    return Px2NebVec(GET_DYNAMIC(id)->getAngularVelocity());
}

//------------------------------------------------------------------------------
/**
*/
void
ActorContext::ApplyImpulseAtPos(ActorId id, const Math::vector& impulse, const Math::point& pos)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    PxRigidDynamic* actor = GET_DYNAMIC(id);    
    PxRigidBodyExt::addForceAtPos(*actor, Neb2PxVec(impulse), Neb2PxPnt(pos));
}

//------------------------------------------------------------------------------
/**
*/
physx::PxRigidActor * 
ActorContext::GetPxActor(ActorId id)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    return static_cast<physx::PxRigidActor*>(GET_ACTOR(id).actor);
}

//------------------------------------------------------------------------------
/**
*/
physx::PxRigidDynamic* 
ActorContext::GetPxDynamic(ActorId id)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    return GET_DYNAMIC(id);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ActorContext::GetShapeCount(ActorId id)
{
    if (id.id != Ids::InvalidId32)
    {
        physx::PxRigidActor* actor = GetPxActor(id);
        if (actor != nullptr)
        {
            return actor->getNbShapes();
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
void ActorContext::GetShapes(ActorId id, ShapeArrayType& shapes)
{
    if (id.id != Ids::InvalidId32)
    {
        physx::PxRigidActor* actor = GetPxActor(id);
        if (actor != nullptr)
        {
            auto count = actor->getNbShapes();

            Util::StackArray<physx::PxShape*, DefaultShapeAlloc> buffer;
            PxShape** shapeBuffer = buffer.EmplaceArray(count);
            ShapeHandle* handles = shapes.EmplaceArray(count);
            actor->getShapes(shapeBuffer, count);
            for (int i = 0; i < count; ++i)
            {
                handles[i].shape = shapeBuffer[i];
            }
        }
    }
}
Math::transform ActorContext::GetShapeTransform(const ShapeHandle& shape)
{
    if (shape.IsValid())
    {
        return Px2NebTrans(shape.shape->getLocalPose());
    }
    return Math::transform();
}

void ActorContext::SetShapeTransform(const ShapeHandle& shape, const Math::transform& transform)
{
    if (shape.IsValid())
    {
        shape.shape->setLocalPose(Neb2PxTrans(transform));
    }
}

//------------------------------------------------------------------------------
/**
*/
AggregateId
AggregateContext::AllocateAggregateId(AggregateResourceId res)
{
    AggregateId id;
    bool const allocated = AggregateContext::aggPool.Allocate(id.id);
    Ids::Id24 idx = Ids::Index(id.id);
    if (allocated)
    {
        AggregateContext::aggregates.Append(Physics::Aggregate());
        n_assert(idx == AggregateContext::aggregates.Size() - 1);
    }

    Aggregate& aggregate = AggregateContext::aggregates[idx];
    aggregate.id = id;
    aggregate.res = res;
    return id;
}

//------------------------------------------------------------------------------
/**
*/
Aggregate&
AggregateContext::GetAggregate(AggregateId id)
{
    n_assert(AggregateContext::aggPool.IsValid(id.id));
    return AggregateContext::aggregates[Ids::Index(id.id)];
}


//------------------------------------------------------------------------------
/**
*/
void
AggregateContext::DiscardAggregate(AggregateId id)
{
    n_assert(AggregateContext::aggPool.IsValid(id.id));
    Aggregate& aggregate = AggregateContext::GetAggregate(id);
    for (ActorId actorId : aggregate.actors)
    {
        Actor& actor = ActorContext::GetActor(actorId);
        auto scene = actor.actor->getScene();
        if (scene)
        {
            scene->removeActor(*actor.actor);
        }
        state.DiscardActor(actorId);
        actor.actor->release();
    }
        
    AggregateContext::aggPool.Deallocate(id.id);
}





}
