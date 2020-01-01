#pragma once
#ifndef HTTP_HTTPMESSAGEHANDLER_H
#define HTTP_HTTPMESSAGEHANDLER_H
//------------------------------------------------------------------------------
/**
    @class Http::HttpMessageHandler
    
    Runs the HttpServer thread, and owns the central http server. Processes
    messages sent to the HttpInterface from other threads.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "interface/interfacehandlerbase.h"
#include "messaging/message.h"
#include "io/console.h"
#include "http/httpserver.h"
#include "http/httpprotocol.h"

//------------------------------------------------------------------------------
namespace Http
{
class HttpMessageHandler : public Interface::InterfaceHandlerBase
{
    __DeclareClass(HttpMessageHandler);
public:
    /// constructor
    HttpMessageHandler();
    /// destructor
    virtual ~HttpMessageHandler();

    /// open the handler
    virtual void Open();
    /// close the handler
    virtual void Close();
    /// handle a message, return true if handled
    virtual bool HandleMessage(const Ptr<Messaging::Message>& msg);
    /// do per-frame work
    virtual void DoWork();
	/// set tcpPort
	void SetTcpPort(ushort port);

private:
    ushort DefaultTcpPort;

    /// handle AttachRequestHandler message
    void OnAttachRequestHandler(const Ptr<AttachRequestHandler>& msg);
    /// handle RemoveRequestHandler message
    void OnRemoveRequestHandler(const Ptr<RemoveRequestHandler>& msg);

    Ptr<HttpServer> httpServer;
};

//------------------------------------------------------------------------------
/**
*/
inline void
HttpMessageHandler::SetTcpPort(ushort port)
{
	this->DefaultTcpPort = port;
}

} // namespace HttpMessageHandler
//------------------------------------------------------------------------------
#endif
