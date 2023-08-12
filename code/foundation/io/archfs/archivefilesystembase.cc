//------------------------------------------------------------------------------
//  archivefilesystembase.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/archfs/archivefilesystembase.h"
#include "io/archfs/archive.h"
#include "io/assignregistry.h"

namespace IO
{
__ImplementClass(IO::ArchiveFileSystemBase, 'AFSB', Core::RefCounted);
__ImplementInterfaceSingleton(IO::ArchiveFileSystemBase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
ArchiveFileSystemBase::ArchiveFileSystemBase() :
    isValid(false)
{
    __ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ArchiveFileSystemBase::~ArchiveFileSystemBase()
{
    if (this->IsValid())
    {
        // make sure that derived method is called
        this->Discard();
    }    
    __DestructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
    Setup the archive file system. Subclasses may register their
    archive stream classes with the SchemeRegistry here.
*/
void
ArchiveFileSystemBase::Setup()
{
    n_assert(!this->IsValid());
    this->isValid = true;
}

//------------------------------------------------------------------------------
/**
    Discard the archive file system.
*/
void
ArchiveFileSystemBase::Discard()
{
    n_assert(this->IsValid());

    // unmount all mounted filesystems
    while (!this->archives.IsEmpty())
    {
        // make sure that derived method is called
        this->Unmount(this->archives.ValueAtIndex(0));
    }

    this->isValid = false;
}

//------------------------------------------------------------------------------
/**
    This "mounts" an archive file by creating a new Archive object
    and adding it to the archive dictionary. If mounting fails, an invalid
    pointer will be returned!
*/
Ptr<Archive>
ArchiveFileSystemBase::Mount(const URI& uri)
{
    return MountEmbedded(uri, "");
}
//------------------------------------------------------------------------------
/**
    This "mounts" an archive file by creating a new Archive object
    and adding it to the archive dictionary. If mounting fails, an invalid
    pointer will be returned!
*/
Ptr<Archive>
ArchiveFileSystemBase::MountEmbedded(const URI& uri, const Util::String& rootPath)
{
    n_assert(!this->IsMounted(uri));
    String path = AssignRegistry::Instance()->ResolveAssigns(uri).LocalPath();
    Ptr<Archive> newArchive = Archive::Create();
    if (newArchive->Setup(uri, rootPath))
    {
        this->critSect.Enter();
        this->archives.Add(path, newArchive);
        this->critSect.Leave();
    }
    else
    {
        newArchive = nullptr;
    }
    return newArchive;
}


//------------------------------------------------------------------------------
/**
    Unmount a zip archive, this will remove the archive from the internal
    archive registry, and call the Discard() method on it.
*/
void
ArchiveFileSystemBase::Unmount(const Ptr<Archive>& archive)
{
    n_assert(this->IsMounted(archive->GetURI()));
    archive->Discard();
    String path = AssignRegistry::Instance()->ResolveAssigns(archive->GetURI()).LocalPath();

    this->critSect.Enter();
    this->archives.Erase(path);
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
    Unmount an archive by the archive's URI. 
*/
void
ArchiveFileSystemBase::Unmount(const URI& uri)
{
    n_assert(this->IsMounted(uri));
    String path = AssignRegistry::Instance()->ResolveAssigns(uri).LocalPath();
    Ptr<Archive> archive = this->archives[path];
    archive->Discard();

    this->critSect.Enter();
    this->archives.Erase(path);
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
    Return all currently mounted archives.
*/
Array<Ptr<Archive> >
ArchiveFileSystemBase::GetMountedArchives() const
{
    this->critSect.Enter();    
    Array<Ptr<Archive> > archiveArray = this->archives.ValuesAsArray();
    this->critSect.Leave();
    return archiveArray;
}

//------------------------------------------------------------------------------
/**
    Resolve an archive path into an Archive pointer. Returns 0
    if no archive with that name exists. The filename will be resolved into
    an absolute path internally before the lookup happens.
*/
Ptr<Archive>
ArchiveFileSystemBase::FindArchive(const URI& uri) const
{
    String path = AssignRegistry::Instance()->ResolveAssigns(uri).LocalPath();
    Ptr<Archive> result;

    this->critSect.Enter();    
    IndexT index = this->archives.FindIndex(path);
    if (InvalidIndex != index)
    {
        result = this->archives.ValueAtIndex(index);
    }
    this->critSect.Leave();
    return result;
}

//------------------------------------------------------------------------------
/**
*/
bool
ArchiveFileSystemBase::IsMounted(const URI& uri) const
{
    String path = AssignRegistry::Instance()->ResolveAssigns(uri).LocalPath();
    this->critSect.Enter();
    bool result = this->archives.Contains(path);
    this->critSect.Leave();
    return result;
}

//------------------------------------------------------------------------------
/**
*/
bool
ArchiveFileSystemBase::HasArchives() const
{
    return !this->archives.IsEmpty();
}

//------------------------------------------------------------------------------
/**
    This method should return the archive which contains the provided 
    file URI. Override this method in a derived class!
*/
Ptr<Archive>
ArchiveFileSystemBase::FindArchiveWithFile(const URI& uri) const
{
    return Ptr<Archive>();
}

//------------------------------------------------------------------------------
/**
    This method should return the archive which contains the
    provided directory URI. Override this method in a derived class!
*/
Ptr<Archive>
ArchiveFileSystemBase::FindArchiveWithDir(const URI& uri) const
{
    return Ptr<Archive>();
}

//------------------------------------------------------------------------------
/**
    This method should check if the provided URI is located in any
    of the mounted archives, and if yes, return a converted the URI 
    which describes how the file in the archive is accessed.

    This method must be overriden in a subclass, the base class implementation
    will always return the original URI!
*/
URI
ArchiveFileSystemBase::ConvertFileToArchiveURIIfExists(const URI& uri) const
{
    // make sure that derived method is called
    Ptr<Archive> archive = this->FindArchiveWithFile(uri);
    if (archive.isvalid())
    {
        return archive->ConvertToArchiveURI(uri);
    }
    // fallthrough: no match, return original uri
    return uri;
}

//------------------------------------------------------------------------------
/**
    This method is the directory version of ConvertFileToArchiveURIIfExists().
*/
URI
ArchiveFileSystemBase::ConvertDirToArchiveURIIfExists(const URI& uri) const
{
    Ptr<Archive> archive = this->FindArchiveWithDir(uri);
    if (archive.isvalid())
    {
        return archive->ConvertToArchiveURI(uri);
    }
    // fallthrough: no match, return original uri
    return uri;
}

} // namespace IO
