//------------------------------------------------------------------------------
//  streamwriter.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/streamwriter.h"

namespace IO
{
__ImplementClass(IO::StreamWriter, 'STWR', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
StreamWriter::StreamWriter() :
    isOpen(false),
    streamWasOpen(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
StreamWriter::~StreamWriter()
{
    n_assert(!this->IsOpen());
}

//------------------------------------------------------------------------------
/**
    Attaches the writer to a stream. This will imcrement the refcount
    of the stream.
*/
void
StreamWriter::SetStream(const Ptr<Stream>& s)
{
    this->stream = s;
}

//------------------------------------------------------------------------------
/**
    Get pointer to the attached stream. If there is no stream attached,
    an assertion will be thrown. Use HasStream() to determine if a stream
    is attached.
*/
const Ptr<Stream>&
StreamWriter::GetStream() const
{
    return this->stream;
}

//------------------------------------------------------------------------------
/**
    Returns true if a stream is attached to the writer.
*/
bool
StreamWriter::HasStream() const
{
    return this->stream.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
bool
StreamWriter::Open()
{
    n_assert(!this->isOpen);
    n_assert(this->stream.isvalid());
    n_assert(this->stream->CanWrite());
    if (this->stream->IsOpen())
    {
        n_assert((this->stream->GetAccessMode() == Stream::WriteAccess) || (this->stream->GetAccessMode() == Stream::ReadWriteAccess) || (this->stream->GetAccessMode() == Stream::AppendAccess));
        this->streamWasOpen = true;
        this->isOpen = true;
    }
    else
    {
        this->streamWasOpen = false;
        this->stream->SetAccessMode(Stream::WriteAccess);
        this->isOpen = this->stream->Open();
    }
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
void
StreamWriter::Close()
{
    n_assert(this->isOpen);
    if ((!this->streamWasOpen) && stream->IsOpen())
    {
        stream->Close();
    }
    this->isOpen = false;
}

} // namespace IO
