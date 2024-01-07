#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::FileStream
  
    A stream to which offers read/write access to filesystem files.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "io/stream.h"
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
    virtual bool CanRead() const override;
    /// supports writing?
    virtual bool CanWrite() const override;
    /// supports seeking?
    virtual bool CanSeek() const override;
    /// supports memory mapping (read-only)?
    virtual bool CanBeMapped() const override;
    /// get the size of the stream in bytes
    virtual Size GetSize() const override;
    /// get the current position of the read/write cursor
    virtual Position GetPosition() const override;
    /// open the stream
    virtual bool Open() override;
    /// close the stream
    virtual void Close() override;
    /// directly write to the stream
    virtual void Write(const void* ptr, Size numBytes) override;
    /// directly read from the stream
    virtual Size Read(void* ptr, Size numBytes) override;
    /// seek in stream
    virtual void Seek(Offset offset, SeekOrigin origin) override;
    /// flush unsaved data
    virtual void Flush() override;
    /// return true if end-of-stream reached
    virtual bool Eof() const override;
    /// map stream to memory
    virtual void* Map() override;
    /// unmap stream
    virtual void Unmap() override;
    /// memory map stream to memory
    virtual void* MemoryMap() override;
    /// unmap memory stream 
    virtual void MemoryUnmap() override;

protected:
    FSWrapper::Handle handle;
    FSWrapper::Handle mapHandle;
    void* mappedContent;
};

} // namespace IO
//------------------------------------------------------------------------------
