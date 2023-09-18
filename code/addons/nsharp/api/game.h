#pragma once
//------------------------------------------------------------------------------
/**
	@file scripting/api/game.h

	Contains api calls to various game system functions

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "nsharp/monoconfig.h"

namespace Scripting
{

namespace Api
{

//------------------------------------------------------------------------------
/**
	Check whether an entity id is valid.
*/
NEBULA_EXPORT bool IsEntityValid(unsigned int entity);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT unsigned int CreateEntity();

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void DeleteEntity(unsigned int entity);

} // namespace Api

} // namespace Scripting
