//------------------------------------------------------------------------------
//  posixenvironment.cc
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "system/posix/posixenvironment.h"

namespace Posix
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
bool
PosixEnvironment::Exists(const String& envVarName)
{
    return NULL != getenv(envVarName.AsCharPtr());    
}

//------------------------------------------------------------------------------
/**
*/
String
PosixEnvironment::Read(const String& envVarName)
{
    char * env = getenv(envVarName.AsCharPtr());
    if(NULL == env)
    {
        n_error("Failed to read environment variable %s\n",envVarName.AsCharPtr());
    }
    Util::String str;
    str.SetCharPtr(env);
    return env;    
}

} // namespace Posix