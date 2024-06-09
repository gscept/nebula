#pragma once
//------------------------------------------------------------------------------
/**
    @type Threading::ThreadId
    
    The ThreadId type is used to uniqely identify a Nebula Thread. The
    main thread always has a ThreadId of 0. Get the current thread-id
    with the static method Thread::GetMyThreadId().
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#if __WIN32__
#include "threading/win32/win32threadid.h"
#elif __APPLE__
#include "threading/posix/posixthreadid.h"
#elif __linux__
#include "threading/linux/linuxthreadid.h"
#else
#error "Threading::ThreadId not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

