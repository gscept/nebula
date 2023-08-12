#pragma once
//------------------------------------------------------------------------------
/**
    @class Threading::Interlocked
    
    Provide simple atomic operations on memory variables.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"

namespace Threading
{
typedef volatile int AtomicCounter;
}

#if (__WIN32__)
#include "threading/win32/win32interlocked.h"
namespace Threading
{
class Interlocked : public Win32::Win32Interlocked
{ };
}
#elif ( __linux__ || __OSX__ || __APPLE__)
#include "threading/gcc/interlocked.h"
namespace Threading
{
class Interlocked : public Gcc::GccInterlocked
{ };
}
#else
#error "Threading::Interlocked not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
