#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AppLauncher
    
    Launch an external application.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __WIN32__
#include "win32/win32applauncher.h"
namespace ToolkitUtil
{
typedef ToolkitUtil::Win32AppLauncher AppLauncher;
}
#elif __LINUX__
#include "toolkitutil/posix/posixapplauncher.h"
namespace ToolkitUtil
{
typedef ToolkitUtil::PosixAppLauncher AppLauncher;
}
#else
#error "ToolkitUtil::AppLauncher not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
    
