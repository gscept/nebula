#pragma once
//------------------------------------------------------------------------------
/**
    @class Posix::PosixThreadId
 
    A thread id uniquely identifies a thread within the process.
 
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

namespace Threading
{
typedef pthread_t ThreadId;
static const ThreadId InvalidThreadId = 0;
typedef int64_t ThreadIdStorage;
static_assert(sizeof(ThreadId) == sizeof(ThreadIdStorage));
}
//------------------------------------------------------------------------------
