//------------------------------------------------------------------------------
//  iointerface.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/iointerface.h"
#include "io/iointerfacehandler.h"
#include "messaging/blockinghandlerthread.h"

namespace IO
{
__ImplementClass(IO::IoInterface, 'IIOF', Interface::InterfaceBase);
__ImplementInterfaceSingleton(IO::IoInterface);

using namespace Messaging;
using namespace Interface;

//------------------------------------------------------------------------------
/**
*/
IoInterface::IoInterface()
{
    __ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
IoInterface::~IoInterface()
{
    __DestructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
IoInterface::Open()
{
    // setup the message handler thread object
    Ptr<BlockingHandlerThread> handlerThread = BlockingHandlerThread::Create();
    handlerThread->SetName("IoInterface Thread");
	handlerThread->SetThreadAffinity(System::Cpu::Core3);
#if __WII_
    handlerThread->SetPriority(Thread::NormalBoost);
#endif        
    handlerThread->AttachHandler(IoInterfaceHandler::Create());
    this->SetHandlerThread(handlerThread.cast<HandlerThreadBase>());

    InterfaceBase::Open();
}

} // namespace IO
