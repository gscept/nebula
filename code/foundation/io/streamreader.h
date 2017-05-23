#pragma once
#ifndef IO_STREAMREADER_H
#define IO_STREAMREADER_H
//------------------------------------------------------------------------------
/**
    @class IO::StreamReader
    
    Stream reader classes provide a specialized read-interface for a stream. 
    This is the abstract base class for all stream readers. It is possible
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
class StreamReader : public Core::RefCounted
{
    __DeclareClass(StreamReader);
public:
    /// constructor
    StreamReader();
    /// destructor
    virtual ~StreamReader();
    /// set stream to read from
    void SetStream(const Ptr<Stream>& s);
    /// get currently set stream 
    const Ptr<Stream>& GetStream() const;
    /// return true if a stream is set
    bool HasStream() const;
    /// return true if the stream has reached EOF
    bool Eof() const;
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
StreamReader::IsOpen() const
{
    return this->isOpen;
}

} // namespace IO
//------------------------------------------------------------------------------
#endif    