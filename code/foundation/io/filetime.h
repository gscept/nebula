#pragma once
#ifndef IO_FILETIME_H
#define IO_FILETIME_H
//------------------------------------------------------------------------------
/**
    @class IO::FileTime
    
    Defines a file-access timestamp.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__ || __XBOX360__)
#include "io/win360/win360filetime.h"
namespace IO
{
typedef Win360::Win360FileTime FileTime;
}
#elif __WII__
#include "io/wii/wiifiletime.h"
namespace IO
{
typedef Wii::WiiFileTime FileTime;
}
#elif __PS3__
#include "io/ps3/ps3filetime.h"
namespace IO
{
typedef PS3::PS3FileTime FileTime;
}
#elif __OSX__
#include "io/osx/osxfiletime.h"
namespace IO
{
typedef OSX::OSXFileTime FileTime;
}
#elif __linux__
#include "io/posix/posixfiletime.h"
namespace IO
{
typedef Posix::PosixFileTime FileTime;
}
#else
#error "FileTime class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
#endif


