#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Guid
    
    Implements a GUID.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if __WIN32__
#include "util/win32/win32guid.h"
namespace Util
{
typedef Win32::Win32Guid Guid;
}
#elif __XBOX360__
#include "util/xbox360/xbox360guid.h"
namespace Util
{
typedef Xbox360::Xbox360Guid Guid;
}
#elif __WII__
#include "util/wii/wiiguid.h"
namespace Util
{
typedef Wii::WiiGuid Guid;
}
#elif __PS3__
#include "util/ps3/ps3guid.h"
namespace Util
{
typedef PS3::PS3Guid Guid;
}
#elif __OSX__
#include "util/osx/osxguid.h"
namespace Util
{
typedef OSX::OSXGuid Guid;
}
#elif ( __APPLE__ || linux )
#include "util/posix/posixguid.h"
namespace Util
{
typedef Posix::PosixGuid Guid;
}
#else
#error "Util::Guid not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
    