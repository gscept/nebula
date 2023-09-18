#pragma once
//------------------------------------------------------------------------------
/**
	@file monobindings.h

	Bindings specifically for mono scripting

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "monoconfig.h"
#include "mono/metadata/object.h"
#include "game/entity.h"
#include "mono/metadata/object-forward.h"

namespace Mono
{

class MonoBindings
{
public:
	MonoBindings() = default;
	MonoBindings(MonoImage* image);
	~MonoBindings();

	void Initialize();

private:
	static void SetupInternalCalls();

	MonoImage* image = nullptr;
};

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


} // namespace Mono
