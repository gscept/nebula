#pragma once
//------------------------------------------------------------------------------
/**
    @class Threading::Thread
    
    @todo describe Thread class
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if __WIN32__
#include "threading/win32/win32thread.h"
namespace Threading
{
class Thread : public Win32::Win32Thread
{ 
    __DeclareClass(Thread);
};
}
#elif __linux__ 
#include "threading/linux/linuxthread.h"
namespace Threading
{
class Thread : public Linux::LinuxThread
{
    __DeclareClass(Thread);
};
}
#elif ( __OSX__ || __APPLE__ )
#include "threading/posix/posixthread.h"
namespace Threading
{
class Thread : public Posix::PosixThread
{
    __DeclareClass(Thread);
};
}
#else
#error "Threading::Thread not implemented on this platform!"
#endif
namespace Threading
{
extern Threading::ThreadId MainThreadId;
}
//------------------------------------------------------------------------------
