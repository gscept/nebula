#pragma once
//------------------------------------------------------------------------------
/**
    @class Http::HttpClient
    
    Use a HTTP client to send HTTP requests to a HTTP server, and
    receive and decode HTTP responses. The HttpClient class
    is generally blocking. For non-blocking behaviour it's best
    to wrap the HttpClient into a thread.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#include "core/config.h"
#include "core/refcounted.h"
#include "http/httpstatus.h"
#include "http/httpmethod.h"
#include "io/uri.h"
#include "io/stream.h"
#include "curl/curl.h"

#ifdef USE_CURL
//------------------------------------------------------------------------------
namespace Http
{
class CurlHttpClient : public Core::RefCounted
{
    __DeclareClass(CurlHttpClient);
public:
    /// constructor
    CurlHttpClient();
    /// destructor
    virtual ~CurlHttpClient();
    
    /// set the user-agent to use for HTTP requests
    void SetUserAgent(const Util::String& userAgent);
    /// get the user-agent
    const Util::String& GetUserAgent() const;

    /// establish a connection to a HTTP server
    bool Connect(const IO::URI& uri);
    /// disconnect from the server
    void Disconnect();
    /// return true if currently connected
    bool IsConnected() const;

    /// send request and write result to provided response content stream
    HttpStatus::Code SendRequest(HttpMethod::Code requestMethod, const IO::URI& uri, const Ptr<IO::Stream>& responseContentStream);
    /// send request with body and write result to provided response content stream
    HttpStatus::Code SendRequest(HttpMethod::Code requestMethod, const IO::URI& uri, const Util::String & body, const Ptr<IO::Stream>& responseContentStream);
private:
    Util::String userAgent;
    void* curlSession;
    char* curlErrorBuffer;

    /// curl write-data callback
    static size_t CurlWriteDataCallback(char* ptr, size_t size, size_t nmemb, void* userData);
    /// curl header-data callback
    static size_t CurlHeaderCallback(char* ptr, size_t size, size_t nmenb, void* userData);
};

//------------------------------------------------------------------------------
/**
*/
inline void
CurlHttpClient::SetUserAgent(const Util::String& agent)
{
    this->userAgent = agent;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
CurlHttpClient::GetUserAgent() const
{
    return this->userAgent;
}

} // namespace Http
//------------------------------------------------------------------------------
#endif