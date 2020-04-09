#pragma once
//------------------------------------------------------------------------------
/**
    @class Threading::Rendezvous
    
    A thread-barrier for exactly 2 threads.
    
    (C) 2010 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__)
#include "threading/win360/win360rendezvous.h"
namespace Threading
{
class Rendezvous : public Win360::Win360Rendezvous
{ };
}
#elif __OSX__
#include "threading/osx/osxrendezvous.h"
namespace Threading
{
class Rendezvous : public OSX::OSXRendezvous
{ };
}
#elif __LINUX__
#include "threading/linux/linuxrendezvous.h"
namespace Threading
{
class Rendezvous : public Linux::LinuxRendezvous
{ };
}
#else
#error "Threading::Rendezvous not implemented on this platform!"
#endif


    
