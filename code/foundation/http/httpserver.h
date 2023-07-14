#pragma once
#ifndef HTTP_HTTPSERVER_H
#define HTTP_HTTPSERVER_H
//------------------------------------------------------------------------------
/**
    @class Http::HttpServer
    
    Implements an extremly simple standalone HTTP server with attached
    HttpRequestHandlers. Can be used to serve debug information about the 
    Nebula application to web browsers.

    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "net/tcpserver.h"
#include "net/socket/ipaddress.h"
#include "io/textwriter.h"
#include "http/httpresponsewriter.h"
#include "http/httprequesthandler.h"
#include "http/defaulthttprequesthandler.h"

//------------------------------------------------------------------------------
namespace Http
{
class HttpServer : public Core::RefCounted
{
    __DeclareClass(HttpServer);
    __DeclareSingleton(HttpServer);
public:
    /// constructor
    HttpServer();
    /// destructor
    virtual ~HttpServer();
    
    /// set port number for http service
    void SetPort(ushort p);
    /// get port number of http service
    ushort GetPort() const;
    /// turn single-thread mode on/off (useful for debugging), default is off
    void SetSingleThreadMode(bool b);
    /// get single-thread mode
    bool IsSingleThreadMode() const;
    /// open the http server
    bool Open();
    /// close the http server
    void Close();
    /// return true if server is open
    bool IsOpen() const;
    /// attach a request handler to the server
    void AttachRequestHandler(const Ptr<HttpRequestHandler>& h);
    /// remove a request handler from the server
    void RemoveRequestHandler(const Ptr<HttpRequestHandler>& h);
    /// get registered request handlers
    Util::Array<Ptr<HttpRequestHandler> > GetRequestHandlers() const;
    /// call this method frequently to serve http connections
    void OnFrame();

private:
    /// handle an HttpRequest
    bool HandleHttpRequest(const Ptr<Net::TcpClientConnection>& clientConnection);
    /// build an HttpResponse for a handled http request
    bool BuildHttpResponse(const Ptr<Net::TcpClientConnection>& clientConnection, const Ptr<HttpRequest>& httpRequest);

    struct PendingRequest
    {
        Ptr<Net::TcpClientConnection> clientConnection;
        Ptr<HttpRequest> httpRequest;
    };

    Util::Dictionary<Util::String, Ptr<HttpRequestHandler> > requestHandlers;
    Ptr<DefaultHttpRequestHandler> defaultRequestHandler;    
    Net::IpAddress ipAddress;
    Ptr<Net::TcpServer> tcpServer;
    Util::Array<PendingRequest> pendingRequests;
    bool isOpen;
    bool isSingleThreadMode;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
HttpServer::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
    Switch on single thread mode, default is off. In single thread mode,
    http requests will be processed immediately in the OnFrame() method,
    not added to the request handler for asynchronous processing. This
    may be useful for debugging, but is dangerous/impossible if HTTP
    request handlers live in different threads!!!
*/
inline void
HttpServer::SetSingleThreadMode(bool b)
{
    n_assert(!this->isOpen);
    this->isSingleThreadMode = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
HttpServer::IsSingleThreadMode() const
{
    return this->isSingleThreadMode;
}

//------------------------------------------------------------------------------
/**
*/
inline void
HttpServer::SetPort(ushort p)
{
    this->ipAddress.SetPort(p);
}

//------------------------------------------------------------------------------
/**
*/
inline ushort
HttpServer::GetPort() const
{
    return this->ipAddress.GetPort();
}

//------------------------------------------------------------------------------
/**
*/
inline Util::Array<Ptr<HttpRequestHandler> >
HttpServer::GetRequestHandlers() const
{
    return this->requestHandlers.ValuesAsArray();
}

} // namespace Http
//------------------------------------------------------------------------------
#endif
