//------------------------------------------------------------------------------
//  filestream.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/filestream.h"

namespace IO
{
__ImplementClass(IO::FileStream, 'FSTR', IO::Stream);

using namespace Util;
using namespace Core;

//------------------------------------------------------------------------------
/**
*/
FileStream::FileStream() :
    handle(0),
    mappedContent(0),
    mapHandle(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FileStream::~FileStream()
{
    if (this->IsOpen())
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
FileStream::CanRead() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
FileStream::CanWrite() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
FileStream::CanSeek() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
    FileStreams support mapping (only read access for now).
*/
bool
FileStream::CanBeMapped() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
Stream::Size
FileStream::GetSize() const
{
    n_assert(0 != this->handle);
    return FSWrapper::GetFileSize(this->handle);
}

//------------------------------------------------------------------------------
/**
*/
Stream::Position
FileStream::GetPosition() const
{
    n_assert(0 != this->handle);
    return FSWrapper::Tell(this->handle);
}

//------------------------------------------------------------------------------
/**
*/
bool
FileStream::Open()
{
    n_assert(!this->IsOpen());
    n_assert(0 == this->handle);
    n_assert(this->uri.Scheme() == "file");
    n_assert(this->uri.LocalPath().IsValid());
    if (Stream::Open())
    {
        this->handle = FSWrapper::OpenFile(this->uri.GetHostAndLocalPath(), this->accessMode, this->accessPattern);
        if (0 != this->handle)
        {
            return true;
        }
    }

    // fallthrough: failure
    Stream::Close();
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
FileStream::Close()
{
    n_assert(this->IsOpen());
    n_assert(0 != this->handle);
    if (this->IsMapped())
    {
        if (this->mapHandle == nullptr)
            this->Unmap();
        else
            this->MemoryUnmap();
    }
    FSWrapper::CloseFile(this->handle);
    this->handle = 0;
    Stream::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
FileStream::Write(const void* ptr, Size numBytes)
{
    n_assert(!this->IsMapped());
    if (numBytes > 0)
    {
        n_assert(this->IsOpen());
        n_assert(0 != this->handle);
        n_assert(0 != ptr);
        FSWrapper::Write(this->handle, ptr, numBytes);
    }
}

//------------------------------------------------------------------------------
/**
*/
Stream::Size
FileStream::Read(void* ptr, Size numBytes)
{
    n_assert(!this->IsMapped());
    n_assert(this->IsOpen());
    n_assert(0 != this->handle);
    n_assert(0 != ptr);
    n_assert(numBytes > 0);
    return FSWrapper::Read(this->handle, ptr, numBytes);
}

//------------------------------------------------------------------------------
/**
*/
void
FileStream::Seek(Offset offset, SeekOrigin origin)
{
    n_assert(!this->IsMapped());
    n_assert(this->IsOpen());
    n_assert(0 != this->handle);
    FSWrapper::Seek(this->handle, offset, origin);
}

//------------------------------------------------------------------------------
/**
*/
void
FileStream::Flush()
{
    n_assert(!this->IsMapped());
    n_assert(this->IsOpen());
    n_assert(0 != this->handle);
    FSWrapper::Flush(this->handle);
}

//------------------------------------------------------------------------------
/**
*/
bool
FileStream::Eof() const
{
    n_assert(!this->IsMapped());
    n_assert(this->IsOpen());
    n_assert(0 != this->handle);
    return FSWrapper::Eof(this->handle);
}

//------------------------------------------------------------------------------
/**
*/
void*
FileStream::Map()
{
    n_assert(0 == this->mappedContent);
    
    Size size = this->GetSize();
    n_assert(size > 0);
    this->mappedContent = Memory::Alloc(Memory::ScratchHeap, size);
    this->Seek(0, Begin);
    Size readSize = this->Read(this->mappedContent, size);
    n_assert(readSize == size);
    Stream::Map();
    return this->mappedContent;
}

//------------------------------------------------------------------------------
/**
*/
void
FileStream::Unmap()
{
    n_assert(0 != this->mappedContent);
    Stream::Unmap();
    Memory::Free(Memory::ScratchHeap, this->mappedContent);
    this->mappedContent = 0;
}

//------------------------------------------------------------------------------
/**
*/
void* 
FileStream::MemoryMap()
{
    n_assert(0 == this->mappedContent);
    this->mappedContent = FSWrapper::Map(this->handle, this->accessMode, this->mapHandle);
    Stream::Map();
    return this->mappedContent;
}

//------------------------------------------------------------------------------
/**
*/
void 
FileStream::MemoryUnmap()
{
    n_assert(0 != this->mappedContent);
    FSWrapper::Unmap(this->mapHandle, (char*)this->mappedContent);
    this->mappedContent = nullptr;
    Stream::Unmap();
}

} // namespace IO
