//-------------------------------------------------------------------------------
//  linuxthread.cc
//  (C) 2010 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//-------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "linuxthread.h"
#if !(__OSX__ || __NACL__)
#include <sched.h>
#include <sys/prctl.h>
#endif

#if NEBULA_ENABLE_THREADLOCAL_STRINGATOM_TABLES
#include "util/localstringatomtable.h"
#endif

#if __ANDROID__
#include "nvidia/nv_thread/nv_thread.h"
#endif

namespace Linux
{
__ImplementClass(Linux::LinuxThread, 'THRD', Core::RefCounted);

using namespace System;
using namespace Util;

#if NEBULA_DEBUG
Threading::CriticalSection LinuxThread::criticalSection;
List<LinuxThread*> LinuxThread::ThreadList;
#endif

//------------------------------------------------------------------------------
/**
*/
LinuxThread::LinuxThread() :
    priority(Normal),
    stackSize(0),
    coreId(System::Cpu::Core0),
    threadState(Initial),
    thread(0)
{
    CPU_ZERO(&this->affinity);
    // register with thread list
    #if NEBULA_DEBUG
    LinuxThread::criticalSection.Enter();
    this->threadListIterator = ThreadList.AddBack(this);
    LinuxThread::criticalSection.Leave();
    #endif
}

//------------------------------------------------------------------------------
/**
*/
LinuxThread::~LinuxThread()
{
    if (this->IsRunning())
    {
        this->Stop();
    }

    // unregister from thread list
    #if NEBULA_DEBUG
    n_assert(0 != this->threadListIterator);
    LinuxThread::criticalSection.Enter();
    ThreadList.Remove(this->threadListIterator);
    LinuxThread::criticalSection.Leave();
    this->threadListIterator = 0;
    #endif
}

//------------------------------------------------------------------------------
/**
*/
void
LinuxThread::Start()
{
    n_assert(!this->IsRunning());

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    // set priority
	#if !__NACL__
    /*
    NOTE: don't mess around with thread priorities (at least for now)

    struct sched_param schedParam;
    pthread_attr_getschedparam(&attr, &schedParam);
    switch (this->priority)
    {
        case Low: schedParam.sched_priority = 24; break;
        case Normal: break;
        case High: schedParam.sched_priority = 16; break;
    }
    pthread_attr_setschedparam(&attr, &schedParam);
    */
    #endif
    // dont mess with pthreads stack size if you dont know what you are doing.
    if(this->stackSize != 0)
    {
        pthread_attr_setstacksize(&attr, this->stackSize);
    }

    // start thread
	#if __ANDROID__
		// on Android, native threads must be registered with the Java runtime
		NVThreadSpawnJNIThread(&this->thread, &attr, ThreadProc, (void*)this);
	#else
		pthread_create(&this->thread, &attr, ThreadProc, (void*)this);
	#endif
    pthread_attr_destroy(&attr);

    // FIXME: on NACL, use pthread_setschedprio() to set the
    // thread priority (-> what are valid prio values?)

    // FIXME: thread stack size isn't set?

    // if affinity is set apply it
    if(CPU_COUNT(&this->affinity))
    {
        pthread_setaffinity_np(this->thread, sizeof(cpu_set_t), &this->affinity);        
    }
    // wait for thread to run
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
LinuxThread::EmitWakeupSignal()
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
LinuxThread::Stop()
{
    n_assert(this->IsRunning());

    // signal the thread to stop
    this->stopRequestEvent.Signal();

    // call the wakeup-thread method, may be derived in a subclass
    // if the threads needs to be woken up, it is important that this
    // method is called AFTER the stopRequestEvent is signalled!
    this->EmitWakeupSignal();

    // wait for the thread to terminate
    pthread_join(this->thread, 0);
    this->threadState = Stopped;
}

//------------------------------------------------------------------------------
/**
    Internal static helper method. This is called by pthread_create() and
    simply calls the virtual DoWork() method on the thread object.
*/
void *
LinuxThread::ThreadProc(void *self)
{
    n_assert(0 != self);
    n_dbgout("LinuxThread::ThreadProc(): thread started!\n");

    #if NEBULA_ENABLE_THREADLOCAL_STRINGATOM_TABLES
    // setup thread-local string atom table (will be discarded when thread terminates)
    LocalStringAtomTable* localStringAtomTable = n_new(LocalStringAtomTable);
    #endif

    LinuxThread* threadObj = static_cast<LinuxThread*>(self);
    LinuxThread::SetMyThreadName(threadObj->GetName());
    threadObj->threadState = Running;
    threadObj->threadStartedEvent.Signal();
    threadObj->DoWork();
    threadObj->threadState = Stopped;
    // discard local string atom table
    #if NEBULA_ENABLE_THREADLOCAL_STRINGATOM_TABLES
    n_delete(localStringAtomTable);
    #endif
    // tell memory system that a thread is ending
    // FIXME, currently not implemented or used
    // Memory::OnExitThread();

    return 0;
}

//------------------------------------------------------------------------------
/**
    Returns true if the thread is currently running.
*/
bool
LinuxThread::IsRunning() const
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
LinuxThread::DoWork()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Static method which returns the ThreadId of this thread.
*/
Threading::ThreadId
LinuxThread::GetMyThreadId()
{
    return (pthread_t) pthread_self();
}

//------------------------------------------------------------------------------
/**
    Static method which returns the stop-requested state of this
    thread. Not yet implemented in Linux (see improvements to thread-local-
    data system under Linux in the Nebula mobile thread).
*/
bool
LinuxThread::GetMyThreadStopRequested()
{
    // FIXME! see Windows implementation for details
    return false;
}

//------------------------------------------------------------------------------
/**
    Give up time slice.
*/
void
LinuxThread::YieldThread()
{
    sched_yield();
}

//------------------------------------------------------------------------------
/**
    Returns an array with infos about all currently existing thread objects.
*/
#if NEBULA_DEBUG
Array<LinuxThread::ThreadDebugInfo>
LinuxThread::GetRunningThreadDebugInfos()
{
    // NOTE: Portions of this loop aren't completely thread-safe
    // (getting the thread-name for instance), but since those
    // attributes don't change when the thread has been started
    // this shouldn't be a problem.
    Array<ThreadDebugInfo> infos;
    LinuxThread::criticalSection.Enter();
    List<LinuxThread*>::Iterator iter;
    for (iter = ThreadList.Begin(); iter != ThreadList.End(); iter++)
    {
        LinuxThread* cur = *iter;
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
    LinuxThread::criticalSection.Leave();
    return infos;
}
#endif

//------------------------------------------------------------------------------
/**
 */
void
LinuxThread::SetMyThreadName(const String& n)
{
    #if __OSX__
    // OSX is BSD style UNIX and has pthread_np functions
    pthread_setname_np(n.AsCharPtr());
	#elif __NACL__
    // FIXME: can't set thread name in NACL?
    #else
    // Linux is a bit more complicated...
    String cappedName = n;
    if (cappedName.Length() > 15)
    {
        cappedName.TerminateAtIndex(15);
    }
    prctl(PR_SET_NAME, (unsigned long) cappedName.AsCharPtr(), 0, 0, 0);
    #endif
}

//------------------------------------------------------------------------------
/**
 */
const char *
LinuxThread::GetMyThreadName()
{
	#if __OSX__
    // OSX is BSD style UNIX and has pthread_np functions
 	return "<FIXME>";   
    #else
    static char buffer[16];
    prctl(PR_GET_NAME, buffer,0,0,0,0);
    return buffer;
    #endif
}

//------------------------------------------------------------------------------
/**
 */
int
LinuxThread::GetMyThreadPriority()
{
    sched_param param;
    int policy;
    pthread_getschedparam(pthread_self(), &policy, &param);
    return param.sched_priority;
}

//------------------------------------------------------------------------------
/**
 */
void
LinuxThread::SetThreadAffinity(uint mask)
{    
	CPU_SET(mask, &this->affinity);
    if(this->thread != 0)
    {
	    pthread_setaffinity_np(this->thread, sizeof(cpu_set_t), &this->affinity);
    }
}

} // namespace Linux
