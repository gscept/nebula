#pragma once
//------------------------------------------------------------------------------
/**
	@file nsbindings.h

	Bindings specifically for C# scripting

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "nsharp/nsconfig.h"

namespace Scripting
{

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void N_Print(const char* string, int32_t isStdout);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void N_Error(const char* string, int32_t isStdout);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void N_Log(const char* logDomain, const char* logLevel, const char* message, int32_t fatal, void* userData);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void N_Assert(bool value);

} // namespace Scripting
