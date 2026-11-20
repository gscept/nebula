#pragma once
//------------------------------------------------------------------------------
/**
	@file scripting/api/game.h

	Contains api calls to various game system functions

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "nsharp/nsconfig.h"

namespace Scripting
{

namespace Api
{

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT bool EntityIsValid(uint64_t entity);

//------------------------------------------------------------------------------
/**
    Whether the entity has an instance. IsValid only says the id is allocated, and an id can be held by
    something that has taken the instance out of the world, which is what an editor command does while it
    waits for a redo. A mirror entity that is only an id has nothing for a component access to read.
*/
NEBULA_EXPORT bool EntityHasInstance(uint64_t entity);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT uint64_t EntityCreate(uint32_t worldId);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void EntityDelete(uint64_t entity);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT bool EntityHasComponent(uint64_t entity, uint32_t componentId);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT Math::float4 EntityGetPosition(uint64_t entity);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void EntitySetPosition(uint64_t entity, Math::vec3 pos);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT Math::float4 EntityGetOrientation(uint64_t entity);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void EntitySetOrientation(uint64_t entity, Math::quat orientation);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT Math::float4 EntityGetScale(uint64_t entity);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void EntitySetScale(uint64_t entity, Math::vec3 scale);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT uint32_t ComponentGetId(const char* name);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT uint32_t ComponentGetIdUtf8(const char* name, uint32_t length);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void ComponentGetData(uint64_t entity, uint32_t componentId, void* outData, int dataSize);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void ComponentSetData(uint64_t entity, uint32_t componentId, void* data, int dataSize);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT uint32_t WorldGetDefaultWorldId();

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT bool WorldHasWorld(uint32_t worldId);

//------------------------------------------------------------------------------
/**
    Whether the world is simulating. This is the same rule a processor has: a property only runs in a
    world whose mode is PlayMode, which is what keeps an editor's authoring world from running anything.
*/
NEBULA_EXPORT bool WorldIsSimulating(uint32_t worldId);

//------------------------------------------------------------------------------
/**
    Copy every time source into the buffer C# owns. The layout is the contract between the two sides, and
    the doubles come before the ints so that neither side pads. Returns the number of entries written.
*/
NEBULA_EXPORT int TimeSourceSync(void* buffer, int maxEntries);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void TimeSourceSetTimeFactor(uint32_t hash, float factor);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void TimeSourceSetPauseCounter(uint32_t hash, int counter);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT uint32_t TimeSourceCreate(uint32_t hash, double fixedFrameTime, int maxStepsPerFrame);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void TimeSetGlobalTimeFactor(float factor);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT float TimeGetGlobalTimeFactor();

} // namespace Api

} // namespace Scripting
