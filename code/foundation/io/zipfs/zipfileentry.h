#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::ZipFileEntry
  
    A file entry in a zip archive. The ZipFileEntry class is thread-safe,
    all public methods can be invoked from on the same object from different
    threads.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/    
#include "io/stream.h"
#include "minizip/unzip.h"
#include "util/stringatom.h"

//------------------------------------------------------------------------------
namespace IO
{
class ZipFileEntry
{
public:
    /// constructor
    ZipFileEntry();
    /// destructor
    ~ZipFileEntry();
    
    /// get name of the file entry
    const Util::StringAtom& GetName() const;
    /// get the uncompressed file size in bytes
    IO::Stream::Size GetFileSize() const;

    /// open the zip file
    bool Open(const Util::String& password = "");
    /// close the zip file
    void Close();
    /// read the *entire* content into the provided memory buffer
    bool Read(void* buf, IO::Stream::Size bufSize) const;

private:
    friend class ZipArchive;
    
    /// setup the file entry object
    void Setup(const Util::StringAtom& name, unzFile zipFileHandle, Threading::CriticalSection* critSect);

    Threading::CriticalSection* archiveCritSect;
    Util::StringAtom name;
    unzFile zipFileHandle;    // handle on zip file
    unz_file_pos filePosInfo; // info about position in zip file
    uint uncompressedSize;    // uncompressed size of the file
};

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
ZipFileEntry::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline IO::Stream::Size
ZipFileEntry::GetFileSize() const
{
    return this->uncompressedSize;
}

} // namespace IO
//------------------------------------------------------------------------------

