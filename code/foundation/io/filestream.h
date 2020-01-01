#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::FileStream
  
    A stream to which offers read/write access to filesystem files.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "io/stream.h"
#include "util/string.h"
#include "io/filetime.h"
#include "io/fswrapper.h"

//------------------------------------------------------------------------------
namespace IO
{
class FileStream : public Stream
{
    __DeclareClass(FileStream);
public:
    /// constructor
    FileStream();
    /// destructor
    virtual ~FileStream();
    /// supports reading?
    virtual bool CanRead() const;
    /// supports writing?
    virtual bool CanWrite() const;
    /// supports seeking?
    virtual bool CanSeek() const;
    /// supports memory mapping (read-only)?
    virtual bool CanBeMapped() const;
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
    /// flush unsaved data
    virtual void Flush();
    /// return true if end-of-stream reached
    virtual bool Eof() const;
    /// map stream to memory
    virtual void* Map();
    /// unmap stream
    virtual void Unmap();

protected:
    FSWrapper::Handle handle;
    void* mappedContent;
};

} // namespace IO
//------------------------------------------------------------------------------
