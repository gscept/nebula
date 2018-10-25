//------------------------------------------------------------------------------
//  memorystream.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/memorystream.h"

namespace IO
{
__ImplementClass(IO::MemoryStream, 'MSTR', IO::Stream);

//------------------------------------------------------------------------------
/**
*/
MemoryStream::MemoryStream() :
    capacity(0),
    size(0),
    position(0),
    buffer(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
MemoryStream::~MemoryStream()
{
    // close the stream if still open
    if (this->IsOpen())
    {
        this->Close();
    }

    // release memory buffer if allocated
    if (0 != this->buffer)
    {
        Memory::Free(Memory::StreamDataHeap, this->buffer);
        this->buffer = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
MemoryStream::CanRead() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
MemoryStream::CanWrite() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
MemoryStream::CanSeek() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
MemoryStream::CanBeMapped() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
MemoryStream::SetSize(Size s)
{
    if (s > this->capacity)
    {
        this->Realloc(s);
    }
    this->size = s;
}

//------------------------------------------------------------------------------
/**
*/
Stream::Size
MemoryStream::GetSize() const
{
    return this->size;
}

//------------------------------------------------------------------------------
/**
*/
Stream::Position
MemoryStream::GetPosition() const
{
    return this->position;
}

//------------------------------------------------------------------------------
/**
    Open the stream for reading or writing. The stream may already contain
    data if it has been opened/closed before. 
*/
bool
MemoryStream::Open()
{
    n_assert(!this->IsOpen());
    
    // nothing to do here, allocation happens in the first Write() call
    // if necessary, all we do is reset the read/write position to the
    // beginning of the stream
    if (Stream::Open())
    {
        if (WriteAccess == this->accessMode)
        {
            this->position = 0;
        }
        else if (AppendAccess == this->accessMode)
        {
            this->position = this->size;
        }
        else
        {
            this->position = 0;
        }
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Close the stream. The contents of the stream will remain intact until
    destruction of the object, so that the same data may be accessed 
    or modified during a later session. 
*/
void
MemoryStream::Close()
{
    n_assert(this->IsOpen());
    if (this->IsMapped())
    {
        this->Unmap();
    }
    Stream::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
MemoryStream::Write(const void* ptr, Size numBytes)
{
    n_assert(this->IsOpen());
    n_assert(!this->IsMapped()); 
    n_assert((WriteAccess == this->accessMode) || (AppendAccess == this->accessMode) || (ReadWriteAccess == this->accessMode))
    n_assert((this->position >= 0) && (this->position <= this->size));

    // if not enough room, allocate more memory
    if (!this->HasRoom(numBytes))
    {
        this->MakeRoom(numBytes);
    }

    // write data to stream
    n_assert((this->position + numBytes) <= this->capacity);
    Memory::Copy(ptr, this->buffer + this->position, numBytes);
    this->position += numBytes;
    if (this->position > this->size)
    {
        this->size = this->position;
    }
}

//------------------------------------------------------------------------------
/**
*/
Stream::Size
MemoryStream::Read(void* ptr, Size numBytes)
{
    n_assert(this->IsOpen());
    n_assert(!this->IsMapped()); 
    n_assert((ReadAccess == this->accessMode) || (ReadWriteAccess == this->accessMode))
    n_assert((this->position >= 0) && (this->position <= this->size));

    // check if end-of-stream is near
    Size readBytes = numBytes <= this->size - this->position ? numBytes : this->size - this->position;
    n_assert((this->position + readBytes) <= this->size);
    if (readBytes > 0)
    {
        Memory::Copy(this->buffer + this->position, ptr, readBytes);
        this->position += readBytes;
    }
    return readBytes;
}

//------------------------------------------------------------------------------
/**
*/
void
MemoryStream::Seek(Offset offset, SeekOrigin origin)
{
    n_assert(this->IsOpen());
    n_assert(!this->IsMapped()); 
    n_assert((this->position >= 0) && (this->position <= this->size));
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
    }

    // make sure read/write position doesn't become invalid
    this->position = Math::n_iclamp(this->position, 0, this->size);
}

//------------------------------------------------------------------------------
/**
*/
bool
MemoryStream::Eof() const
{
    n_assert(this->IsOpen());
    n_assert(!this->IsMapped());
    n_assert((this->position >= 0) && (this->position <= this->size));
    return (this->position == this->size);
}

//------------------------------------------------------------------------------
/**
*/
bool
MemoryStream::HasRoom(Size numBytes) const
{
    return ((this->position + numBytes) <= this->capacity);
}

//------------------------------------------------------------------------------
/**
    This (re-)allocates the memory buffer to a new size. If the new size
    is smaller then the existing size, the buffer contents will be clipped.
*/
void
MemoryStream::Realloc(Size newCapacity)
{
    unsigned char* newBuffer = (unsigned char*) Memory::Alloc(Memory::StreamDataHeap, newCapacity);
    unsigned char* endOfNewBuffer = newBuffer + newCapacity;
    n_assert(0 != newBuffer);
    int newSize = newCapacity < this->size ? newCapacity : this->size;
    if (0 != this->buffer)
    {
        n_assert((newBuffer + newSize) < endOfNewBuffer);
        Memory::Copy(this->buffer, newBuffer, newSize);
        Memory::Free(Memory::StreamDataHeap, this->buffer);
    }
    this->buffer = newBuffer;
    this->size = newSize;
    this->capacity = newCapacity;
    if (this->position > this->size)
    {
        this->position = this->size;
    }
}

//------------------------------------------------------------------------------
/**
    This method makes room for at least N more bytes. The actually allocated
    memory buffer will be greater then that. This operation involves a copy
    of existing data.
*/
void
MemoryStream::MakeRoom(Size numBytes)
{
    n_assert(numBytes > 0);
    n_assert((this->size + numBytes) > this->capacity);

    // compute new capacity
    Size oneDotFiveCurrentSize = this->capacity + (this->capacity >> 1);
    Size newCapacity = this->size + numBytes;
    if (oneDotFiveCurrentSize > newCapacity)
    {
        newCapacity = oneDotFiveCurrentSize;
    }
    if (16 > newCapacity)
    {
        newCapacity = 16;
    }
    n_assert(newCapacity > this->capacity);

    // (re-)allocate memory buffer
    this->Realloc(newCapacity);
}

//------------------------------------------------------------------------------
/**
    Map the stream for direct memory access. This is much faster then 
    reading/writing, but less flexible. A mapped stream cannot grow, instead
    the allowed memory range is determined by GetSize(). The read/writer must 
    take special care to not read or write past the memory buffer boundaries!
*/
void*
MemoryStream::Map()
{
    n_assert(this->IsOpen());
    Stream::Map();
    n_assert(this->GetSize() > 0);
    return this->buffer;
}

//------------------------------------------------------------------------------
/**
    Unmap a memory-mapped stream.
*/
void
MemoryStream::Unmap()
{
    n_assert(this->IsOpen());
    Stream::Unmap();
}

//------------------------------------------------------------------------------
/**
    Get a direct pointer to the raw data. This is a convenience method
    and only works for memory streams.
    NOTE: writing new data to the stream may/will result in an invalid
    pointer, don't keep the returned pointer around between writes!
*/
void*
MemoryStream::GetRawPointer() const
{
    n_assert(0 != this->buffer);
    return this->buffer;
}

} // namespace IO
