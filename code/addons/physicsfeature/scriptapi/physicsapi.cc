//------------------------------------------------------------------------------
//  @file physicsapi.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "PxConfig.h"
#include "extensions/PxRigidBodyExt.h"
#include "physicsapi.h"
#include "physicsfeature/physicsfeatureunit.h"
#include "physics/actorcontext.h"
#include "physics/charactercontext.h"
#include "physics/scenequery.h"
#include "physics/utils.h"
#include "game/world.h"

using namespace physx;

namespace PhysicsFeature
{
namespace ScriptApi
{

// The managed mirror and the physics module both describe a query hit. The layout is the contract
// between them, so this is checked at compile time rather than left to a test
static_assert(sizeof(PhysicsFeature::RaycastHit) == sizeof(Physics::RaycastHit), "RaycastHit layout mismatch between the physics module and the managed mirror");
static_assert(sizeof(PhysicsFeature::QueryFilter) == sizeof(Physics::QueryFilter), "QueryFilter layout mismatch between the physics module and the managed mirror");

//------------------------------------------------------------------------------
/**
    An actor id a script hands in is not one this side resolved, so it has to be checked against the pool
    before it is used. A script can hold an id across a rebuild, and the entity the id named can be gone by
    the time the call comes back
*/
static Physics::ActorId
ResolveActor(uint32_t actorId)
{
    Physics::ActorId const handle(actorId);
    if (!Physics::ActorContext::IsValid(handle))
        return Physics::ActorId(Ids::InvalidId32);

    return handle;
}

//------------------------------------------------------------------------------
/**
    The dynamic body of an actor. Velocity and the sleep state only exist on a dynamic body, so a static or
    kinematic one answers with null and the call does nothing
*/
static PxRigidDynamic*
ResolveDynamic(Physics::ActorId id)
{
    if (id.id == Ids::InvalidId32)
        return nullptr;

    PxRigidActor* actor = Physics::ActorContext::GetPxActor(id);
    return actor != nullptr ? actor->is<PxRigidDynamic>() : nullptr;
}

//------------------------------------------------------------------------------
/**
*/
bool
Physics_IsActorValid(uint32_t actorId)
{
    return Physics::ActorContext::IsValid(ResolveActor(actorId));
}

//------------------------------------------------------------------------------
/**
    The mass the body actually has. The Rigidbody component's mass is 0 when it is derived from the shape
    volumes and the material densities, so reading the component is not the same as reading the body
*/
float
Physics_GetMass(uint32_t actorId)
{
    Physics::ActorId const id = ResolveActor(actorId);
    if (id.id == Ids::InvalidId32)
        return 0.0f;

    PxRigidBody* body = Physics::ActorContext::GetPxActor(id)->is<PxRigidBody>();
    return body != nullptr ? body->getMass() : 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
bool
Physics_IsSleeping(uint32_t actorId)
{
    PxRigidDynamic* body = ResolveDynamic(ResolveActor(actorId));
    return body == nullptr || body->isSleeping();
}

//------------------------------------------------------------------------------
/**
*/
void
Physics_WakeUp(uint32_t actorId)
{
    PxRigidDynamic* body = ResolveDynamic(ResolveActor(actorId));
    if (body != nullptr)
        body->wakeUp();
}

//------------------------------------------------------------------------------
/**
*/
void
Physics_Sleep(uint32_t actorId)
{
    PxRigidDynamic* body = ResolveDynamic(ResolveActor(actorId));
    if (body != nullptr)
        body->putToSleep();
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
Physics_GetVelocity(uint32_t actorId)
{
    PxRigidDynamic* body = ResolveDynamic(ResolveActor(actorId));
    if (body == nullptr)
        return Math::vec3(0, 0, 0);

    return Px2NebVec(body->getLinearVelocity());
}

//------------------------------------------------------------------------------
/**
*/
void
Physics_SetVelocity(uint32_t actorId, Math::vec3 velocity)
{
    PxRigidDynamic* body = ResolveDynamic(ResolveActor(actorId));
    if (body != nullptr)
        body->setLinearVelocity(Neb2PxVec(velocity));
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
Physics_GetAngularVelocity(uint32_t actorId)
{
    PxRigidDynamic* body = ResolveDynamic(ResolveActor(actorId));
    if (body == nullptr)
        return Math::vec3(0, 0, 0);

    return Px2NebVec(body->getAngularVelocity());
}

//------------------------------------------------------------------------------
/**
*/
void
Physics_SetAngularVelocity(uint32_t actorId, Math::vec3 angularVelocity)
{
    PxRigidDynamic* body = ResolveDynamic(ResolveActor(actorId));
    if (body != nullptr)
        body->setAngularVelocity(Neb2PxVec(angularVelocity));
}

//------------------------------------------------------------------------------
/**
    Newtons. The mode is what makes a force and an impulse different quantities here: a force is integrated
    by the step, an impulse is the velocity change the step applies to the body
*/
void
Physics_AddForce(uint32_t actorId, Math::vec3 force)
{
    PxRigidBody* body = ResolveDynamic(ResolveActor(actorId));
    if (body != nullptr)
        body->addForce(Neb2PxVec(force), PxForceMode::eFORCE);
}

//------------------------------------------------------------------------------
/**
*/
void
Physics_AddImpulse(uint32_t actorId, Math::vec3 impulse)
{
    PxRigidBody* body = ResolveDynamic(ResolveActor(actorId));
    if (body != nullptr)
        body->addForce(Neb2PxVec(impulse), PxForceMode::eIMPULSE);
}

//------------------------------------------------------------------------------
/**
    An impulse at a point also gives the body angular velocity, which is why this goes through the extension
    rather than the body
*/
void
Physics_AddImpulseAtPoint(uint32_t actorId, Math::vec3 impulse, Math::vec3 point)
{
    PxRigidBody* body = ResolveDynamic(ResolveActor(actorId));
    if (body != nullptr)
        PxRigidBodyExt::addForceAtPos(*body, Neb2PxVec(impulse), Neb2PxVec(point), PxForceMode::eIMPULSE);
}

//------------------------------------------------------------------------------
/**
*/
void
Physics_AddTorque(uint32_t actorId, Math::vec3 torque)
{
    PxRigidBody* body = ResolveDynamic(ResolveActor(actorId));
    if (body != nullptr)
        body->addTorque(Neb2PxVec(torque), PxForceMode::eFORCE);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
Physics_GetSceneForWorld(uint32_t worldId)
{
    if (!PhysicsFeatureUnit::HasInstance())
        return 0xFFFFFFFF;

    Game::World* world = Game::GetWorld(worldId);
    if (world == nullptr)
        return 0xFFFFFFFF;

    return (uint32_t)PhysicsFeatureUnit::Instance()->SceneForWorld(world);
}

//------------------------------------------------------------------------------
/**
    Write a hit into the buffer the managed side owns.

    The fields are assigned one at a time rather than by copying the struct, because the two sides only share
    the field order, not the types. The managed mirror has Vector3 where this side has the 16 byte aligned
    math types, and a managed buffer has no alignment guarantee.
*/
static void
CopyHit(const Physics::RaycastHit& hit, PhysicsFeature::RaycastHit* outHit)
{
    outHit->entity = hit.entity;
    outHit->shapeId = hit.shapeId;
    outHit->distance = hit.distance;
    outHit->faceIndex = hit.faceIndex;

    outHit->point.x = hit.point.x;
    outHit->point.y = hit.point.y;
    outHit->point.z = hit.point.z;

    outHit->normal.x = hit.normal.x;
    outHit->normal.y = hit.normal.y;
    outHit->normal.z = hit.normal.z;
}

//------------------------------------------------------------------------------
/**
    The filter arrives as a pointer because a blittable managed struct passed with ref is a pointer on this
    side. The excluded actor is named by an id the managed side holds, so it has to be resolved against the
    pool before the query uses it
*/
static Physics::QueryFilter
ResolveFilter(const PhysicsFeature::QueryFilter* filter)
{
    Physics::QueryFilter resolved;
    resolved.group = filter->group;
    resolved.mask = filter->mask;
    resolved.excludeActorId = ResolveActor(filter->excludeActorId);
    return resolved;
}

//------------------------------------------------------------------------------
/**
*/
bool
Physics_Raycast(uint32_t scene, Math::point origin, Math::vector direction, float maxDistance, PhysicsFeature::RaycastHit* outHit, const PhysicsFeature::QueryFilter* filter)
{
    if (outHit == nullptr || filter == nullptr || scene == 0xFFFFFFFF)
        return false;

    Physics::RaycastHit hit;
    if (!Physics::Raycast((IndexT)scene, origin, direction, maxDistance, hit, ResolveFilter(filter)))
        return false;

    CopyHit(hit, outHit);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
Physics_SegmentCast(uint32_t scene, Math::point from, Math::point to, PhysicsFeature::RaycastHit* outHit, const PhysicsFeature::QueryFilter* filter)
{
    if (outHit == nullptr || filter == nullptr || scene == 0xFFFFFFFF)
        return false;

    Physics::RaycastHit hit;
    if (!Physics::SegmentCast((IndexT)scene, from, to, hit, ResolveFilter(filter)))
        return false;

    CopyHit(hit, outHit);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
static Physics::CharacterId
ResolveCharacter(uint32_t characterId)
{
    Physics::CharacterId const handle(characterId);
    if (!Physics::CharacterContext::IsValid(handle))
        return Physics::CharacterId(Ids::InvalidId32);

    return handle;
}

//------------------------------------------------------------------------------
/**
*/
bool
Physics_IsCharacterValid(uint32_t characterId)
{
    return ResolveCharacter(characterId).id != Ids::InvalidId32;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
Physics_GetCharacterFootPosition(uint32_t characterId)
{
    Physics::CharacterId const id = ResolveCharacter(characterId);
    if (id.id == Ids::InvalidId32)
        return Math::vec3(0.0f);

    return Physics::CharacterContext::GetCharacterFootPosition(id);
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
Physics_GetCharacterCenterPosition(uint32_t characterId)
{
    Physics::CharacterId const id = ResolveCharacter(characterId);
    if (id.id == Ids::InvalidId32)
        return Math::vec3(0.0f);

    return Physics::CharacterContext::GetCharacterCenterPosition(id);
}

//------------------------------------------------------------------------------
/**
    The collision bits as a plain number, because the managed side has no mirror of the engine's bitfield.
    A character which is not in the pool answers with zero, which is no collision.
*/
uint32_t
Physics_MoveCharacter(uint32_t characterId, Math::vec3 displacement, float minDist, float timeSinceLastMove)
{
    Physics::CharacterId const id = ResolveCharacter(characterId);
    if (id.id == Ids::InvalidId32)
        return 0;

    Physics::CharacterCollision collision = Physics::CharacterContext::MoveCharacter(id, displacement, minDist, timeSinceLastMove);

    uint32_t bits = 0;
    if (collision.IsSet(Physics::CharacterCollisionBits::Sides))
        bits |= 1 << 0;
    if (collision.IsSet(Physics::CharacterCollisionBits::Up))
        bits |= 1 << 1;
    if (collision.IsSet(Physics::CharacterCollisionBits::Down))
        bits |= 1 << 2;

    return bits;
}

//------------------------------------------------------------------------------
/**
    A teleport, which is what PhysX documents this as: it does not check for collisions. The position is
    the foot, which is the same thing the entity's Position component holds
*/
void
Physics_SetCharacterPosition(uint32_t characterId, Math::vec3 footPosition)
{
    Physics::CharacterId const id = ResolveCharacter(characterId);
    if (id.id == Ids::InvalidId32)
        return;

    Physics::CharacterContext::SetCharacterPosition(id, footPosition);
}

//------------------------------------------------------------------------------
/**
*/
void
Physics_SetCharacterHeight(uint32_t characterId, float newHeight)
{
    Physics::CharacterId const id = ResolveCharacter(characterId);
    if (id.id == Ids::InvalidId32)
        return;

    Physics::CharacterContext::SetCharacterHeight(id, newHeight);
}

} // namespace ScriptApi
} // namespace PhysicsFeature
