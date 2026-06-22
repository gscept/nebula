#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::ArchiveFileSystem
    
    Top-level platform wrapper class of archive file systems. 
    
    @copyright
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#if __WIN32__ || __linux__ || __APPLE__
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
#else
#error "IO::ArchiveFileSystem not implemented on this platform!"
#endif
