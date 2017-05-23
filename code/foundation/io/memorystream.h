#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::MemoryStream
    
    Implements a stream class which writes to and reads from system RAM. 
    Memory streams provide memory mapping for fast direct read/write access.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "io/stream.h"

//------------------------------------------------------------------------------
namespace IO
{
class MemoryStream : public Stream
{
    __DeclareClass(MemoryStream);
public:
    /// constructor
    MemoryStream();
    /// destructor
    virtual ~MemoryStream();
    /// memory streams support reading
    virtual bool CanRead() const;
    /// memory streams support writing
    virtual bool CanWrite() const;
    /// memory streams support seeking
    virtual bool CanSeek() const;
    /// memory streams are mappable
    virtual bool CanBeMapped() const;
    /// set new size of the stream in bytes
    virtual void SetSize(Size s);
    /// get the size of the stream in bytes
    virtual Size GetSize() const;
    /// get the current position of the read/write cursor
    virtual Position GetPosition() const;
    /// open the stream
    virtual bool Open();
    /// close the stream
    virtual void Close();
    /// directly write to the stream
    virtual void Write(const void* ptr, Size numBytes);
    /// directly read from the stream
    virtual Size Read(void* ptr, Size numBytes);
    /// seek in stream
    virtual void Seek(Offset offset, SeekOrigin origin);
    /// return true if end-of-stream reached
    virtual bool Eof() const;
    /// map for direct memory-access
    virtual void* Map();
    /// unmap a mapped stream
    virtual void Unmap();
    /// get a direct "raw" pointer to the data
    void* GetRawPointer() const;

private:
    /// re-allocate the memory buffer
    void Realloc(Size s);
    /// return true if there's enough space for n more bytes
    bool HasRoom(Size numBytes) const;
    /// make room for at least n more bytes
    void MakeRoom(Size numBytes);

    static const Size InitialSize = 256;
    Size capacity;
    Size size;
    Position position;
    unsigned char* buffer;
};

} // namespace IO
//------------------------------------------------------------------------------
