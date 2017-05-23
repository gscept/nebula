//------------------------------------------------------------------------------
//  httpmessagehandler.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "http/httpmessagehandler.h"
#include "threading/thread.h"

namespace Http
{
__ImplementClass(Http::HttpMessageHandler, 'HTMH', Interface::InterfaceHandlerBase);

using namespace Interface;
using namespace Messaging;
using namespace Threading;

//------------------------------------------------------------------------------
/**
*/
HttpMessageHandler::HttpMessageHandler()
:DefaultTcpPort(2100)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
HttpMessageHandler::~HttpMessageHandler()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
HttpMessageHandler::Open()
{   
    n_assert(!this->IsOpen());
    InterfaceHandlerBase::Open();

    // setup core runtime and central http server
    this->httpServer = HttpServer::Create();
    this->httpServer->SetPort(DefaultTcpPort);
    this->httpServer->Open();
}

//------------------------------------------------------------------------------
/**
*/
void
HttpMessageHandler::Close()
{
    n_assert(this->IsOpen());

    this->httpServer->Close();
    this->httpServer = 0;

    InterfaceHandlerBase::Close();
}

//------------------------------------------------------------------------------
/**
    Triggers the http server from time to time.
*/
void
HttpMessageHandler::DoWork()
{
    n_assert(this->IsOpen());

    // only process http requests once in a while
    this->httpServer->OnFrame();
    Core::SysFunc::Sleep(0.1f);   
}

//------------------------------------------------------------------------------
/**
*/
bool
HttpMessageHandler::HandleMessage(const Ptr<Message>& msg)
{
    n_assert(msg.isvalid());

    if (msg->CheckId(AttachRequestHandler::Id))
    {
        this->OnAttachRequestHandler(msg.cast<AttachRequestHandler>());
    }
    else if (msg->CheckId(RemoveRequestHandler::Id))
    {
        this->OnRemoveRequestHandler(msg.cast<RemoveRequestHandler>());
    }
    else
    {
        // unknown message
        return false;
    }
    // fallthrough: message was handled
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
HttpMessageHandler::OnAttachRequestHandler(const Ptr<AttachRequestHandler>& msg)
{
    this->httpServer->AttachRequestHandler(msg->GetRequestHandler());
}

//------------------------------------------------------------------------------
/**
*/
void
HttpMessageHandler::OnRemoveRequestHandler(const Ptr<RemoveRequestHandler>& msg)
{
    this->httpServer->RemoveRequestHandler(msg->GetRequestHandler());
}

} // namespace Http
