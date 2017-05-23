#pragma once
#ifndef HTTP_HTTPREQUESTHANDLER_H
#define HTTP_HTTPREQUESTHANDLER_H
//------------------------------------------------------------------------------
/**
    @class Http::HttpRequestHandler
    
    HttpRequestHandlers are attached to the HttpServer and process
    incoming HTTP requests. When an Http request comes in, the
    HttpServer asks every attached HttpRequestHandler until the first one
    accepts the request. If the HttpRequestHandler accepts the request
    its HandleRequest() method will be called with a pointer to a content
    stream. The request handler is expected to write the response to the
    content stream (IMPORTANT: don't forget to set the MediaType on the stream!)
    and return with a HttpStatus code.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "http/httprequest.h"
#include "io/uri.h"
#include "io/stream.h"
#include "util/string.h"
#include "threading/safequeue.h"

//------------------------------------------------------------------------------
namespace Http
{
class HttpRequestHandler : public Core::RefCounted
{
    __DeclareClass(HttpRequestHandler);
public:
    /// constructor
    HttpRequestHandler();
    /// destructor
    virtual ~HttpRequestHandler();

    /// get a human readable name of the request handler
    const Util::String& GetName() const;
    /// get a human readable description of the request handler
    const Util::String& GetDesc() const;
    /// get a resource location path which is accepted by the handler (e.g. "/display")
    const Util::String& GetRootLocation() const;

protected:
    friend class HttpServer;
    friend class HttpServerProxy;

    /// handle a http request, overwrite this method in you subclass
    virtual void HandleRequest(const Ptr<HttpRequest>& request);
    /// handle all pending requests, called by local-thread's HttpServerProxy
    void HandlePendingRequests();
    /// put a request to the pending queue, called by HttpServer thread
    void PutRequest(const Ptr<HttpRequest>& httpRequest);
    /// set human readable name of the request handler
    void SetName(const Util::String& n);
    /// set human readable description
    void SetDesc(const Util::String& d);
    /// set the root location of the request handler
    void SetRootLocation(const Util::String& l);

    Util::String name;
    Util::String desc;
    Util::String rootLocation;
    Threading::SafeQueue<Ptr<HttpRequest> > pendingRequests;
    Util::Array<Ptr<HttpRequest> > curWorkRequests;
};

//------------------------------------------------------------------------------
/**
*/
inline void
HttpRequestHandler::SetName(const Util::String& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
HttpRequestHandler::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline void
HttpRequestHandler::SetDesc(const Util::String& d)
{
    this->desc = d;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
HttpRequestHandler::GetDesc() const
{
    return this->desc;
}

//------------------------------------------------------------------------------
/**
*/
inline void
HttpRequestHandler::SetRootLocation(const Util::String& l)
{
    this->rootLocation = l;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
HttpRequestHandler::GetRootLocation() const
{
    return this->rootLocation;
}

} // namespace Http
//------------------------------------------------------------------------------
#endif

