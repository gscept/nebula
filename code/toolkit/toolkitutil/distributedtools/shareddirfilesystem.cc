//------------------------------------------------------------------------------
//  shareddirfilesystem.cc
//  (C) 2009 RadonLabs GmbH
//  (C) 2013 - 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "shareddirfilesystem.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "io/console.h"

using namespace IO;
using namespace Util;

namespace DistributedTools
{
    __ImplementClass(DistributedTools::SharedDirFileSystem,'SDFS',DistributedTools::SharedDirControl)
//------------------------------------------------------------------------------
/**
    Constructor	
*/
SharedDirFileSystem::SharedDirFileSystem()
{
    SharedDirControl::SharedDirControl();
}

//------------------------------------------------------------------------------
/**
    Destructor	
*/
SharedDirFileSystem::~SharedDirFileSystem()
{
}

//------------------------------------------------------------------------------
/**
    copy a single file to shared dir (src being absolute path, dst relative to control dir)
*/
void
SharedDirFileSystem::CopyFileToSharedDir(const Util::String & src, const Util::String & dst)
{
    String dstPath;
    dstPath.Format("%s/%s",
        this->sharedDir.AsString().AsCharPtr(),
        dst.AsCharPtr()
    );
    if(!IoServer::Instance()->CopyFile(src, dstPath))
    {
        Console::Instance()->Error("Could not copy %s to %s.", src.AsCharPtr(), dstPath.AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
    copy all files from src to shared dir (not recursive, only toplevel)
    (src being absolute path, dst relative to control dir)
*/
void
SharedDirFileSystem::CopyFilesToSharedDir(const Util::String & src, const Util::String & dst)
{
    Array<String> files = IoServer::Instance()->ListFiles(src, "*");
    IndexT index;
    for(index = 0; index < files.Size(); index++)
    {
        String srcpath;
        srcpath.Format("%s/%s", src.AsCharPtr(), files[index].AsCharPtr());
        this->CopyFileToSharedDir(srcpath, dst);
    }
}

//------------------------------------------------------------------------------
/**
    copy everything (including sub dirs) from src into
    shared dirs guid folder (src being absolute path)
*/
void
SharedDirFileSystem::CopyDirectoryContentToSharedDir(const Util::String & src)
{
    this->CopyDirectoryContent(src, this->sharedDir.AsString());
}

//------------------------------------------------------------------------------
/**
    copy a single file from shared dir to dst (dst being absolute, src relative to control dir)
*/
void
SharedDirFileSystem::CopyFileFromSharedDir(const Util::String & src, const Util::String & dst)
{
    String srcPath;
    srcPath.Format("%s/%s",
        this->sharedDir.AsString().AsCharPtr(),
        src.AsCharPtr()
    );
    if(!IoServer::Instance()->CopyFile(srcPath, dst))
    {
        Console::Instance()->Error("Could not copy %s to %s.", srcPath.AsCharPtr(), dst.AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
    copy all files from shared dir sub dir to dst (not recursive, only toplevel)
    (dst being absolute, src relative to control dir)
*/
void
SharedDirFileSystem::CopyFilesFromSharedDir(const Util::String & src, const Util::String & dst)
{
    Array<String> files = this->ListFiles(src);
    IndexT index;
    for(index = 0; index < files.Size(); index++)
    {
        String srcpath;
        srcpath.Format("%s/%s", src.AsCharPtr(), files[index].AsCharPtr());
        this->CopyFileFromSharedDir(srcpath, dst);
    }
}

//------------------------------------------------------------------------------
/**
    copy everything (including sub dirs) from shared dirs guid folder
    into dst (dst being absolute path)
*/
void
SharedDirFileSystem::CopyDirectoryContentFromSharedDir(const Util::String & dst)
{
    this->CopyDirectoryContent(this->sharedDir.AsString(), dst);
}

//------------------------------------------------------------------------------
/**
    removes specified file (relative to control dir)
*/
void
SharedDirFileSystem::RemoveFileInSharedDir(const Util::String & filepath)
{
    String path;
    path.Format("%s/%s",
        this->sharedDir.AsString().AsCharPtr(),
        filepath.AsCharPtr()
    );
    if(!IoServer::Instance()->DeleteFile(path))
    {
        Console::Instance()->Error("Could not delete file %s.", path.AsCharPtr());
    }
}
    
//------------------------------------------------------------------------------
/**
    removes specified directory if empty (relative to control dir)
*/
void
SharedDirFileSystem::RemoveDirectoryInSharedDir(const Util::String & dirpath)
{
    n_assert(this->sharedDir.IsValid());
    String path;
    path.Format("%s/%s",
        this->sharedDir.AsString().AsCharPtr(),
        dirpath.AsCharPtr()
    );
    if(!IoServer::Instance()->DeleteDirectory(path))
    {
        Console::Instance()->Error("Could not delete directory %s.", path.AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
    remove all files and directories of specified
    directory (relative to control dir) recursively 
*/
void
SharedDirFileSystem::RemoveDirectoryContent(const Util::String & path)
{
    Array<String> files = IoServer::Instance()->ListFiles(path, "*");
    IndexT fileIndex;
    for(fileIndex = 0; fileIndex < files.Size(); fileIndex++)
    {
        String file;
        file.Format("%s/%s", path.AsCharPtr(), files[fileIndex].AsCharPtr());
        IoServer::Instance()->DeleteFile(file);
    }
    Array<String> dirs = IoServer::Instance()->ListDirectories(path, "*");
    IndexT dirIndex;
    for(dirIndex = 0; dirIndex < dirs.Size(); dirIndex++)
    {
        String dir;
        dir.Format("%s/%s", path.AsCharPtr(), dirs[dirIndex].AsCharPtr());
        this->RemoveDirectoryContent(dir);
        IoServer::Instance()->DeleteDirectory(dir);
    }
}

//------------------------------------------------------------------------------
/**
    checks if a subdirectory exists, which name equals the given guid
*/
bool
SharedDirFileSystem::ContainsGuidSubDir(const Util::Guid & guid)
{
    String path;
    path.Format("%s/%s",
        this->path.AsString().AsCharPtr(),
        guid.AsString().AsCharPtr()
    );
    return IoServer::Instance()->DirectoryExists(path);
}

//------------------------------------------------------------------------------
/**
    checks if directory is empty (relative to control dir)
*/
bool
SharedDirFileSystem::DirIsEmpty(const Util::String & dir)
{
    String path;
    path.Format("%s/%s",
        this->sharedDir.AsString().AsCharPtr(),
        dir.AsCharPtr()
    );
    Array<String> files = IoServer::Instance()->ListFiles(path, "*");
    Array<String> dirs = IoServer::Instance()->ListDirectories(path, "*");
    return (files.IsEmpty() && dirs.IsEmpty());
}
    
//------------------------------------------------------------------------------
/**
    returns all subdirectories of specified directory (relative to control dir)
*/
Util::Array<Util::String>
SharedDirFileSystem::ListDirectories(const Util::String & dir)
{
    String path;
    path.Format("%s/%s",
        this->sharedDir.AsString().AsCharPtr(),
        dir.AsCharPtr()
    );
    return IoServer::Instance()->ListDirectories(path, "*");
}

//------------------------------------------------------------------------------
/**
    returns all files inside the specified directory (relative to control dir)
*/
Util::Array<Util::String>
SharedDirFileSystem::ListFiles(const Util::String & dir)
{
    String path;
    path.Format("%s/%s",
        this->sharedDir.AsString().AsCharPtr(),
        dir.AsCharPtr()
    );
    return IoServer::Instance()->ListFiles(path, "*");
}

//------------------------------------------------------------------------------
/**
    creates control directory from guid
*/
void
SharedDirFileSystem::CreateControlDir()
{
    if(!IoServer::Instance()->CreateDirectory(this->sharedDir))
    {
        Console::Instance()->Error("Could not create control dir in %s.", this->path.LocalPath().AsCharPtr());
    }
}
    
//------------------------------------------------------------------------------
/**
    removes control dir
*/
void
SharedDirFileSystem::RemoveControlDir()
{
    // We assume that shared dir is composed of root shared directory
    // and guid sub dir. DeleteDirectory will only remove the subdir (guid).
    if(!IoServer::Instance()->DeleteDirectory(this->sharedDir))
    {
        Console::Instance()->Error("Could not delete control dir in %s.", this->sharedDir.AsString().ExtractToLastSlash().AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
	copy all content from src to dst recursively 
*/
void
SharedDirFileSystem::CopyDirectoryContent(const Util::String & src, const Util::String & dst)
{
    Array<String> files = IoServer::Instance()->ListFiles(src, "*");
    IndexT fileIndex;
    for(fileIndex = 0; fileIndex < files.Size(); fileIndex++)
    {
        String srcPath;
        String dstPath;
        srcPath.Format("%s/%s", src.AsCharPtr(), files[fileIndex].AsCharPtr());
        dstPath.Format("%s/%s", dst.AsCharPtr(), files[fileIndex].AsCharPtr());
        IoServer::Instance()->CopyFile(srcPath, dstPath);
    }
    Array<String> dirs = IoServer::Instance()->ListDirectories(src, "*");
    IndexT dirIndex;
    for(dirIndex = 0; dirIndex < dirs.Size(); dirIndex++)
    {
        String srcDir;
        String dstDir;
        srcDir.Format("%s/%s", src.AsCharPtr(), dirs[dirIndex].AsCharPtr());
        dstDir.Format("%s/%s", dst.AsCharPtr(), dirs[dirIndex].AsCharPtr());
        IoServer::Instance()->CreateDirectory(dstDir);
        this->CopyDirectoryContent(srcDir, dstDir);
    }
}

} // namespace DistributedTools
