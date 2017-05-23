#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::StreamWriter
  
    Stream writer classes provide a specialized write-interface for a stream.
    This is the abstract base class for all stream writers. It is possible
    to attach any number of readers and writers to the same stream.

    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "io/stream.h"

//------------------------------------------------------------------------------
namespace IO
{
class StreamWriter : public Core::RefCounted
{
    __DeclareClass(StreamWriter);
public:
    /// constructor
    StreamWriter();
    /// destructor
    virtual ~StreamWriter();
    /// set stream to write to
    void SetStream(const Ptr<Stream>& s);
    /// get currently set stream 
    const Ptr<Stream>& GetStream() const;
    /// return true if a stream is set
    bool HasStream() const;
    /// begin reading from the stream
    virtual bool Open();
    /// end reading from the stream
    virtual void Close();
    /// return true if currently open
    bool IsOpen() const;

protected:
    Ptr<Stream> stream;
    bool isOpen;
    bool streamWasOpen;
};

//------------------------------------------------------------------------------
/**
*/
inline
bool
StreamWriter::IsOpen() const
{
    return this->isOpen;
}

} // namespace IO
//------------------------------------------------------------------------------