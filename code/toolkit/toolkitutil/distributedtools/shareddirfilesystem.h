#pragma once
//------------------------------------------------------------------------------
/**
	@class DistributedTools::SharedDirFileSystem

    A helper class that encapsulates all administrational activities for
    the shared directory, used by slave applications in distributed build.
    
    This is a subclass of SharedDirControl. These operations are only
    applicable for a shared directory in a regular file system.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "distributedtools/shareddircontrol.h"
#include "util/guid.h"
#include "io/uri.h"

//------------------------------------------------------------------------------
namespace DistributedTools
{
class SharedDirFileSystem : public DistributedTools::SharedDirControl
{
__DeclareClass(DistributedTools::SharedDirFileSystem)
public:
    /// constructor
    SharedDirFileSystem();
    /// destructor
    ~SharedDirFileSystem();

    /// copy a single file to shared dir (src being absolute path, dst relative to control dir)
    void CopyFileToSharedDir(const Util::String & src, const Util::String & dst);
    /// copy all files from src to shared dir (src being absolute path, dst relative to control dir)
    void CopyFilesToSharedDir(const Util::String & src, const Util::String & dst);
    /// copy everything from src to shared dir (src being absolute path)
    void CopyDirectoryContentToSharedDir(const Util::String & src);

    /// copy a single file from shared dir to dst (dst being absolute, src relative to control dir)
    void CopyFileFromSharedDir(const Util::String & src, const Util::String & dst);
    /// copy all files from shared dir sub dir to dst (dst being absolute, src relative to control dir)
    void CopyFilesFromSharedDir(const Util::String & src, const Util::String & dst);
    /// copy everything from shared dir sub dir to dst (dst being absolute)
    void CopyDirectoryContentFromSharedDir(const Util::String & dst);

    /// removes specified file (relative to control dir)
    void RemoveFileInSharedDir(const Util::String & filepath);
    /// removes specified directory if empty (relative to control dir)
    void RemoveDirectoryInSharedDir(const Util::String & dirpath);
    /// removes all content inside of given directory (relative to control dir)
    void RemoveDirectoryContent(const Util::String & dir);

    /// checks if a subdirectory exists, which name equals the given guid
    bool ContainsGuidSubDir(const Util::Guid & guid);
    /// checks if directory is empty (relative to control dir)
    bool DirIsEmpty(const Util::String & dir);
    /// returns all subdirectories of specified directory (relative to control dir)
    Util::Array<Util::String> ListDirectories(const Util::String & dir);
    /// returns all files inside the specified directory (relative to control dir)
    Util::Array<Util::String> ListFiles(const Util::String & dir);

private:
    /// creates control directory from guid
    void CreateControlDir();
    /// removes control dir
    void RemoveControlDir();
    /// copy all content from src to dst recursively
    void CopyDirectoryContent(const Util::String & src, const Util::String & dst);

};

} // namespace DistributedTools
