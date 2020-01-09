//------------------------------------------------------------------------------
//  httpinterface.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "http/httpinterface.h"
#include "http/httpmessagehandler.h"
#include "messaging/runthroughhandlerthread.h"

namespace Http
{
__ImplementClass(Http::HttpInterface, 'HTIF', Interface::InterfaceBase);
__ImplementInterfaceSingleton(Http::HttpInterface);

using namespace Interface;
using namespace Messaging;

//------------------------------------------------------------------------------
/**
*/
HttpInterface::HttpInterface()
	:tcpPort(2100)
{
    __ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
HttpInterface::~HttpInterface()
{
    __DestructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
HttpInterface::Open()
{
    // setup runthrough handler thread
    Ptr<RunThroughHandlerThread> handlerThread = RunThroughHandlerThread::Create();
    handlerThread->SetName("HttpInterface Thread");
	
	// This has been renamed to SetThreadAffinity
    // handlerThread->SetCoreId(System::Cpu::::MiscThreadCore); 
	handlerThread->SetThreadAffinity(System::Cpu::Core4);

	Ptr<HttpMessageHandler> handler = HttpMessageHandler::Create();
	handler->SetTcpPort(this->tcpPort);
	handlerThread->AttachHandler(handler.cast<Messaging::Handler>());

    this->SetHandlerThread(handlerThread.cast<HandlerThreadBase>());

    InterfaceBase::Open();
}

} // namespace Http
