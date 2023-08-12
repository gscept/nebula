//------------------------------------------------------------------------------
//  archivefilesystem.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/archfs/archivefilesystem.h"

namespace IO
{
#if __WIN32__ || __linux__
__ImplementClass(IO::ArchiveFileSystem, 'ARFS', IO::ZipFileSystem);
__ImplementInterfaceSingleton(IO::ArchiveFileSystem);
#else
#error "IO::ArchiveFileSystem not implemented on this platform!"
#endif

//------------------------------------------------------------------------------
/**
*/
ArchiveFileSystem::ArchiveFileSystem()
{
    __ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ArchiveFileSystem::~ArchiveFileSystem()
{
    __DestructInterfaceSingleton;
}

} // namespace IO
