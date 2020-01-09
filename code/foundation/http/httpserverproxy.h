#pragma once
#ifndef HTTP_HTTPSERVERPROXY_H
#define HTTP_HTTPSERVERPROXY_H
//------------------------------------------------------------------------------
/**
    @class Http::HttpServerProxy
    
    Client-side proxy of the HttpServer. Client threads create and
    attach HttpRequestHandlers to their HttpServerProxy. The HttpServerProxy 
    receives incoming http requests from the http thread, and lets
    its HttpRequestHandlers process the request in the client thread's
    context, then sends the result back to the http thread.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "http/httprequesthandler.h"

//------------------------------------------------------------------------------
namespace Http
{
class HttpServerProxy : public Core::RefCounted
{
    __DeclareClass(HttpServerProxy);
    __DeclareSingleton(HttpServerProxy);
public:
    /// constructor
    HttpServerProxy();
    /// destructor
    virtual ~HttpServerProxy();

    /// open the server proxy
    void Open();
    /// close the server proxy
    void Close();
    /// return true if open
    bool IsOpen() const;

    /// attach a request handler to the server
    void AttachRequestHandler(const Ptr<Http::HttpRequestHandler>& h);
    /// remove a request handler from the server
    void RemoveRequestHandler(const Ptr<Http::HttpRequestHandler>& h);
    /// handle pending http requests, call this method frequently
    void HandlePendingRequests();

private:
    Util::Array<Ptr<HttpRequestHandler> > requestHandlers;
    bool isOpen;
};        

//------------------------------------------------------------------------------
/**
*/
inline bool
HttpServerProxy::IsOpen() const
{
    return this->isOpen;
}

} // namespace Http
//------------------------------------------------------------------------------
#endif
    