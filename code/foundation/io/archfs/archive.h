#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::Archive
  
    Wrapper class for a platform-specific file archive.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#if __WIN32__ || __XBOX360__ || __linux__
#include "io/zipfs/ziparchive.h"
namespace IO
{
class Archive : public ZipArchive
{
    __DeclareClass(Archive);
};
}
#elif __WII__
#include "io/wii/wiiarchive.h"
namespace IO
{
class Archive : public Wii::WiiArchive
{
    __DeclareClass(Archive);
};
}
#elif __PS3__
#include "io/ps3/ps3archive.h"
namespace IO
{
class Archive : public PS3::PS3Archive
{
    __DeclareClass(Archive);
};
}
#else
#error "IO::Archive not implemented on this platform!"
#endif
