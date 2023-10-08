#pragma once
//------------------------------------------------------------------------------
/**
	@file nsbindings.h

	Bindings specifically for C# scripting

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "nsconfig.h"
#include "game/entity.h"

namespace Scripting
{

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void N_Print(const char *string, int32_t is_stdout);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void N_Error(const char *string, int32_t is_stdout);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void N_Log(const char *log_domain, const char *log_level, const char *message, int32_t fatal, void *user_data);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void N_Assert(bool value);

} // namespace Scripting
