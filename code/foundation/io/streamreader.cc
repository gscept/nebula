//------------------------------------------------------------------------------
//  streamreader.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/streamreader.h"

namespace IO
{
__ImplementClass(IO::StreamReader, 'STRR', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
StreamReader::StreamReader() :
    isOpen(false),
    streamWasOpen(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
StreamReader::~StreamReader()
{
    n_assert(!this->IsOpen());
}

//------------------------------------------------------------------------------
/**
    Attaches the reader to a stream. This will imcrement the refcount
    of the stream.
*/
void
StreamReader::SetStream(const Ptr<Stream>& s)
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
StreamReader::GetStream() const
{
    return this->stream;
}

//------------------------------------------------------------------------------
/**
    Returns true if a stream is attached to the reader.
*/
bool
StreamReader::HasStream() const
{
    return this->stream.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
bool
StreamReader::Eof() const
{
    n_assert(this->IsOpen());
    return this->stream->Eof();
}

//------------------------------------------------------------------------------
/**
*/
bool
StreamReader::Open()
{
    n_assert(!this->isOpen);
    n_assert(this->stream.isvalid());
    n_assert(this->stream->CanRead());
    if (this->stream->IsOpen())
    {
        n_assert((this->stream->GetAccessMode() == Stream::ReadAccess) || (this->stream->GetAccessMode() == Stream::ReadWriteAccess));
        this->streamWasOpen = true;
        this->isOpen = true;
    }
    else
    {
        this->streamWasOpen = false;
        this->stream->SetAccessMode(Stream::ReadAccess);
        this->isOpen = this->stream->Open();
    }
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
void
StreamReader::Close()
{
    n_assert(this->isOpen);
    if ((!this->streamWasOpen) && stream->IsOpen())
    {
        this->stream->Close();
    }
    this->isOpen = false;
}

} // namespace IO
