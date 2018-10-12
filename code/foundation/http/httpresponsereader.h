#pragma once
//------------------------------------------------------------------------------
/**
    @class Http::HttpResponseReader
    
    Decodes a response header from a HTTP server and optionally writes
    received content data to a provided stream.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "io/streamreader.h"
#include "http/httpstatus.h"
#include "io/mediatype.h"

//------------------------------------------------------------------------------
namespace Http
{
class HttpResponseReader : public IO::StreamReader
{
    __DeclareClass(HttpResponseReader);
public:
    /// constructor
    HttpResponseReader();

    /// read the response
    bool ReadResponse();
    /// return true if this was a valid response
    bool IsValidHttpResponse() const;
    /// get the HTTP status code which was sent by the server
    HttpStatus::Code GetStatusCode() const;
    /// get content type
    const IO::MediaType& GetContentType() const;
    /// get content length
    SizeT GetContentLength() const;

private:
    bool isValidHttpResponse;
    HttpStatus::Code httpStatus;
    IO::MediaType contentType;
    SizeT contentLength;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
HttpResponseReader::IsValidHttpResponse() const
{
    return this->isValidHttpResponse;
}

//------------------------------------------------------------------------------
/**
*/
inline HttpStatus::Code
HttpResponseReader::GetStatusCode() const
{
    return this->httpStatus;
}

//------------------------------------------------------------------------------
/**
*/
inline const IO::MediaType&
HttpResponseReader::GetContentType() const
{
    return this->contentType;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
HttpResponseReader::GetContentLength() const
{
    return this->contentLength;
}

} // namespace Http
//------------------------------------------------------------------------------

    