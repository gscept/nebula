#pragma once
#ifndef HTTP_HTTPREQUEST_H
#define HTTP_HTTPREQUEST_H
//------------------------------------------------------------------------------
/**
    @class Http::HttpRequest
    
    Encapsulates a complete Http request into a message.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "messaging/message.h"
#include "http/httpmethod.h"
#include "http/httpstatus.h"
#include "io/uri.h"

//------------------------------------------------------------------------------
namespace Http
{
class HttpRequest : public Messaging::Message
{
    __DeclareClass(HttpRequest);
    __DeclareMsgId;
public:
    /// constructor
    HttpRequest();
    /// destructor
    virtual ~HttpRequest();
    
    /// set the Http method (GET, PUT, etc...)
    void SetMethod(HttpMethod::Code requestMethod);
    /// get the Http method
    HttpMethod::Code GetMethod() const;
    /// set the request URI
    void SetURI(const IO::URI& requestUri);
    /// get the request URI
    const IO::URI& GetURI() const;
    /// set the response content stream
    void SetResponseContentStream(const Ptr<IO::Stream>& responseContentStream);
    /// get the response content stream
    const Ptr<IO::Stream>& GetResponseContentStream() const;
    /// set the http status (set by HttpRequestHandler)
    void SetStatus(HttpStatus::Code status);
    /// get the http status 
    HttpStatus::Code GetStatus() const;

private:
    HttpMethod::Code method;
    IO::URI uri;
    Ptr<IO::Stream> responseContentStream;
    HttpStatus::Code status;
};

//------------------------------------------------------------------------------
/**
*/
inline void
HttpRequest::SetMethod(HttpMethod::Code m)
{
    this->method = m;
}

//------------------------------------------------------------------------------
/**
*/
inline HttpMethod::Code
HttpRequest::GetMethod() const
{
    return this->method;
}

//------------------------------------------------------------------------------
/**
*/
inline void
HttpRequest::SetURI(const IO::URI& u)
{
    this->uri = u;
}

//------------------------------------------------------------------------------
/**
*/
inline const IO::URI&
HttpRequest::GetURI() const
{
    return this->uri;
}

//------------------------------------------------------------------------------
/**
*/
inline void
HttpRequest::SetResponseContentStream(const Ptr<IO::Stream>& s)
{
    this->responseContentStream = s;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<IO::Stream>&
HttpRequest::GetResponseContentStream() const
{
    return this->responseContentStream;
}

//------------------------------------------------------------------------------
/**
*/
inline void
HttpRequest::SetStatus(HttpStatus::Code s)
{
    this->status = s;
}

//------------------------------------------------------------------------------
/**
*/
inline HttpStatus::Code
HttpRequest::GetStatus() const
{
    return this->status;
}

} // namespace Http
//------------------------------------------------------------------------------
#endif
    