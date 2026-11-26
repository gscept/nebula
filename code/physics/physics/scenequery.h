#pragma once
//------------------------------------------------------------------------------
/**
    Scene queries against a physics scene.

    PhysX only allows scene queries while the scene is not simulating, so these run outside
    the step of the physics frame event.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "physicsinterface.h"

namespace Physics
{

//------------------------------------------------------------------------------
/**
*/
struct QueryFilter
{
    uint32_t group = 0;
    uint32_t mask = 0xFFFFFFFF;
    Physics::ActorId excludeActorId;
};

//------------------------------------------------------------------------------
/**
    A query hit.

    @note The field order is the layout the C# mirror is generated from, so both sides have to be synced.
    The order puts the 16 byte math values after the four byte ones, which is what makes the padding the
    same on both sides.
*/
struct RaycastHit
{
    uint64_t entity; // game entity id
    uint32_t shapeId;
    float distance;
    Math::vec3 point;
    Math::vec3 normal;
    uint32_t faceIndex;
};

/// Closest hit along a ray. The direction has to be normalized. False when nothing was hit
bool Raycast(IndexT scene, const Math::point& origin, const Math::vector& direction, float maxDistance, Physics::RaycastHit& out, const Physics::QueryFilter& filter);

/// Closest hit along a segment. The direction is the segment itself.
bool SegmentCast(IndexT scene, const Math::point& from, const Math::point& to, Physics::RaycastHit& out, const Physics::QueryFilter& filter);

} // namespace Physics
