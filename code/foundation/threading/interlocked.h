#pragma once
//------------------------------------------------------------------------------
/**
    @class Threading::Interlocked
    
    Provide simple atomic operations on memory variables.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__ || __XBOX360__)
#include "threading/win360/win360interlocked.h"
namespace Threading
{
class Interlocked : public Win360::Win360Interlocked
{ };
}
#elif __WII__
#include "threading/wii/wiiinterlocked.h"
namespace Threading
{
class Interlocked : public Wii::WiiInterlocked
{ };
}
#elif __PS3__
#include "threading/ps3/ps3interlocked.h"
namespace Threading
{
class Interlocked : public PS3::PS3Interlocked
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
#include "threading/linux/linuxinterlocked.h"
namespace Threading
{
class Interlocked : public Linux::LinuxInterlocked
{ };
}
#else
#error "Threading::Interlocked not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
