//------------------------------------------------------------------------------
//  cachedstream.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/cache/cachedstream.h"
#include "io/cache/streamcache.h"

namespace IO
{
//------------------------------------------------------------------------------
/**
*/
CachedStream::CachedStream() :
    position(0),
    buffer(nullptr)
{
    accessMode = ReadAccess;
}

//------------------------------------------------------------------------------
/**
*/
CachedStream::~CachedStream()
{
    // close the stream if still open
    if (this->IsOpen())
    {
        this->Close();
    }
    this->parentStream = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
bool
CachedStream::CanRead() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
CachedStream::CanSeek() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
CachedStream::CanBeMapped() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
*/
Stream::Size
CachedStream::GetSize() const
{
    n_assert(this->parentStream.isvalid());
    return this->parentStream->GetSize();
}

//------------------------------------------------------------------------------
/**
*/
Stream::Position
CachedStream::GetPosition() const
{
    return this->position;
}

//------------------------------------------------------------------------------
/**
  
*/
bool
CachedStream::Open()
{
    n_assert(!this->IsOpen());
    n_assert(this->accessMode == ReadAccess);

    if (Stream::Open())
    {
        if(!StreamCache::Instance()->IsCached(this->uri))
        {
            if (!StreamCache::Instance()->OpenStream(this->uri, this->GetParentRtti()))
            {
                return false;
            }
        }

        Util::KeyValuePair<void*, Ptr<IO::Stream>> entry = StreamCache::Instance()->GetCachedStream(this->uri);
        this->parentStream = entry.Value();
        n_assert(this->parentStream.isvalid());
        this->position = 0;
        this->buffer = (unsigned char*)entry.Key();
        this->size = this->parentStream->GetSize();
        return true;
    }
    return false;
}

/**
  
*/
void
CachedStream::Close()
{
    if (this->IsMapped())
    {
        this->Unmap();
    }
    Stream::Close();
    if (StreamCache::Instance()->IsCached(this->uri))
    {
        StreamCache::Instance()->RemoveStream(this->uri);
    }
}

//------------------------------------------------------------------------------
/**
*/
Stream::Size
CachedStream::Read(void* ptr, Size numBytes)
{
    n_assert(this->IsOpen());
    n_assert(!this->IsMapped()); 
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
CachedStream::Seek(Offset offset, SeekOrigin origin)
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
CachedStream::Eof() const
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
CachedStream::Map()
{
    n_assert(this->IsOpen());
    Stream::Map();
    n_assert(this->GetSize() > 0);
    return this->buffer;
}

//------------------------------------------------------------------------------
/**
  
*/
void*
CachedStream::MemoryMap()
{
    return this->Map();
}

//------------------------------------------------------------------------------
/**
    Unmap a memory-mapped stream.
*/
void
CachedStream::Unmap()
{
    n_assert(this->IsOpen());
    Stream::Unmap();
}

//------------------------------------------------------------------------------
/**
    Get a direct pointer to the raw data. This is a convenience method
    and only works for memory streams.
*/
void*
CachedStream::GetRawPointer() const
{
    n_assert(0 != this->buffer);
    return this->buffer;
}

} // namespace IO
