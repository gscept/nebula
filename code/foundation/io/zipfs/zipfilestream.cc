//------------------------------------------------------------------------------
//  zipfilestream.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "io/zipfs/zipfilestream.h"
#include "io/zipfs/zipfilesystem.h"
#include "io/zipfs/ziparchive.h"
#include "io/zipfs/zipfileentry.h"
#include "io/archfs/archive.h"

namespace IO
{
__ImplementClass(IO::ZipFileStream, 'ZFST', IO::Stream);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
ZipFileStream::ZipFileStream() :
    size(0),
    position(0),
    mapBuffer(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ZipFileStream::~ZipFileStream()
{
    if (this->IsOpen())
    {
        this->Close();
    }
    n_assert(!this->mapBuffer);
}

//------------------------------------------------------------------------------
/**
*/
bool
ZipFileStream::CanRead() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
ZipFileStream::CanWrite() const
{
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
ZipFileStream::CanSeek() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
ZipFileStream::CanBeMapped() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
Stream::Size
ZipFileStream::GetSize() const
{
    return this->size;
}

//------------------------------------------------------------------------------
/**
*/
Stream::Position
ZipFileStream::GetPosition() const
{
    return this->position;
}

//------------------------------------------------------------------------------
/**
    Open the stream for reading. This will decompress the entire file
    from the zip archive into memory.
*/
bool
ZipFileStream::Open()
{
    n_assert(!this->IsOpen());
    n_assert(!this->mapBuffer);
    // allow only read access
    if (ReadAccess == this->accessMode)
    {
        if (Stream::Open())
        {
            // get zip archive which contains the file
            Ptr<ZipArchive> zipArchive = ZipFileSystem::Instance()->FindArchive(this->uri).cast<ZipArchive>();
            if (zipArchive.isvalid())
            {
                Dictionary<String,String> params = this->uri.ParseQuery();
                if (params.Contains("file"))
                {
                    const String& pathInZip = params["file"];
                    String pwd;
                    if (params.Contains("pwd"))
                    {
                        pwd = params["pwd"];
                    }

                    // find the zip file in the archive, the path into
                    // the archive is encoded in the query part of our URI
                    this->zipFileEntry = zipArchive->FindFileEntry(pathInZip);
                    if (0 != this->zipFileEntry)
                    {
                        // read content of zip file entry into private buffer
                        this->size = this->zipFileEntry->GetFileSize();
                        if(!this->zipFileEntry->Open(pwd)) return false;
                        this->position = 0;
                        return true;
                    }
                }
            }
            // fallthrough: failure
            this->Close();
        }
    }    
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
ZipFileStream::Close()
{
    n_assert(this->IsOpen());
    n_assert(this->zipFileEntry);
    if (this->IsMapped())
    {
        this->Unmap();
    }
    Stream::Close();
    this->zipFileEntry->Close();
    this->size = 0;
    this->position = 0;
}

//------------------------------------------------------------------------------
/**
*/
Stream::Size
ZipFileStream::Read(void* ptr, Size numBytes)
{
    n_assert(ptr);
    n_assert(this->IsOpen());
    n_assert(!this->IsMapped()); 
    n_assert(ReadAccess == this->accessMode)
    n_assert((this->position >= 0) && (this->position <= this->size));
    n_assert(this->zipFileEntry);

    // check if end-of-stream is near
    Size readBytes = Math::n_min(numBytes, this->size - this->position);
    n_assert((this->position + readBytes) <= this->size);
    if (readBytes > 0)
    {
        if(!this->zipFileEntry->Read(ptr, readBytes)) return 0;
        this->position += readBytes;
    }
    return readBytes;
}

//------------------------------------------------------------------------------
/**
*/
void
ZipFileStream::Seek(Offset offset, SeekOrigin origin)
{
    n_assert(this->IsOpen());
    n_assert(!this->IsMapped()); 
    n_assert((this->position >= 0) && (this->position <= this->size));

    const Position posBefore = this->position;

    switch (origin)
    {
        case Begin:
            this->position = offset;
            break;
        case Current:
            this->position += offset;
            break;
        case End:
            this->position = this->size + offset;
            break;
        default:
            n_assert(false);
    }

    // make sure read/write position doesn't become invalid
    this->position = Math::n_iclamp(this->position, 0, this->size);

    n_assert(this->position >= posBefore);
    if(this->position == posBefore) return;

    const long readBytes = this->position - posBefore;
    n_assert(readBytes > 0);
    char *dummyBuffer = (char *) Memory::Alloc(Memory::StreamDataHeap, readBytes);
    n_assert(dummyBuffer);
    int read = this->Read(dummyBuffer, readBytes);
    Memory::Free(Memory::StreamDataHeap, dummyBuffer);
    dummyBuffer = NULL;
}

//------------------------------------------------------------------------------
/**
*/
bool
ZipFileStream::Eof() const
{
    n_assert(this->IsOpen());
    n_assert(!this->IsMapped());
    n_assert((this->position >= 0) && (this->position <= this->size));
    return (this->position == this->size);
}

//------------------------------------------------------------------------------
/**
*/
void*
ZipFileStream::Map()
{
    n_assert(this->IsOpen());
    n_assert(ReadAccess == this->accessMode);
    Stream::Map();
    n_assert(this->GetSize() > 0);
    n_assert(!this->mapBuffer);
    this->mapBuffer = (unsigned char*)Memory::Alloc(Memory::StreamDataHeap, this->size);
    n_assert(0 != this->mapBuffer);
    bool success = this->zipFileEntry->Read(this->mapBuffer, this->size);
    n_assert(success);
    return this->mapBuffer;
}

//------------------------------------------------------------------------------
/**
*/
void
ZipFileStream::Unmap()
{
    n_assert(this->IsOpen());
    Stream::Unmap();
    n_assert(this->mapBuffer);
    Memory::Free(Memory::StreamDataHeap, this->mapBuffer);
    this->mapBuffer = NULL;
}

} // namespace IO
