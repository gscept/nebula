//------------------------------------------------------------------------------
//  debuginterface.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "debug/debuginterface.h"
#include "debug/debughandler.h"
#include "messaging/runthroughhandlerthread.h"

namespace Debug
{
__ImplementClass(Debug::DebugInterface, 'DBIF', Interface::InterfaceBase);
__ImplementInterfaceSingleton(Debug::DebugInterface);

using namespace Interface;
using namespace Messaging;

//------------------------------------------------------------------------------
/**
*/
DebugInterface::DebugInterface()
{
    __ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
DebugInterface::~DebugInterface()
{
    __DestructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
DebugInterface::Open()
{
    // setup the message handler thread object
    Ptr<RunThroughHandlerThread> handlerThread = RunThroughHandlerThread::Create();
    handlerThread->SetName("DebugInterface Thread");
    handlerThread->SetCoreId(System::Cpu::MiscThreadCore);
#if __WII_
    handlerThread->SetPriority(Thread::NormalBoost);
#endif        
    handlerThread->AttachHandler(DebugHandler::Create());
    this->SetHandlerThread(handlerThread.cast<HandlerThreadBase>());

    InterfaceBase::Open();
}

} // namespace Debug
