#pragma once
//------------------------------------------------------------------------------
/**
    @class Core::SysFunc
    
    Wrap some platform specific low-level functions.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if __WIN32__
#include "core/win32/win32sysfunc.h"
namespace Core
{
class SysFunc : public Win32::SysFunc
{
    // empty
};
} // namespace Core
#elif ( __OSX__ || __APPLE__ || __linux__ )
#include "core/posix/posixsysfunc.h"
namespace Core
{
class SysFunc : public Posix::SysFunc
{
    // empty
};
} // namespace Core
#else
#error "Core::SysFunc not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
