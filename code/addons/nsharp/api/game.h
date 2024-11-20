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
*/
NEBULA_EXPORT uint64_t EntityCreateFromTemplate(uint32_t worldId, const char* tmpl);

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
NEBULA_EXPORT void ComponentGetData(uint64_t entity, uint32_t componentId, void* outData, int dataSize);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void ComponentSetData(uint64_t entity, uint32_t componentId, void* data, int dataSize);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT uint32_t WorldGetDefaultWorldId();

} // namespace Api

} // namespace Scripting
