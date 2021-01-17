#pragma once
//------------------------------------------------------------------------------
/**
    @class Win32::Win32Environment
    
    Provides read-access to environment variables. Useful for tools.
    NOTE: using this class restricts your code to the Win32 platform.    
    
    @copyright
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace Win32
{
class Win32Environment
{
public:
    /// return true if env-variable exists
    static bool Exists(const Util::String& envVarName);
    /// get value of existing env-variable
    static Util::String Read(const Util::String& envVarName);
};

} // namespace Win32
//------------------------------------------------------------------------------
    