#pragma once
//------------------------------------------------------------------------------
/**
    PhysicsFeature::ScriptApi

    The script facing side of the physics feature.

    This is compiled as an object library. The app links nsharp with -Wl,--exclude-libs,ALL, so a
    NEBULA_EXPORT symbol inside this module's archive does not reach the executable's dynamic
    symbol table, and [DllImport("__Internal")] cannot resolve it.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "math/vec3.h"
#include "nsharp/nsconfig.h"
#include "components/physicsfeature.h"

namespace PhysicsFeature
{
namespace ScriptApi
{

NEBULA_EXPORT bool Physics_IsActorValid(uint32_t actorId);
NEBULA_EXPORT float Physics_GetMass(uint32_t actorId);
NEBULA_EXPORT bool Physics_IsSleeping(uint32_t actorId);
NEBULA_EXPORT void Physics_WakeUp(uint32_t actorId);
NEBULA_EXPORT void Physics_Sleep(uint32_t actorId);

NEBULA_EXPORT Math::vec3 Physics_GetVelocity(uint32_t actorId);
NEBULA_EXPORT void Physics_SetVelocity(uint32_t actorId, Math::vec3 velocity);
NEBULA_EXPORT Math::vec3 Physics_GetAngularVelocity(uint32_t actorId);
NEBULA_EXPORT void Physics_SetAngularVelocity(uint32_t actorId, Math::vec3 velocity);

NEBULA_EXPORT void Physics_AddForce(uint32_t actorId, Math::vec3 force);
NEBULA_EXPORT void Physics_AddImpulse(uint32_t actorId, Math::vec3 impulse);
NEBULA_EXPORT void Physics_AddImpulseAtPoint(uint32_t actorId, Math::vec3 impulse, Math::vec3 point);
NEBULA_EXPORT void Physics_AddTorque(uint32_t actorId, Math::vec3 torque);

NEBULA_EXPORT uint32_t Physics_GetSceneForWorld(uint32_t worldId);

NEBULA_EXPORT bool Physics_Raycast(uint32_t scene, Math::point origin, Math::vector direction, float maxDistance, PhysicsFeature::RaycastHit* outHit, const PhysicsFeature::QueryFilter* filter);
NEBULA_EXPORT bool Physics_SegmentCast(uint32_t scene, Math::point from, Math::point to, PhysicsFeature::RaycastHit* outHit, const PhysicsFeature::QueryFilter* filter);

// ------------------------------------------------------------------------------------------
// Character controllers
// ------------------------------------------------------------------------------------------

NEBULA_EXPORT bool Physics_IsCharacterValid(uint32_t characterId);

NEBULA_EXPORT Math::vec3 Physics_GetCharacterFootPosition(uint32_t characterId);
NEBULA_EXPORT Math::vec3 Physics_GetCharacterCenterPosition(uint32_t characterId);

/// The collision bits the move answered with, as a bitfield: bit 0 sides, bit 1 up, bit 2 down
NEBULA_EXPORT uint32_t Physics_MoveCharacter(uint32_t characterId, Math::vec3 displacement, float minDist, float timeSinceLastMove);

NEBULA_EXPORT void Physics_SetCharacterPosition(uint32_t characterId, Math::vec3 footPosition);
NEBULA_EXPORT void Physics_SetCharacterHeight(uint32_t characterId, float newHeight);

} // namespace ScriptApi
} // namespace PhysicsFeature
