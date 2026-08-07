//------------------------------------------------------------------------------
//  posixthread.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "threading/posix/posixthread.h"
#include <limits.h>

namespace Posix
{
__ImplementClass(Posix::PosixThread, 'THRD', Core::RefCounted);

#ifdef HAVE_THREAD_LOCAL_STORAGE
__thread const char* PosixThread::ThreadName = 0; 
#endif

#if NEBULA_DEBUG
Threading::CriticalSection PosixThread::criticalSection;
Util::List<PosixThread*> PosixThread::ThreadList;
#endif

//------------------------------------------------------------------------------
/**
*/
PosixThread::PosixThread() :
    threadHandle(0),
    running(false),
    priority(Normal),
    stackSize(4096)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
PosixThread::~PosixThread()
{
    if (this->IsRunning())
    {
        this->Stop();
    }
}

//------------------------------------------------------------------------------
/**
    Start the thread, this creates a Posix thread and calls the static
    ThreadProc, which in turn calls the virtual DoWork() class of this object.
    The method returns immediately without waiting for the thread to start.
*/
void
PosixThread::Start()
{
    n_assert(!this->IsRunning());
    n_assert(0 == this->threadHandle);
    pthread_attr_t threadAttributes;
    pthread_attr_init(&threadAttributes);
    pthread_attr_setstacksize(&threadAttributes, PTHREAD_STACK_MIN + this->stackSize);
    pthread_attr_setinheritsched(&threadAttributes, PTHREAD_INHERIT_SCHED);
    int r = pthread_create(&(this->threadHandle), &threadAttributes, ThreadProc, (void *) this);
    n_assert(0 == r);
    sched_param sched = {0};
    //int priomin, priomax;

    // apply thread priority
    switch (this->priority)
    {
        case Low:
            sched.sched_priority = sched_get_priority_min(SCHED_OTHER);
            break;
        case Normal:
            sched.sched_priority = (sched_get_priority_max(SCHED_OTHER) - sched_get_priority_min(SCHED_OTHER)) >> 1;
            break;
        case High:
            sched.sched_priority = sched_get_priority_max(SCHED_OTHER);
            break;
    }
    pthread_setschedparam(this->threadHandle, SCHED_OTHER, &sched);
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
PosixThread::EmitWakeupSignal()
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
PosixThread::Stop()
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
    pthread_join(this->threadHandle, NULL);
    this->threadHandle = 0;
}

//------------------------------------------------------------------------------
/**
    This method should be derived in a Thread subclass and contains the
    actual code which is run in the thread. The method must not call
    C-Lib functions under Posix. To terminate the thread, just return from
    this function. If DoWork() runs in an infinite loop, call ThreadStopRequested()
    to check whether the Thread object wants the thread code to quit.
*/
void
PosixThread::DoWork()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Internal static helper method. This is called by CreateThread() and
    simply calls the virtual DoWork() method on the thread object.
*/
void*
PosixThread::ThreadProc(void* self)
{
    n_assert(0 != self);
    PosixThread* threadObj = (PosixThread*) self;
#ifdef HAVE_THREAD_LOCAL_STORAGE
    ThreadName = threadObj->GetName().AsCharPtr();
#endif
    threadObj->DoWork();
    return 0;
}

//------------------------------------------------------------------------------
/**
    Static method to obtain the current thread name from anywhere
    in the thread's code.
*/
const char*
PosixThread::GetMyThreadName()
{
#ifdef HAVE_THREAD_LOCAL_STORAGE
    return ThreadName;
#else
    return NULL;
#endif
}
//------------------------------------------------------------------------------
/**
    Static method which returns the ThreadId of this thread.
*/
Threading::ThreadId
PosixThread::GetMyThreadId()
{
#if __APPLE__
    return reinterpret_cast<uint64_t>(pthread_self());
#else
    return pthread_self();
#endif
}

//------------------------------------------------------------------------------
/**
    Yields thread
*/ 
void 
Posix::PosixThread::YieldThread()
{
#if (__APPLE__ || __OSX__)
    pthread_yield_np();
#else
    int r = pthread_yield();
    n_assert(r == 0);
#endif
}

//------------------------------------------------------------------------------
/**
    Returns an array with infos about all currently existing thread objects.
*/
#if NEBULA_DEBUG
Util::Array<PosixThread::ThreadDebugInfo>
PosixThread::GetRunningThreadDebugInfos()
{
    // NOTE: Portions of this loop aren't completely thread-safe
    // (getting the thread-name for instance), but since those
    // attributes don't change when the thread has been started
    // this shouldn't be a problem.
    Util::Array<ThreadDebugInfo> infos;
    PosixThread::criticalSection.Enter();
    Util::List<PosixThread*>::Iterator iter;
    for (iter = ThreadList.Begin(); iter != ThreadList.End(); iter++)
    {
        PosixThread* cur = *iter;
        if (cur->IsRunning())
        {
            ThreadDebugInfo info;
            info.threadName = cur->GetName();
            info.threadPriority = cur->GetPriority();
            info.threadCoreId = (System::Cpu::CoreId)cur->GetThreadAffinity();
            info.threadStackSize = cur->GetStackSize();
            infos.Append(info);
        }
    }
    PosixThread::criticalSection.Leave();
    return infos;
}
#endif

//------------------------------------------------------------------------------
/**
 */
void
PosixThread::SetThreadAffinity(uint mask)
{
    if (this->threadHandle != 0)
    {
        
    }
    // TODO: MacOS doesn't seem to support this
}
    
};
