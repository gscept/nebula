//------------------------------------------------------------------------------
//  nsbindings.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "nsbindings.h"
#include "propertyextension.h"
#include "core/debug.h"

namespace Scripting
{

//------------------------------------------------------------------------------
/**
*/
void
N_Print(const char * string, int32_t is_stdout)
{
	n_printf("%s", string);
}

//------------------------------------------------------------------------------
/**
*/
void
N_Error(const char * string, int32_t is_stdout)
{
	n_error("%s", string);
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

//------------------------------------------------------------------------------
/**
*/
void
RecordPropertyChange(uint64_t entityId, const char* path, const char* oldText, const char* newText)
{
    Scripting::PropertyLevelExtension::RecordChange(Game::Entity(entityId), path, oldText, newText);
}

//------------------------------------------------------------------------------
/**
*/
void
RequestPropertyRevert(uint64_t entityId, const char* path)
{
    Scripting::PropertyLevelExtension::RequestRevert(Game::Entity(entityId), path);
}

//------------------------------------------------------------------------------
/**
*/
bool
PropertyExtension_IsAuthored(uint64_t entityId, const char* path)
{
    return Scripting::PropertyLevelExtension::IsAuthored(Game::Entity(entityId), path);
}

//------------------------------------------------------------------------------
/**
*/
void
PropertyExtension_AddName(const char* propertyName)
{
    Scripting::PropertyLevelExtension::AddPropertyName(propertyName);
}

//------------------------------------------------------------------------------
/**
*/
void
PropertyExtension_ClearNames()
{
    Scripting::PropertyLevelExtension::ClearPropertyNames();
}

} // namespace Scripting
