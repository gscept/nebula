#pragma once
#ifndef HTTP_HTTPREQUESTREADER_H
#define HTTP_HTTPREQUESTREADER_H
//------------------------------------------------------------------------------
/**
    @class Http::HttpRequestReader
    
    A stream reader which cracks a HTTP request into its components.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "io/streamreader.h"
#include "http/httpmethod.h"
#include "io/uri.h"

//------------------------------------------------------------------------------
namespace Http
{
class HttpRequestReader : public IO::StreamReader
{
    __DeclareClass(HttpRequestReader);
public:
    /// constructor
    HttpRequestReader();
    /// decode the request from the stream (call first before Get methods!)
    bool ReadRequest();
    /// return true if the stream contains a valid HTTP request 
    bool IsValidHttpRequest() const;
    /// get HTTP request method
    HttpMethod::Code GetHttpMethod() const;
    /// get request URI
    const IO::URI& GetRequestURI() const;

private:
    bool isValidHttpRequest;
    HttpMethod::Code httpMethod;
    IO::URI requestURI;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
HttpRequestReader::IsValidHttpRequest() const
{
    return this->isValidHttpRequest;
}

//------------------------------------------------------------------------------
/**
*/
inline HttpMethod::Code
HttpRequestReader::GetHttpMethod() const
{
    return this->httpMethod;
}

//------------------------------------------------------------------------------
/**
*/
inline const IO::URI&
HttpRequestReader::GetRequestURI() const
{
    return this->requestURI;
}

} // namespace Http
//------------------------------------------------------------------------------
#endif
