//------------------------------------------------------------------------------
//  httpstream.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "http/httpstream.h"
#include "http/httpclientregistry.h"

// HttpStream not implemented on Wii
#if __NEBULA3_HTTP_FILESYSTEM__
namespace Http
{
__ImplementClass(Http::HttpStream, 'HTST', IO::MemoryStream);

//------------------------------------------------------------------------------
/**
*/
HttpStream::HttpStream()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
HttpStream::~HttpStream()
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
HttpStream::Open()
{
    n_assert(!this->IsOpen());
    bool retval = false;

    if (MemoryStream::Open())
    {
        n_printf("HttpStream: Opening '%s'...", this->uri.AsString().AsCharPtr());

        // create a HTTP client and open connection
        Ptr<HttpClient> httpClient = HttpClientRegistry::Instance()->ObtainConnection(this->uri);
        if (httpClient->IsConnected())
        {
            AccessMode oldAccessMode = this->accessMode;
            this->accessMode = WriteAccess;
            HttpStatus::Code res = httpClient->SendRequest(HttpMethod::Get, this->uri, this);
            this->accessMode = oldAccessMode;
            this->Seek(0, Stream::Begin);
            if (HttpStatus::OK == res)
            {
                n_printf("ok!\n");
                retval = true;
            }
            else
            {
                n_printf("failed!\n");
            }
            HttpClientRegistry::Instance()->ReleaseConnection(this->uri);
        }
    }
    return retval;
}

} // namespace Http
#endif // __WII__