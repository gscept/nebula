#pragma once
//------------------------------------------------------------------------------
/**
    @class Posix::PosixEnvironment
    
    Provides read-access to environment variables. Useful for tools.
            
    (C) 2013 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace Posix
{
class PosixEnvironment
{
public:
    /// return true if env-variable exists
    static bool Exists(const Util::String& envVarName);
    /// get value of existing env-variable
    static Util::String Read(const Util::String& envVarName);
};

} // namespace Posix
//------------------------------------------------------------------------------
    