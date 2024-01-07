//------------------------------------------------------------------------------
//  zipfilesystem.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/zipfs/zipfilesystem.h"
#include "io/assignregistry.h"
#include "io/ioserver.h"
#include "io/archfs/archive.h"
#include "io/zipfs/zipfilestream.h"

namespace IO
{
__ImplementClass(IO::ZipFileSystem, 'ZPFS', IO::ArchiveFileSystemBase);
__ImplementInterfaceSingleton(IO::ZipFileSystem);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
ZipFileSystem::ZipFileSystem()
{
    __ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ZipFileSystem::~ZipFileSystem()
{
    if (this->IsValid())
    {
        this->Discard();
    }
    __DestructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
    Setup the ZipFileSystem. Registers the ZipFileStream class.
*/
void
ZipFileSystem::Setup()
{
    n_assert(!this->IsValid());
    ArchiveFileSystemBase::Setup();
    SchemeRegistry::Instance()->RegisterUriScheme("zip", ZipFileStream::RTTI);
}

//------------------------------------------------------------------------------
/**
*/
void
ZipFileSystem::Discard()
{
    n_assert(this->IsValid());
    SchemeRegistry::Instance()->UnregisterUriScheme("zip");
    ArchiveFileSystemBase::Discard();
}

//------------------------------------------------------------------------------
/**
    This method takes a normal file URI and checks if the local path
    of the URI is contained as file entry in any mounted zip archive. If yes
    ptr to the zip archive is returned, otherwise a 0 pointer. NOTE: if the 
    same path resides in several zip archives, it is currently not defined
    which one will be returned (the current implementation returns the
    first zip archive in alphabetical order which contains the file).
*/
Ptr<Archive>
ZipFileSystem::FindArchiveWithFile(const URI& uri) const
{
    // get the local path from the URI
    String localPath = AssignRegistry::Instance()->ResolveAssigns(uri).LocalPath();
    n_assert(localPath.IsValid());

    // check each mounted archive
    Ptr<ZipArchive> result;
    this->critSect.Enter();
    IndexT i;
    for (i = 0; (i < this->archives.Size()) && (!result.isvalid()); i++)
    {
        const Ptr<ZipArchive>& arch = this->archives.ValueAtIndex(i).cast<ZipArchive>();
        String pathInZipArchive = arch->ConvertToPathInArchive(localPath);
        if (pathInZipArchive.IsValid())
        {
            if (0 != arch->FindFileEntry(pathInZipArchive))
            {
                result = arch;
                break;
            }
        }
    }
    this->critSect.Leave(); 

    // result may be invalid pointer at this point
    return result.cast<Archive>();
}

//------------------------------------------------------------------------------
/**
    Same as FindArchiveWithFile(), but checks for a directory entry 
    in a zip file.
*/
Ptr<Archive>
ZipFileSystem::FindArchiveWithDir(const URI& uri) const
{
    // get the local path from the URI
    String localPath = AssignRegistry::Instance()->ResolveAssigns(uri).LocalPath();
    n_assert(localPath.IsValid());

    // check each mounted archive
    Ptr<ZipArchive> result;
    this->critSect.Enter();
    IndexT i;
    for (i = 0; (i < this->archives.Size()) && (!result.isvalid()); i++)
    {
        const Ptr<ZipArchive>& arch = this->archives.ValueAtIndex(i).cast<ZipArchive>();
        String pathInZipArchive = arch->ConvertToPathInArchive(localPath);
        if (pathInZipArchive.IsValid())
        {
            if (0 != arch->FindDirEntry(pathInZipArchive))
            {
                result = arch;
                break;
            }
        }
    }
    this->critSect.Leave(); 

    // result may be invalid pointer at this point
    return result.cast<Archive>();
}

} // namespace IO
