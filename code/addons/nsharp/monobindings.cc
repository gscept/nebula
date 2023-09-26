//------------------------------------------------------------------------------
//  monobindings.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "monobindings.h"
#include "core/debug.h"
#include "mono/metadata/loader.h"
#include "mono/jit/jit.h"
#include "mono/metadata/assembly.h"
#include "mono/metadata/debug-helpers.h"
#include "conversion/vector2.h"
#include "conversion/vector3.h"
#include "conversion/vector4.h"

namespace Mono
{

MonoBindings::MonoBindings(MonoImage* image) :
    image(image)
{
}

MonoBindings::~MonoBindings()
{
}

//------------------------------------------------------------------------------
/**
*/
void
MonoBindings::Initialize()
{
    // Setup math class conversions
    Mono::Vector2::Setup(this->image);
    Mono::Vector3::Setup(this->image);
    Mono::Vector4::Setup(this->image);

    // Setup calls between C/C++/C#
	MonoBindings::SetupInternalCalls();
}

//------------------------------------------------------------------------------
/**
*/
void
MonoBindings::SetupInternalCalls()
{
}

//------------------------------------------------------------------------------
/**
*/
void
N_Print(const char * string, int32_t is_stdout)
{
	n_printf(string);
}

//------------------------------------------------------------------------------
/**
*/
void
N_Error(const char * string, int32_t is_stdout)
{
	n_error(string);
}

//------------------------------------------------------------------------------
/**
*/
void
N_Log(const char * log_domain, const char * log_level, const char * message, int32_t fatal, void * user_data)
{
	if (fatal)
	{
		n_error(message);
	}
	else
	{
		n_warning(message);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
N_Assert(bool value)
{
    n_assert(value);
}

} // namespace Mono
