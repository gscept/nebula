#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::FSWrapper
    
    This is an internal IO class used to wrap platform specific
    filesystem access into a generic class. To port the filesystem code
    to a new platform all that has to be done is to write a new 
    FSWrapper class.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if (__WIN32__ || __XBOX360__)
#include "io/win360/win360fswrapper.h"
namespace IO
{
class FSWrapper : public Win360::Win360FSWrapper
{ };
}
#elif __WII__
#include "io/wii/wiifswrapper.h"
namespace IO
{
class FSWrapper : public Wii::WiiFSWrapper
{ };
}
#elif __PS3__
#include "io/ps3/ps3fswrapper.h"
namespace IO
{
class FSWrapper : public PS3::PS3FSWrapper
{ };
}
#elif ( __OSX__ || __APPLE__ || linux )
#include "io/posix/posixfswrapper.h"
namespace IO
{
class FSWrapper : public Posix::PosixFSWrapper
{ };
}
#else
#error "FSWrapper class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------


