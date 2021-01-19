#pragma once
//------------------------------------------------------------------------------
/**
    @class Http::HttpStream
  
    Wraps client HTTP requests to a HTTP server into an IO::Stream.
    
    @copyright
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#include "core/config.h"
#include "io/memorystream.h"
#include "http/httpclient.h"

//------------------------------------------------------------------------------
namespace Http
{
class HttpStream : public IO::MemoryStream
{
    __DeclareClass(HttpStream);
public:
    /// constructor
    HttpStream();
    /// destructor
    virtual ~HttpStream();
    
    /// open the stream
    virtual bool Open();

    ///
    void SetRetries(int count);
    ///
    int GetRetries() const;

private:
    // amount of retries for slow servers (503 responses)
    int retries;
};

//------------------------------------------------------------------------------
/**
*/
inline void
HttpStream::SetRetries(int count)
{
    this->retries = count;
}

//------------------------------------------------------------------------------
/**
*/
inline int
HttpStream::GetRetries() const
{
    return this->retries;
}

} // namespace Http
//------------------------------------------------------------------------------
