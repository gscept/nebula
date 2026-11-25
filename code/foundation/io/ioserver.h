#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::IoServer
  
    The central server object of the IO subsystem offers the following
    services:

    * associate stream classes with URI schemes
    * create the right stream object for a given URI
    * transparant (ZIP) archive support
    * path assign management
    * global filesystem manipulation and query methods
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "util/array.h"
#include "util/string.h"
#include "io/filetime.h"
#include "io/schemeregistry.h"
#include "archfs/archivefilesystem.h"
#include "io/cache/streamcache.h"

namespace Http
{
    class HttpClientRegistry;
};

//------------------------------------------------------------------------------
namespace IO
{
class ArchiveFileSystem;
class FileWatcher;
class Stream;
class URI;
class AssignRegistry;

struct FilesystemStartupInfo
{

};

/// Setup the file/stream system
void StartupFilesystem(const FilesystemStartupInfo& info);
/// Break down the filesystem
void ShutdownFilesystem();

/// mount a file archive (without archive file extension!)
bool MountArchive(const URI& uri);
/// mount an embedded file archive
bool MountEmbeddedArchive(const URI& uri);
/// unmount a file archive (without archive file extension!)
void UnmountArchive(const URI& uri);
/// return true if a archive is mounted (without archive file extension!)
bool IsArchiveMounted(const URI& uri);
/// enable/disable transparent archive filesystem layering (default is yes)
void SetArchiveFileSystemEnabled(bool b);
/// return true if transparent archive filesystem is enabled
bool IsArchiveFileSystemEnabled();
/// mount standard archives (e.g. home:export.zip and home:export_$(platform).zip)
void MountStandardArchives();
/// unmount standard archives
void UnmountStandardArchives();

/// create a stream object for the given uri
Ptr<Stream> CreateStream(const URI& uri);
/// create all missing directories in the path
bool CreateDirectory(const URI& uri);
/// create all missing directories
bool EnsureDirectoriesForFile(const URI& uri);
/// delete an empty directory
bool DeleteDirectory(const URI& path);
/// return true if directory exists
bool DirectoryExists(const URI& path);

/// copy a file
bool CopyFile(const URI& from, const URI& to);
/// rename a file or a directory. The target must not exist as the exact name, so a rename never replaces
bool RenameFile(const URI& from, const URI& to);
/// delete a file
bool DeleteFile(const URI& path);
/// return true if file exists
bool FileExists(const URI& path);
/// return if file is locked
bool IsLocked(const URI& path);
/// Move file to new path
bool MoveFile(const URI& oldPath, const URI& newPath);
/// set the readonly status of a file
void SetReadOnly(const URI& path, bool b);
/// return read only status of a file
bool IsReadOnly(const URI& path);
/// get the CRC checksum of a file
unsigned int ComputeFileCrc(const URI& path);
/// set the write-time of a file
void SetFileWriteTime(const URI& path, FileTime fileTime);
/// return the last write-time of a file
FileTime GetFileWriteTime(const URI& path);
/// read contents of file and return as string
bool ReadFile(const URI& path, Util::String& contents);
/// return native path
Util::String NativePath(const Util::String& path);

/// get file/folder information (size, access time, modified time, creation time) via stat calls
bool GetIOInfo(const URI& uri, IOStat& outInfo, bool prioritizeArchive);

/// list all files matching a pattern in a directory
Util::Array<Util::String> ListFiles(const URI& dir, const Util::String& pattern, bool asFullPath = false);
/// list all subdirectories matching a pattern in a directory
Util::Array<Util::String> ListDirectories(const URI& dir, const Util::String& pattern, bool asFullPath = false, bool prioritizeArchive = true);

using FileMapFunc = void(const Util::String& file, const Util::String& folder);
/// iterate through all files in hierarchy and call provided function
void IterateFolders(const URI& dir, const Util::String& pattern, FileMapFunc iter);

/// create a temporary file name
URI CreateTemporaryFilename(const URI& path);

} // namespace IO
//------------------------------------------------------------------------------
