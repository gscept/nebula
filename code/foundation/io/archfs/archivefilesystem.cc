//------------------------------------------------------------------------------
//  archivefilesystem.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/archfs/archivefilesystem.h"

namespace IO
{
#if __WIN32__ || __XBOX360__ || __linux__
__ImplementClass(IO::ArchiveFileSystem, 'ARFS', IO::ZipFileSystem);
__ImplementInterfaceSingleton(IO::ArchiveFileSystem);
#elif __WII__
__ImplementClass(IO::ArchiveFileSystem, 'ARFS', Wii::WiiArchiveFileSystem);
__ImplementInterfaceSingleton(IO::ArchiveFileSystem);
#elif __PS3__
__ImplementClass(IO::ArchiveFileSystem, 'ARFS', PS3::PS3ArchiveFileSystem);
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
