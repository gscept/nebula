//------------------------------------------------------------------------------
//  safefilestream.cc
//  (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/safefilestream.h"
#include "ioserver.h"

namespace IO
{
__ImplementClass(IO::SafeFileStream, 'SFSR', IO::FileStream);

using namespace Util;
using namespace Core;

//------------------------------------------------------------------------------
/**
*/
SafeFileStream::SafeFileStream()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
SafeFileStream::~SafeFileStream()
{
    if (this->IsOpen())
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
SafeFileStream::Open()
{
    n_assert(!this->IsOpen());
    n_assert(0 == this->handle);
    n_assert(this->uri.Scheme() == "safefile");
    n_assert(this->uri.LocalPath().IsValid());

    if (Stream::Open())
    {
		if (FSWrapper::FileExists(this->uri.LocalPath()))
		{
			String folder = this->uri.LocalPath().ExtractToLastSlash();
			this->tmpUri = IoServer::Instance()->CreateTemporaryFilename(URI(folder));
			this->handle = FSWrapper::OpenFile(this->tmpUri.GetHostAndLocalPath(), this->accessMode, this->accessPattern);
		}
		else
		{
			this->handle = FSWrapper::OpenFile(this->uri.GetHostAndLocalPath(), this->accessMode, this->accessPattern);
		}
        if (0 != this->handle)
        {
            return true;
        }
    }

    // fallthrough: failure
    Stream::Close();
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
SafeFileStream::Close()
{
    n_assert(this->IsOpen());
    n_assert(0 != this->handle);
    if (this->IsMapped())
    {
        this->Unmap();
    }
    FSWrapper::CloseFile(this->handle);
    this->handle = 0;
	Stream::Close();
	if (this->tmpUri.IsValid())
	{
		FSWrapper::ReplaceFile(this->tmpUri.LocalPath(), this->uri.LocalPath());
	}
}

} // namespace IO
