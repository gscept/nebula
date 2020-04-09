#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::FSWrapper
    
    This is an internal IO class used to wrap platform specific
    filesystem access into a generic class. To port the filesystem code
    to a new platform all that has to be done is to write a new 
    FSWrapper class.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#if (__WIN32__)
#include "io/win32/win32fswrapper.h"
namespace IO
{
class FSWrapper : public Win32::Win32FSWrapper
{ };
}
#elif ( __OSX__ || __APPLE__ || __linux__ )
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


