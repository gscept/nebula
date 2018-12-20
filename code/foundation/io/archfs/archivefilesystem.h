#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::ArchiveFileSystem
    
    Top-level platform wrapper class of archive file systems. 
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#if __WIN32__ || __XBOX360__ || __linux__
#include "io/zipfs/zipfilesystem.h"
namespace IO
{
class ArchiveFileSystem : public ZipFileSystem
{
    __DeclareClass(ArchiveFileSystem);
    __DeclareInterfaceSingleton(ArchiveFileSystem);
public:
    /// constructor
    ArchiveFileSystem();
    /// destructor
    virtual ~ArchiveFileSystem();
};
}
#elif __WII__
#include "io/wii/wiiarchivefilesystem.h"
namespace IO
{
class ArchiveFileSystem : public Wii::WiiArchiveFileSystem
{
    __DeclareClass(ArchiveFileSystem);
    __DeclareInterfaceSingleton(ArchiveFileSystem);
public:
    /// constructor
    ArchiveFileSystem();
    /// destructor
    virtual ~ArchiveFileSystem();
};
}
#elif __PS3__
#include "io/ps3/ps3archivefilesystem.h"
namespace IO
{
class ArchiveFileSystem : public PS3::PS3ArchiveFileSystem
{
    __DeclareClass(ArchiveFileSystem);
    __DeclareInterfaceSingleton(ArchiveFileSystem);
public:
    /// constructor
    ArchiveFileSystem();
    /// destructor
    virtual ~ArchiveFileSystem();
};
}
#else
#error "IO::ArchiveFileSystem not implemented on this platform!"
#endif