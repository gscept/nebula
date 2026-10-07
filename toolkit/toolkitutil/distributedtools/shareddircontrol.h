#pragma once
//------------------------------------------------------------------------------
/**
    @class DistributedTools::SharedDirControl

    A helper class that encapsulates all administrational activities for
    the shared directory, used by slave applications in distributed build.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/guid.h"
#include "io/uri.h"

//------------------------------------------------------------------------------
namespace DistributedTools
{
class SharedDirControl : public Core::RefCounted
{
__DeclareClass(DistributedTools::SharedDirControl)
public:
    /// constructor
    SharedDirControl();
    /// destructor
    virtual ~SharedDirControl();

    /// copy a single file to shared dir (src being absolute path, dst relative to control dir)
    virtual void CopyFileToSharedDir(const Util::String & src, const Util::String & dst);
    /// copy all files from src to shared dir (src being absolute path, dst relative to control dir)
    virtual void CopyFilesToSharedDir(const Util::String & src, const Util::String & dst);
    /// copy everything from src to shared dir (src being absolute path)
    virtual void CopyDirectoryContentToSharedDir(const Util::String & src);

    /// copy a single file from shared dir to dst (dst being absolute, src relative to control dir)
    virtual void CopyFileFromSharedDir(const Util::String & src, const Util::String & dst);
    /// copy all files from shared dir sub dir to dst (dst being absolute, src relative to control dir)
    virtual void CopyFilesFromSharedDir(const Util::String & src, const Util::String & dst);
    /// copy everything from shared dir sub dir to dst (dst being absolute)
    virtual void CopyDirectoryContentFromSharedDir(const Util::String & dst);

    /// removes specified file (relative to control dir)
    virtual void RemoveFileInSharedDir(const Util::String & filepath);
    /// removes specified directory if empty (relative to control dir)
    virtual void RemoveDirectoryInSharedDir(const Util::String & dirpath);
    /// removes all content inside of given directory (relative to control dir)
    virtual void RemoveDirectoryContent(const Util::String & dir);

    /// checks if directory is empty (relative to control dir)
    virtual bool DirIsEmpty(const Util::String & dir);
    /// checks if a subdirectory exists, which name equals the given guid
    virtual bool ContainsGuidSubDir(const Util::Guid & guid);
    /// returns all subdirectories of specified directory (relative to control dir)
    virtual Util::Array<Util::String> ListDirectories(const Util::String & dir);
    /// returns all files inside the specified directory (relative to control dir)
    virtual Util::Array<Util::String> ListFiles(const Util::String & dir);

    /// set up shared directory (creates subdir from guid)
    void SetUp();
    /// cleans up shared directory (removes everything inside)
    void CleanUp();

    /// checks if given controlfile exists
    bool ControlFileEquals(const Util::String & path);
    /// get guid (used for shared sub directory)
    const Util::Guid & GetGuid();
    /// set guid (used for shared sub directory)
    void SetGuid(const Util::Guid & guid);
    /// get path of shared directory
    const IO::URI & GetPath();
    /// set path of shared directory
    void SetPath(const IO::URI & path);

protected:
    /// creates control directory from guid
    virtual void CreateControlDir();
    /// removes control dir
    virtual void RemoveControlDir();

    Util::Guid guid;
    IO::URI sharedDir;
    IO::URI path;

private:

};

//------------------------------------------------------------------------------
/**
    get guid (used for shared sub directory)
*/
inline
const Util::Guid &
SharedDirControl::GetGuid()
{
    return this->guid;
}
//------------------------------------------------------------------------------
/**
    set guid (used for shared sub directory)
*/
inline
void
SharedDirControl::SetGuid(const Util::Guid & guid)
{
    this->guid = guid;
}

//------------------------------------------------------------------------------
/**
    get path of shared directory  
*/
inline
const IO::URI &
SharedDirControl::GetPath()
{
    return this->path;
}
//------------------------------------------------------------------------------
/**
    set path of shared directory
*/
inline
void
SharedDirControl::SetPath(const IO::URI & path)
{
    this->path = path;
}

} // namespace DistributedTools
