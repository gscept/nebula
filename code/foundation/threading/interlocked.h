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
#elif ( __OSX__ || __APPLE__ )
#include "threading/darwin/darwininterlocked.h"
namespace Threading
{
class Interlocked : public Darwin::DarwinInterlocked
{ };
}
#elif __linux__
#include "threading/posix/posixinterlocked.h"
namespace Threading
{
class Interlocked : public Posix::PosixInterlocked
{ };
}
#else
#error "Threading::Interlocked not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
