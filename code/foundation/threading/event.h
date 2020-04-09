#pragma once
//------------------------------------------------------------------------------
/**
    @class Threading::Event
  
    @todo describe Event class
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__)
#include "threading/win32/win32event.h"
namespace Threading
{
class Event : public Win32::Win32Event
{
public:
    Event(bool manualReset=false) : Win32Event(manualReset) {};
};
}
#elif __linux__ 
#include "threading/linux/linuxevent.h"
namespace Threading
{
class Event : public Linux::LinuxEvent
{
public:
    Event(bool manualReset=false) : LinuxEvent(manualReset) {};
};
}
#elif ( __OSX__ || __APPLE__ )
#include "threading/posix/posixevent.h"
namespace Threading
{
class Event : public Posix::PosixEvent
{ 
public:
    Event(bool manualReset=false) : PosixEvent(manualReset) {};
};
}
#else
#error "Threading::Event not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
