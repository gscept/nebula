//------------------------------------------------------------------------------
//  httprequestwriter.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "http/httprequestwriter.h"
#include "io/textwriter.h"

namespace Http
{
__ImplementClass(Http::HttpRequestWriter, 'HTRW', IO::StreamWriter);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
HttpRequestWriter::HttpRequestWriter() :
    httpMethod(HttpMethod::Get),
    userAgent("Mozilla")    // NOTE: web browser are picky about user agent strings, so use something common
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
HttpRequestWriter::WriteRequestHeader()
{
    n_assert(this->IsOpen());
    n_assert(this->uri.IsValid());

    // attach a text writer to our stream
    Ptr<TextWriter> textWriter = TextWriter::Create();
    textWriter->SetStream(this->stream);
    if (textWriter->Open())
    {
        // write header
        textWriter->WriteFormatted("%s /%s HTTP/1.1\r\n",
            HttpMethod::ToString(this->httpMethod).AsCharPtr(),
            this->uri.LocalPath().AsCharPtr());
        if (this->httpMethod == HttpMethod::Delete)
        {
            textWriter->WriteString("Content-Length: 0\r\n");
        }
        textWriter->WriteFormatted("Host: %s\r\n", this->uri.Host().AsCharPtr());
        textWriter->WriteFormatted("User-Agent: %s\r\n", this->userAgent.AsCharPtr());
        if (this->httpMethod == HttpMethod::Delete)
        {           
            textWriter->WriteString("Connection: close\r\n");
        }
        else
        {
            textWriter->WriteString("Keep-Alive: 300\r\n");
            textWriter->WriteString("Connection: keep-alive\r\n");
        }               
        textWriter->WriteString("\r\n");
        textWriter->Close();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
HttpRequestWriter::WriteRequestHeaderWithBody(const Util::String & body)
{
    n_assert(this->IsOpen());
    n_assert(this->uri.IsValid());

    // attach a text writer to our stream
    Ptr<TextWriter> textWriter = TextWriter::Create();
    textWriter->SetStream(this->stream);
    if (textWriter->Open())
    {
        // write header
        textWriter->WriteFormatted("%s /%s HTTP/1.1\r\n",
            HttpMethod::ToString(this->httpMethod).AsCharPtr(),
            this->uri.LocalPath().AsCharPtr());
        textWriter->WriteFormatted("Host: %s\r\n", this->uri.Host().AsCharPtr());
        textWriter->WriteFormatted("User-Agent: %s\r\n", this->userAgent.AsCharPtr());
        textWriter->WriteString("Keep-Alive: 300\r\n");
        textWriter->WriteString("Connection: keep-alive\r\n");
        textWriter->WriteString("Content-Type: text/plain; charset=UTF-8\r\n");
        textWriter->WriteFormatted("Content-Length: %u\r\n", body.Length());
        textWriter->WriteString("\r\n");
        textWriter->WriteString(body);
        textWriter->Close();
        return true;
    }
    return false;
}
} // namespace Http