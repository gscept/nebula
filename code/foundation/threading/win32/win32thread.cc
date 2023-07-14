//------------------------------------------------------------------------------
//  win360thread.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "threading/win32/win32thread.h"

namespace Win32
{
__ImplementClass(Win32::Win32Thread, 'THRD', Core::RefCounted);

using namespace Util;
using namespace System;

ThreadLocal const char* Win32Thread::ThreadName = 0;

#if NEBULA_DEBUG
Threading::CriticalSection Win32Thread::criticalSection;
List<Win32Thread*> Win32Thread::ThreadList;
#endif

//------------------------------------------------------------------------------
/**
*/
Win32Thread::Win32Thread() :
    threadHandle(0),
    affinityMask(0),
    priority(Normal),
    stackSize(NEBULA_THREAD_DEFAULTSTACKSIZE)
{
    // register with thread list
    #if NEBULA_DEBUG
        Win32Thread::criticalSection.Enter();
        this->threadListIterator = ThreadList.AddBack(this);
        Win32Thread::criticalSection.Leave();
    #endif
}

//------------------------------------------------------------------------------
/**
*/
Win32Thread::~Win32Thread()
{
    if (this->IsRunning())
    {
        this->Stop();
    }

    // unregister from thread list
    #if NEBULA_DEBUG
        n_assert(0 != this->threadListIterator);
        Win32Thread::criticalSection.Enter();
        ThreadList.Remove(this->threadListIterator);
        Win32Thread::criticalSection.Leave();
        this->threadListIterator = 0;
    #endif
}

//------------------------------------------------------------------------------
/**
    Start the thread, this creates a Win32 thread and calls the static
    ThreadProc, which in turn calls the virtual DoWork() class of this object.
    The method waits for the thread to start and then returns.
*/
void
Win32Thread::Start()
{
    n_assert(!this->IsRunning());
    n_assert(0 == this->threadHandle);
    this->threadHandle = CreateThread(NULL,             // lpThreadAttributes
                                      this->stackSize,  // dwStackSize
                                      ThreadProc,       // lpStartAddress
                                      (LPVOID) this,    // lpParameter
                                      CREATE_SUSPENDED, // dwCreationFlags
                                      NULL);            // lpThreadId
    n_assert(NULL != this->threadHandle);

    // apply thread priority
    switch (this->priority)
    {
        case Low:   
            SetThreadPriority(this->threadHandle, THREAD_PRIORITY_BELOW_NORMAL);
            break;

        case Normal:
            SetThreadPriority(this->threadHandle, THREAD_PRIORITY_NORMAL);
            break;

        case High:
            SetThreadPriority(this->threadHandle, THREAD_PRIORITY_ABOVE_NORMAL);
            break;
    }
    
    if (this->affinityMask != 0)
        SetThreadAffinityMask(this->threadHandle, this->affinityMask);

    // select a good processor for the thread
    /*
    SystemInfo systemInfo;
    SizeT numCpuCores = systemInfo.GetNumCpuCores();
    DWORD threadIdealProc = 0;
    if (Cpu::InvalidCoreId != this->coreId)
    {
        threadIdealProc = this->coreId % systemInfo.GetNumCpuCores();
    }
    SetThreadIdealProcessor(this->threadHandle, threadIdealProc);
    */

    // resume thread (since it was actived in suspended state)
    ResumeThread(this->threadHandle);

    // wait for the thread to start
    this->threadStartedEvent.Wait();
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
Win32Thread::EmitWakeupSignal()
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
Win32Thread::Stop()
{
    n_assert(this->IsRunning());
    n_assert(0 != this->threadHandle);

    // signal the thread to stop
    this->stopRequestEvent.Signal();

    // call the wakeup-thread method, may be derived in a subclass
    // if the threads needs to be woken up, it is important that this
    // method is called AFTER the stopRequestEvent is signalled!
    this->EmitWakeupSignal();

    // wait for the thread to terminate
    WaitForSingleObject(this->threadHandle, INFINITE);
    CloseHandle(this->threadHandle);
    this->threadHandle = 0;
}

//------------------------------------------------------------------------------
/**
    Returns true if the thread is currently running.
*/
bool
Win32Thread::IsRunning() const
{
    if (0 != this->threadHandle)
    {
        DWORD exitCode = 0;
        if (GetExitCodeThread(this->threadHandle, &exitCode))
        {
            if (STILL_ACTIVE == exitCode)
            {
                return true;
            }
        }
    }
    // fallthrough: thread not yet, or no longer
    return false;
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
Win32Thread::DoWork()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Internal static helper method. This is called by CreateThread() and
    simply calls the virtual DoWork() method on the thread object.
*/
DWORD WINAPI
Win32Thread::ThreadProc(LPVOID self)
{
    n_assert(0 != self);
    
    #if NEBULA_ENABLE_THREADLOCAL_STRINGATOM_TABLES
    // setup thread-local string atom table (will be discarded when thread terminates)
    LocalStringAtomTable localStringAtomTable;
    #endif

    Win32Thread* threadObj = (Win32Thread*) self;
    Win32Thread::SetMyThreadName(threadObj->GetName().AsCharPtr());
    threadObj->threadStartedEvent.Signal();
    threadObj->DoWork();

    return 0;
}

//------------------------------------------------------------------------------
/**
    Static method which sets the name of this thread. This is called from
    within ThreadProc. The string pointed to must remain valid until
    the thread is terminated!
*/
void
Win32Thread::SetMyThreadName(const char* n)
{
    // first update our own internal thread-name pointer
    ThreadName = n;

    // update the Windows thread name so that it shows up correctly
    // in the Debugger
    struct THREADNAME_INFO
    {
        DWORD dwType;     // must be 0x1000
        LPCSTR szName;    // pointer to name (in user address space)
        DWORD dwThreadID; // thread ID (-1 = caller thread)
        DWORD dwFlags;    // reserved for future use, must be zero
    };

    THREADNAME_INFO info;
    info.dwType     = 0x1000;
    info.szName     = n;
    info.dwThreadID = ::GetCurrentThreadId();
    info.dwFlags    = 0;
    __try
    {
        RaiseException( 0x406D1388, 0, sizeof(info) / sizeof(SIZE_T), (SIZE_T*)&info );
    }
    __except( EXCEPTION_CONTINUE_EXECUTION ) 
    {
    }
}

//------------------------------------------------------------------------------
/**
    Static method to obtain the current thread name from anywhere
    in the thread's code.
*/
const char*
Win32Thread::GetMyThreadName()
{
    return ThreadName;
}

//------------------------------------------------------------------------------
/**
    Static method which returns the ThreadId of this thread.
*/
Threading::ThreadId
Win32Thread::GetMyThreadId()
{
    return ::GetCurrentThreadId();
}

//------------------------------------------------------------------------------
/**
    The yield function is empty on Win32 and Xbox360.
*/
void
Win32Thread::YieldThread()
{
    BOOL r = SwitchToThread();
    //n_assert(r);
}

//------------------------------------------------------------------------------
/**
    Returns an array with infos about all currently existing thread objects.
*/
#if NEBULA_DEBUG
Array<Win32Thread::ThreadDebugInfo>
Win32Thread::GetRunningThreadDebugInfos()
{
    // NOTE: Portions of this loop aren't completely thread-safe
    // (getting the thread-name for instance), but since those
    // attributes don't change when the thread has been started
    // this shouldn't be a problem.
    Array<ThreadDebugInfo> infos;
    Win32Thread::criticalSection.Enter();
    List<Win32Thread*>::Iterator iter;
    for (iter = ThreadList.Begin(); iter != ThreadList.End(); iter++)
    {
        Win32Thread* cur = *iter;
        if (cur->IsRunning())
        {
            ThreadDebugInfo info;
            info.threadName = cur->GetName();
            info.threadPriority = cur->GetPriority();
            info.threadStackSize = cur->GetStackSize();
            info.threadCoreId = (System::Cpu::CoreId)cur->GetThreadAffinity();
            infos.Append(info);
        }
    }
    Win32Thread::criticalSection.Leave();
    return infos;
}

#endif

} // namespace Win32
