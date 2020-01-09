#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::ZipArchive
    
    Private helper class for ZipFileSystem to hold per-Zip-archive data.
    Uses the zlib and the minizip lib for zip file access.
    
    Multithreading: access to zlib archives needs to be serialized. A
    ZipArchive objects contains a critical section which it will hand down
    to ZipFileEntry objects.

    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "io/archfs/archivebase.h"
#include "minizip/unzip.h"
#include "io/zipfs/zipfileentry.h"
#include "io/zipfs/zipdirentry.h"

//------------------------------------------------------------------------------
namespace IO
{
class ZipArchive : public ArchiveBase
{
    __DeclareClass(ZipArchive);
public:
    /// constructor
    ZipArchive();
    /// destructor
    virtual ~ZipArchive();

    /// setup the archive from an URI
    bool Setup(const URI& uri);
    /// discard the archive
    void Discard();

    /// list all files in a directory in the archive
    Util::Array<Util::String> ListFiles(const Util::String& dirPathInArchive, const Util::String& pattern) const;
    /// list all subdirectories in a directory in the archive
    Util::Array<Util::String> ListDirectories(const Util::String& dirPathInArchive, const Util::String& pattern) const;
    /// convert a "file:" URI into a "zip:" URI pointing into this archive
    URI ConvertToArchiveURI(const URI& fileURI) const;
    /// convert an absolute path to local path inside archive, returns empty string if absPath doesn't point into this archive
    Util::String ConvertToPathInArchive(const Util::String& absPath) const;

private:
    friend class ZipFileSystem;
    friend class ZipFileStream;

    /// parse the table of contents into memory
    void ParseTableOfContents();
    /// add a new file entry, create missing dir entries on the way
    void AddEntry(const Util::String& path);
    /// find a file entry in the zip archive, return 0 if not exists
    const ZipFileEntry* FindFileEntry(const Util::String& pathInZipArchive) const;
    /// find a file entry in the zip archive, return 0 if not exists
    ZipFileEntry* FindFileEntry(const Util::String& pathInZipArchive);
    /// find a directory entry in the zip archive, return 0 if not exists
    const ZipDirEntry* FindDirEntry(const Util::String& pathInZipArchive) const;

    Util::String rootPath;                      // location of the zip archive file
    unzFile zipFileHandle;                      // the zip file handle
    ZipDirEntry rootEntry;                      // the root entry of the zip archive
    Threading::CriticalSection archiveCritSect; // need to serialize access to archive from multiple threads!
};

} // namespace IO
//------------------------------------------------------------------------------
    