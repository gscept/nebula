#pragma once
#ifndef HTTP_HTTPRESPONSEWRITER_H
#define HTTP_HTTPRESPONSEWRITER_H
//------------------------------------------------------------------------------
/**
    @class Http::HttpResponseWriter
  
    Stream writer which writes a correct HTTP response to a stream.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "io/streamwriter.h"
#include "http/httpstatus.h"

//------------------------------------------------------------------------------
namespace Http
{
class HttpResponseWriter : public IO::StreamWriter
{
    __DeclareClass(HttpResponseWriter);
public:
    /// set status code
    void SetStatusCode(HttpStatus::Code statusCode);
    /// set optional content stream (needs valid media type!)
    void SetContent(const Ptr<IO::Stream>& contentStream);
    /// write http response to the stream
    void WriteResponse();

private:
    HttpStatus::Code statusCode;
    Ptr<IO::Stream> contentStream;
};

//------------------------------------------------------------------------------
/**
*/
inline void
HttpResponseWriter::SetStatusCode(HttpStatus::Code c)
{
    this->statusCode = c;
}

//------------------------------------------------------------------------------
/**
*/
inline void
HttpResponseWriter::SetContent(const Ptr<IO::Stream>& s)
{
    this->contentStream = s;
}

}
//------------------------------------------------------------------------------
#endif
