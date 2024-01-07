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

class IoServer : public Core::RefCounted
{
    __DeclareClass(IoServer);
    __DeclareSingleton(IoServer);
public:
    /// constructor
    IoServer();
    /// destructor
    virtual ~IoServer();

    /// mount a file archive (without archive file extension!)
    bool MountArchive(const URI& uri);
    /// mount an embedded file archive
    bool MountEmbeddedArchive(const URI& uri);
    /// unmount a file archive (without archive file extension!)
    void UnmountArchive(const URI& uri);
    /// return true if a archive is mounted (without archive file extension!)
    bool IsArchiveMounted(const URI& uri) const;
    /// enable/disable transparent archive filesystem layering (default is yes)
    void SetArchiveFileSystemEnabled(bool b);
    /// return true if transparent archive filesystem is enabled
    bool IsArchiveFileSystemEnabled() const;
    /// mount standard archives (e.g. home:export.zip and home:export_$(platform).zip)
    void MountStandardArchives();
    /// unmount standard archives
    void UnmountStandardArchives();

    /// create a stream object for the given uri
    Ptr<Stream> CreateStream(const URI& uri) const;
    /// create all missing directories in the path
    bool CreateDirectory(const URI& uri) const;
    /// delete an empty directory
    bool DeleteDirectory(const URI& path) const;
    /// return true if directory exists
    bool DirectoryExists(const URI& path) const;

    /// copy a file
    bool CopyFile(const URI& from, const URI& to) const;
    /// delete a file
    bool DeleteFile(const URI& path) const;
    /// return true if file exists
    bool FileExists(const URI& path) const;
    /// return if file is locked
    bool IsLocked(const URI& path) const;
    /// set the readonly status of a file
    void SetReadOnly(const URI& path, bool b) const;
    /// return read only status of a file
    bool IsReadOnly(const URI& path) const;
    /// get the CRC checksum of a file
    unsigned int ComputeFileCrc(const URI& path) const;
    /// set the write-time of a file
    void SetFileWriteTime(const URI& path, FileTime fileTime);
    /// return the last write-time of a file
    FileTime GetFileWriteTime(const URI& path) const;
    /// read contents of file and return as string
    static bool ReadFile(const URI& path, Util::String & contents);
    /// return native path
    static Util::String NativePath(const Util::String& path);

    /// list all files matching a pattern in a directory
    Util::Array<Util::String> ListFiles(const URI& dir, const Util::String& pattern, bool asFullPath=false) const;
    /// list all subdirectories matching a pattern in a directory
    Util::Array<Util::String> ListDirectories(const URI& dir, const Util::String& pattern, bool asFullPath=false) const;

    /// create a temporary file name
    URI CreateTemporaryFilename(const URI& path) const;

private:
    /// helper function to add path prefix to file or dir names in array
    Util::Array<Util::String> AddPathPrefixToArray(const Util::String& prefix, const Util::Array<Util::String>& filenames) const;

    bool archiveFileSystemEnabled;    
    Ptr<ArchiveFileSystem> archiveFileSystem;
    static Threading::CriticalSection archiveCriticalSection;
    static bool StandardArchivesMounted;
    
    Ptr<Http::HttpClientRegistry> httpClientRegistry;
    Ptr<AssignRegistry> assignRegistry;
    Ptr<SchemeRegistry> schemeRegistry;
    Ptr<FileWatcher> watcher;
    Ptr<StreamCache> streamCache;
    static Threading::CriticalSection assignCriticalSection;
    static Threading::CriticalSection schemeCriticalSection;
    static Threading::CriticalSection watcherCriticalSection;
};

//------------------------------------------------------------------------------
/**
*/
inline void
IoServer::SetArchiveFileSystemEnabled(bool b)
{
    this->archiveFileSystemEnabled = b;
}

//------------------------------------------------------------------------------
/**
    NOTE: on platforms which provide transparent archive access through
    the OS (like on PS3) this method will always return false. This saves
    some unecessary overhead in the Nebula IoServer.
*/
inline bool
IoServer::IsArchiveFileSystemEnabled() const
{
    return this->archiveFileSystemEnabled && ArchiveFileSystem::Instance()->HasArchives();
}

} // namespace IO
//------------------------------------------------------------------------------
