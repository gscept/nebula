//------------------------------------------------------------------------------
//  @file scenequery.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "PxConfig.h"
#include "physics/scenequery.h"
#include "physics/actorcontext.h"
#include "physics/utils.h"

using namespace physx;

namespace Physics
{

namespace
{

//------------------------------------------------------------------------------
/**
    The query's own filter data is left empty on purpose. PhysX skips a shape when the bitwise
    AND of the query data and the shape's query filter data is zero, and a shape with the
    default group 0 has zero filter data, so a group mask expressed as filter data would skip
    exactly the shapes that are meant to match everything. The group, the mask and the excluded
    actor are carried on the callback instead, and the query runs with ePREFILTER so the
    callback is what decides.
*/
class QueryFilterCallback : public PxQueryFilterCallback
{
public:
    uint32_t group;
    uint32_t mask;
    uint32_t excludeActorId;

    PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override
    {
        // What the actor carries is the id this context allocated for it, which is the same thing the
        // filter names, so the comparison is against the id rather than the entity the actor belongs to.
        // A character controller shape has no actor, so the guard has to come before the read
        if (actor == nullptr)
            return PxQueryHitType::eBLOCK;

        if (this->excludeActorId != Ids::InvalidId32 && actor->userData == (void*)(uintptr_t)this->excludeActorId)
            return PxQueryHitType::eNONE;

        // word0 is the collision group the body carries, which is what the shape's filter data holds
        uint32_t const shapeGroup = shape->getQueryFilterData().word0;
        if (shapeGroup == 0 || this->group == 0)
            return PxQueryHitType::eBLOCK;

        if ((shapeGroup & this->mask) == 0)
            return PxQueryHitType::eNONE;

        return PxQueryHitType::eBLOCK;
    }

    PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit, const PxShape* shape, const PxRigidActor* actor) override
    {
        return PxQueryHitType::eBLOCK;
    }
};

//------------------------------------------------------------------------------
/**
*/
static void
FillHit(Physics::RaycastHit& out, const PxRaycastHit& hit)
{
    out.point.x = hit.position.x;
    out.point.y = hit.position.y;
    out.point.z = hit.position.z;

    out.normal.x = hit.normal.x;
    out.normal.y = hit.normal.y;
    out.normal.z = hit.normal.z;

    out.distance = hit.distance;
    out.faceIndex = hit.faceIndex;

    out.shapeId = 0xFFFFFFFF;
    out.entity = 0;

    if (hit.shape != nullptr)
    {
        ShapeId const shapeId((uint32_t)(uintptr_t)hit.shape->userData);
        if (ActorContext::IsValidShape(shapeId))
            out.shapeId = shapeId.id;
    }

    if (hit.actor != nullptr)
    {
        ActorId const actorId((uint32_t)(uintptr_t)hit.actor->userData);
        if (ActorContext::IsValid(actorId))
            out.entity = ActorContext::GetActor(actorId).userData;
    }
}

//------------------------------------------------------------------------------
/**
*/
static bool
QueryRay(IndexT scene, const Math::point& origin, const Math::vector& direction, float maxDistance, Physics::RaycastHit& out, const Physics::QueryFilter& filter)
{
    if (maxDistance <= 0.0f)
        return false;

    Physics::Scene& s = Physics::GetScene(scene);
    if (s.scene == nullptr)
        return false;

    QueryFilterCallback callback;
    callback.group = filter.group;
    callback.mask = filter.mask;
    callback.excludeActorId = filter.excludeActorId.id;

    PxQueryFilterData filterData;
    filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

    PxRaycastBuffer buffer;
    if (!s.scene->raycast(Neb2PxPnt(origin), Neb2PxVec(direction), maxDistance, buffer, PxHitFlag::eDEFAULT, filterData, &callback))
        return false;

    if (!buffer.hasBlock)
        return false;

    FillHit(out, buffer.block);
    return true;
}

} // anonymous namespace

//------------------------------------------------------------------------------
/**
*/
bool
Raycast(IndexT scene, const Math::point& origin, const Math::vector& direction, float maxDistance, Physics::RaycastHit& out, const Physics::QueryFilter& filter)
{
    return QueryRay(scene, origin, direction, maxDistance, out, filter);
}

//------------------------------------------------------------------------------
/**
*/
bool
SegmentCast(IndexT scene, const Math::point& from, const Math::point& to, Physics::RaycastHit& out, const Physics::QueryFilter& filter)
{
    Math::vector direction(to.x - from.x, to.y - from.y, to.z - from.z);

    float const length = Math::length(direction);
    if (length <= 0.0f)
        return false;

    return QueryRay(scene, from, Math::normalize(direction), length, out, filter);
}

} // namespace Physics
