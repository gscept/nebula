#pragma once
//------------------------------------------------------------------------------
/**
    @class Win32::WinThreadId
    
    A thread id uniquely identifies a thread within the process.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

namespace Threading
{
typedef DWORD ThreadId;
static const ThreadId InvalidThreadId = 0xffffffff;
}
//------------------------------------------------------------------------------
