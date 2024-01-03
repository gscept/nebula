//------------------------------------------------------------------------------
//  embeddedmemorystream.cc
//  (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/embeddedmemorystream.h"

#ifdef HAS_EMBEDDED_EXPORT
#include "system_resources.h"
#endif

namespace IO
{
__ImplementClass(IO::EmbeddedMemoryStream, 'EMSR', IO::Stream);

//------------------------------------------------------------------------------
/**
*/
EmbeddedMemoryStream::EmbeddedMemoryStream() 
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
EmbeddedMemoryStream::~EmbeddedMemoryStream()
{
    // close the stream if still open
    if (this->IsOpen())
    {
        this->Close();
    }

    this->buffer = 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
EmbeddedMemoryStream::CanWrite() const
{
    return false;
}



//------------------------------------------------------------------------------
/**
    Open the stream for reading 
*/
bool
EmbeddedMemoryStream::Open()
{
    n_assert(!this->IsOpen());
#ifdef HAS_EMBEDDED_EXPORT
    if (Stream::Open())
    {
        Util::String localPath = this->GetURI().LocalPath();
        auto const& resource = system_resources::get_resource(localPath.AsCharPtr());
        n_assert(resource.size_ > 0);
        this->size = resource.size_;
        this->capacity = resource.size_;
        this->buffer = (unsigned char*)resource.ptr_;
        this->position = 0;
        return true;
    }
#else
    n_error("trying to use missing embedded export");
#endif
    return false;
}

} // namespace IO
