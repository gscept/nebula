#pragma once
//------------------------------------------------------------------------------
/**
    @class Debug::MiniDump
    
    Support for generating mini dumps. Mini dumps are automatically 
    created when n_assert() or n_error() triggers.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __WIN32__
#include "debug/win32/win32minidump.h"
namespace Debug
{
class MiniDump : public Win32::Win32MiniDump
{ };
}
#endif
//------------------------------------------------------------------------------
