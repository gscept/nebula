#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::ArchiveFileSystemBase
    
    Base class for archive file system wrappers.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "util/dictionary.h"
#include "io/uri.h"

//------------------------------------------------------------------------------
namespace IO
{
class Archive;

class ArchiveFileSystemBase : public Core::RefCounted
{
    __DeclareClass(ArchiveFileSystemBase);
    __DeclareInterfaceSingleton(ArchiveFileSystemBase);
public:
    /// constructor
    ArchiveFileSystemBase();
    /// destructor
    virtual ~ArchiveFileSystemBase();

    /// setup the archive file system
    void Setup();
    /// discard the archive file system
    void Discard();
    /// return true if archive file system has been setup
    bool IsValid() const;
    
    /// mount an archive
    virtual Ptr<Archive> Mount(const URI& uri);
    /// unmount an archive by URI
    virtual void Unmount(const URI& uri);
    /// unmount an archive by pointer
    virtual void Unmount(const Ptr<Archive>& archive);
    /// return true if an archive is mounted
    bool IsMounted(const URI& uri) const;
    
    /// get an array of all mounted archives
    Util::Array<Ptr<Archive> > GetMountedArchives() const;
    /// find a zip archive by its URI, returns invalid ptr if not mounted
    Ptr<Archive> FindArchive(const URI& uri) const;

    /// find first archive which contains the file path
    virtual Ptr<Archive> FindArchiveWithFile(const URI& fileUri) const;
    /// find first archive which contains the directory path
    virtual Ptr<Archive> FindArchiveWithDir(const URI& dirUri) const;
    /// transparently convert a URI pointing to a file into a matching archive URI
    URI ConvertFileToArchiveURIIfExists(const URI& uri) const;
    /// transparently convert a URI pointing to a directory into a matching archive URI    
    URI ConvertDirToArchiveURIIfExists(const URI& uri) const;

protected:
    Threading::CriticalSection critSect;
    Util::Dictionary<Util::String, Ptr<Archive> > archives;
    bool isValid;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
ArchiveFileSystemBase::IsValid() const
{
    return this->isValid;
}

} // namespace IO
//------------------------------------------------------------------------------
