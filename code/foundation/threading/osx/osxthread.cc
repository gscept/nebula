//-------------------------------------------------------------------------------
//  osxthread.cc
//  (C) 2010 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//-------------------------------------------------------------------------------
#include "stdneb.h"
#include "osxthread.h"

namespace OSX
{
__ImplementClass(OSX::OSXThread, 'THRD', Core::RefCounted);
    
using namespace System;    
    
//------------------------------------------------------------------------------
/**
*/
OSXThread::OSXThread() :
    priority(Normal),
    stackSize(16384),
    coreId(Cpu::InvalidCoreId),
    threadState(Initial)
{
    // register with thread list
    #if NEBULA3_DEBUG
    OSXThread::criticalSection.Enter();
    this->threadListIterator = ThreadList.AddBack(this);
    OSXThread::criticalSection.Leave();
    #endif
}

//------------------------------------------------------------------------------
/**
*/
OSXThread::~OSXThread()
{
    if (this->IsRunning())
    {
        this->Stop();
    }
    
    // unregister from thread list
    #if NEBULA3_DEBUG
    n_assert(0 != this->threadListIterator);
    PS3Thread::criticalSection.Enter();
    ThreadList.Remove(this->threadListIterator);
    PS3Thread::criticalSection.Leave();
    this->threadListIterator = 0;
    #endif
}
    
//------------------------------------------------------------------------------
/**
*/
void
OSXThread::Start()
{
    n_error("OSXThread::Start(): IMPLEMENT ME!\n");
}
    
//------------------------------------------------------------------------------
/**
    This method is called by Thread::Stop() after setting the 
    stopRequest event and before waiting for the thread to stop. If your
    thread runs a loop and waits for jobs it may need an extra wakeup
    signal to stop waiting and check for the ThreadStopRequested() event. In
    this case, override this method and signal your event object.
*/
void
OSXThread::EmitWakeupSignal()
{
    // empty, override in subclass!
}
   
//------------------------------------------------------------------------------
/**
    This stops the thread by signalling the stopRequestEvent and waits for the
    thread to actually quit. If the thread code runs in a loop it should use the 
    IsStopRequested() method to see if the thread object wants it to shutdown. 
    If so DoWork() should simply return.
*/
void
OSXThread::Stop()
{
    n_error("OSXThread::Stop(): IMPLEMENT ME!");
}

//------------------------------------------------------------------------------
/**
    Returns true if the thread is currently running.
*/
bool
OSXThread::IsRunning() const
{
    return (Running == this->threadState);
}
    
//------------------------------------------------------------------------------
/**
    This method should be derived in a Thread subclass and contains the
    actual code which is run in the thread. The method must not call
    C-Lib functions under Win32. To terminate the thread, just return from
    this function. If DoWork() runs in an infinite loop, call ThreadStopRequested()
    to check whether the Thread object wants the thread code to quit.
*/
void
OSXThread::DoWork()
{
    // empty
}
    
//------------------------------------------------------------------------------
/**
    Static method which returns the ThreadId of this thread.
*/
Threading::ThreadId
OSXThread::GetMyThreadId()
{
    n_error("OSXThread::GetMyThreadId(): IMPLEMENT ME!");
    return 0;
}
    
//------------------------------------------------------------------------------
/**
    Give up time slice.
*/
void
OSXThread::YieldThread()
{
    n_error("OSXThread::YieldThread(): IMPLEMENT ME!");
}
    
//------------------------------------------------------------------------------
/**
    Returns an array with infos about all currently existing thread objects.
*/
#if NEBULA3_DEBUG
Array<OSXThread::ThreadDebugInfo>
OSXThread::GetRunningThreadDebugInfos()
{
    // NOTE: Portions of this loop aren't completely thread-safe
    // (getting the thread-name for instance), but since those
    // attributes don't change when the thread has been started
    // this shouldn't be a problem.
    Array<ThreadDebugInfo> infos;
    OSXThread::criticalSection.Enter();
    List<OSXThread*>::Iterator iter;
    for (iter = ThreadList.Begin(); iter != ThreadList.End(); iter++)
    {
        OSXThread* cur = *iter;
        if (cur->IsRunning())
        {
            ThreadDebugInfo info;
            info.threadName = cur->GetName();
            info.threadPriority = cur->GetPriority();
            info.threadCoreId = cur->GetCoreId();
            info.threadStackSize = cur->GetStackSize();
            infos.Append(info);
        }
    }
    OSXThread::criticalSection.Leave();
    return infos;
}
#endif
    
} // namespace OSX