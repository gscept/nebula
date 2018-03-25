//------------------------------------------------------------------------------
//  win32environment.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "system/win32/win32environment.h"

namespace Win32
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
bool
Win32Environment::Exists(const String& envVarName)
{
    char buf[256];
    DWORD result = ::GetEnvironmentVariable(envVarName.AsCharPtr(), buf, sizeof(buf));
    if (0 == result)
    {
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
String
Win32Environment::Read(const String& envVarName)
{
    String retval;
    char buf[256];
    DWORD result = ::GetEnvironmentVariable(envVarName.AsCharPtr(), buf, sizeof(buf));
    
    // error?
    if (0 == result)
    {
        n_error("Win32Environment::Read: failed to read env variable '%s'!", envVarName.AsCharPtr());
    }
    else if (result >= sizeof(buf))
    {
        // buffer overflow?
        char* dynBuf = (char*) Memory::Alloc(Memory::ScratchHeap, result);
        DWORD dynResult = ::GetEnvironmentVariable(envVarName.AsCharPtr(), buf, sizeof(buf));
        n_assert(0 != dynResult);
        retval = dynBuf;
        Memory::Free(Memory::ScratchHeap, dynBuf);
        dynBuf = 0;
    }
    else
    {
        // all ok
        retval = buf;
    }
    return retval;
}

} // namespace Win32