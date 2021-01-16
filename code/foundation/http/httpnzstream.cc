//------------------------------------------------------------------------------
//  httpnzstream.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "http/httpnzstream.h"
#include "http/httpclientregistry.h"
#include "zlib/zlib.h"

namespace Http
{
__ImplementClass(Http::HttpNzStream, 'HZST', IO::MemoryStream);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
HttpNzStream::HttpNzStream()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
HttpNzStream::~HttpNzStream()
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
HttpNzStream::Open()
{
    n_assert(!this->IsOpen());
    bool retval = false;

    // build a modified URI
    String nzUriStr = this->uri.AsString();
    nzUriStr.Append(".nz");
    URI nzUri(nzUriStr);

    n_printf("HttpNzStream: Opening '%s'...", nzUri.AsString().AsCharPtr());

    // create a HTTP client and open connection
    Ptr<HttpClient> httpClient = HttpClientRegistry::Instance()->ObtainConnection(nzUri);
    if (httpClient->IsConnected())
    {
        // create a memory stream which contains the compressed data
        Ptr<MemoryStream> srcStream = MemoryStream::Create();
        srcStream->SetAccessMode(Stream::WriteAccess);
        HttpStatus::Code res = httpClient->SendRequest(HttpMethod::Get, nzUri, srcStream.upcast<Stream>());
        if (HttpStatus::OK == res)
        {
            // access to compressed source data
            srcStream->SetAccessMode(Stream::ReadAccess);
            srcStream->Open();
            uint* srcPtr = (uint*) srcStream->Map();
            n_assert(srcStream->GetSize() < INT_MAX);
            uLong srcDataSize = (uLong)srcStream->GetSize() - 8;   // 8 is sizeof header

            // check magic number and get uncompressed size
            if (srcPtr[0] != 'NZ__')
            {
                n_error("HttpNzStream: stream '%s' is not in .nz format!\n", nzUri.AsString().AsCharPtr());
            }
            
            // decompress into self
            SizeT inflatedSize = srcPtr[1];
            this->SetSize(inflatedSize);
            MemoryStream::Open();
            void* dstPtr = this->Map();

            uLongf destLen = inflatedSize;
            int res = uncompress((Bytef*)dstPtr, &destLen, (const Bytef*)&(srcPtr[2]), srcDataSize);
            n_assert(Z_OK == res);

            this->Unmap();
            srcStream->Unmap();
            srcStream->Close();
            srcStream = nullptr;

            n_printf("ok!\n");
            retval = true;
        }
        else
        {
            n_printf("failed!\n");
        }
    }
    HttpClientRegistry::Instance()->ReleaseConnection(this->uri);
    return retval;
}

} // namespace Http
