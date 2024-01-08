//------------------------------------------------------------------------------
//  curlhttpclient.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#ifdef USE_CURL

#include "http/curl/curlhttpclient.h"
#include "http/httprequestwriter.h"
#include "http/httpresponsereader.h"
#include "curl/curl.h"
#include "io/memorystream.h"
#include "net/socket/ipaddress.h"
#include <mutex>

namespace Http
{
__ImplementClass(Http::CurlHttpClient, 'HCCL', Core::RefCounted);

using namespace Util;
using namespace IO;
using namespace Net;

static bool curlInitialized = false;
static std::mutex initMutex;

//------------------------------------------------------------------------------
/**
*/
CurlHttpClient::CurlHttpClient() :
    userAgent("Mozilla"),    // NOTE: web browser are picky about user agent strings, so use something common
    curlSession(nullptr)
{
    this->curlErrorBuffer = (char*)Memory::Alloc(Memory::HeapType::NetworkHeap, 4 * CURL_ERROR_SIZE);
    initMutex.lock();

    if (!curlInitialized)
    {
#if __WIN32__
        // default to windows native CA store
        curl_global_sslset(CURLSSLBACKEND_SCHANNEL,0,0);
#endif
        curl_global_init(CURL_GLOBAL_ALL);
        curlInitialized = true;
    }
    initMutex.unlock();
}

//------------------------------------------------------------------------------
/**
*/
size_t
CurlHttpClient::CurlWriteDataCallback(char* ptr, size_t size, size_t nmemb, void* userData) 
{
    int bytesToWrite = (int) (size * nmemb);
    if (bytesToWrite > 0 && userData != nullptr) 
    {
        IO::MemoryStream* stream = (IO::MemoryStream*) userData;
        stream->Write(ptr, bytesToWrite);
        return bytesToWrite;
    }
    else 
    {
        return 0;
    }
}
//------------------------------------------------------------------------------
/**
*/
CurlHttpClient::~CurlHttpClient()
{
    if (this->IsConnected())
    {
        this->Disconnect();
    }
    Memory::Free(Memory::HeapType::NetworkHeap, this->curlErrorBuffer);
    this->curlErrorBuffer = nullptr;
    this->curlSession = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
bool
CurlHttpClient::Connect(const URI& uri)
{
    n_assert(!this->IsConnected());

    this->curlSession = curl_easy_init();
    n_assert(this->curlSession != nullptr);

    curl_easy_setopt(this->curlSession, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(this->curlSession, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(this->curlSession, CURLOPT_ERRORBUFFER, this->curlErrorBuffer);
    curl_easy_setopt(this->curlSession, CURLOPT_WRITEFUNCTION, CurlWriteDataCallback);
    curl_easy_setopt(this->curlSession, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(this->curlSession, CURLOPT_TCP_KEEPIDLE, 10L);
    curl_easy_setopt(this->curlSession, CURLOPT_TCP_KEEPINTVL, 10L);
    curl_easy_setopt(this->curlSession, CURLOPT_TIMEOUT, 30);
    curl_easy_setopt(this->curlSession, CURLOPT_CONNECTTIMEOUT, 30);
    curl_easy_setopt(this->curlSession, CURLOPT_ACCEPT_ENCODING, "");   // all encodings supported by curl
    curl_easy_setopt(this->curlSession, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(this->curlSession, CURLOPT_USERAGENT, this->userAgent.AsCharPtr());
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
CurlHttpClient::Disconnect()
{
    if (this->IsConnected())
    {
        curl_easy_cleanup(this->curlSession);
        this->curlSession = nullptr;
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
CurlHttpClient::IsConnected() const
{
    return this->curlSession != nullptr;
}

static CURLoption MethodToCurl(HttpMethod::Code code)
{
    switch (code)
    {
        case HttpMethod::Code::Post: return CURLOPT_POST;
        case HttpMethod::Code::Put: return CURLOPT_PUT;
        case HttpMethod::Code::Get: return CURLOPT_HTTPGET;
        default: break;
    }
    return CURLOPT_HTTPGET;
}

//------------------------------------------------------------------------------
/**
*/
HttpStatus::Code 
CurlHttpClient::SendRequest(HttpMethod::Code requestMethod, const IO::URI& uri, const Util::String & body, const Ptr<IO::Stream>& responseContentStream)
{
    n_assert(this->IsConnected());

    URI fullUri = uri;
    IpAddress ip(uri);
    uint16_t port = ip.GetPort();
    if (port == 0)
    {
        if (uri.Scheme() == "https")
        {
            fullUri.SetPort("443");
        }
        else
        {
            fullUri.SetPort("80");
        }
    }
    struct curl_slist * requestHeaders = nullptr;
    requestHeaders = curl_slist_append(requestHeaders, "Connection: keep-alive");
    requestHeaders = curl_slist_append(requestHeaders, "Accept-Encoding: gzip, deflate");
    curl_easy_setopt(this->curlSession, CURLOPT_HTTPHEADER, requestHeaders);
    curl_easy_setopt(this->curlSession, MethodToCurl(requestMethod), 1);
    curl_easy_setopt(this->curlSession, CURLOPT_URL, fullUri.AsString().AsCharPtr());
    curl_easy_setopt(this->curlSession, CURLOPT_WRITEDATA, responseContentStream.get_unsafe());

    if (!body.IsEmpty())
    {
        curl_easy_setopt(this->curlSession, CURLOPT_POSTFIELDSIZE, (long) body.Length());
        curl_easy_setopt(this->curlSession, CURLOPT_POSTFIELDS, body.AsCharPtr());
    }

    CURLcode res = curl_easy_perform(this->curlSession);
    if (res == CURLE_OK)
    {
        long curlHttpCode = 0;
        curl_easy_getinfo(this->curlSession, CURLINFO_RESPONSE_CODE, &curlHttpCode);
        return HttpStatus::FromLong(curlHttpCode);
    }
    return HttpStatus::BadRequest;
}

//------------------------------------------------------------------------------
/**
*/
HttpStatus::Code
CurlHttpClient::SendRequest(HttpMethod::Code requestMethod, const URI& uri, const Ptr<Stream>& responseContentStream)
{
    return this->SendRequest(requestMethod, uri, "", responseContentStream);
}
} // namespace Http
#endif