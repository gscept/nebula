#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::CachedStream
  
    Wraps an underlying mappable stream object to avoid reopening it more than 
    once (e. g. httpstreams)
    
    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/    
#include "core/config.h"
#include "io/stream.h"

//------------------------------------------------------------------------------
namespace IO
{
class StreamCache;
class CachedStream : public IO::Stream
{
public:
    /// constructor
    CachedStream();
    /// destructor
    virtual ~CachedStream();
    
    /// memory streams support reading
    virtual bool CanRead() const;
    /// memory streams support seeking
    virtual bool CanSeek() const;
    /// memory streams are mappable
    virtual bool CanBeMapped() const;
    /// get the size of the stream in bytes
    virtual Size GetSize() const;
    /// get the current position of the read/write cursor
    virtual Position GetPosition() const;
    /// open the stream
    virtual bool Open();
    /// close the stream
    virtual void Close();
    /// directly read from the stream
    virtual Size Read(void* ptr, Size numBytes);
    /// seek in stream
    virtual void Seek(Offset offset, SeekOrigin origin);
    /// return true if end-of-stream reached
    virtual bool Eof() const;
    /// map for direct memory-access
    virtual void* Map();
    /// map for direct memory-access
    virtual void* MemoryMap();
    /// unmap a mapped stream
    virtual void Unmap();
    /// get a direct "raw" pointer to the data
    void* GetRawPointer() const;

protected:
    friend class IO::StreamCache;

    virtual Core::Rtti const& GetParentRtti() = 0;

    // underlying object
    Ptr<IO::Stream> parentStream;

    Size size;
    Position position;
    unsigned char* buffer;
};
} // namespace IO
//------------------------------------------------------------------------------
