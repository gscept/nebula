#pragma once
//------------------------------------------------------------------------------
/**
    @class Http::HttpRequestWriter
    
    Writes a valid HTTP request header to a stream.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "io/streamwriter.h"
#include "http/httpmethod.h"

//------------------------------------------------------------------------------
namespace Http
{
class HttpRequestWriter : public IO::StreamWriter
{
    __DeclareClass(HttpRequestWriter);
public:
    /// constructor
    HttpRequestWriter();
    
    /// set HTTP Method
    void SetMethod(HttpMethod::Code httpMethod);
    /// set the URI of the request
    void SetURI(const IO::URI& uri);
    /// set optional User-Agent string
    void SetUserAgent(const Util::String& userAgent);
    /// write the request header to the stream
    bool WriteRequestHeader();
    /// write a body
    bool WriteRequestHeaderWithBody(const Util::String & body);

private:
    HttpMethod::Code httpMethod;
    IO::URI uri;
    Util::String userAgent;
};

//------------------------------------------------------------------------------
/**
*/
inline void
HttpRequestWriter::SetMethod(HttpMethod::Code m)
{
    this->httpMethod = m;
}

//------------------------------------------------------------------------------
/**
*/
inline void
HttpRequestWriter::SetURI(const IO::URI& u)
{
    this->uri = u;
}

//------------------------------------------------------------------------------
/**
*/
inline void
HttpRequestWriter::SetUserAgent(const Util::String& agent)
{
    this->userAgent = agent;
}

} // namespace Http
//------------------------------------------------------------------------------
