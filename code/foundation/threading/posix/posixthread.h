#pragma once
#ifndef POSIX_POSIXTHREAD_H
#define POSIX_POSIXTHREAD_H
//------------------------------------------------------------------------------
/**
    @class Posix::PosixThread
    
    Posix implementation of thread class.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "threading/posix/posixevent.h"
#include "threading/threadid.h"
#include "system/cpu.h"
#include <sched.h>

//------------------------------------------------------------------------------
namespace Posix
{
class PosixThread : public Core::RefCounted
{
    __DeclareClass(PosixThread);
public:
    /// thread priorities
    enum Priority
    {
        Low,
        Normal,
        High,
    };
    /// constructor
    PosixThread();
    /// destructor
    virtual ~PosixThread();
    /// set the thread priority
    void SetPriority(Priority p);
    /// get the thread priority
    Priority GetPriority() const;
    /// set thread affinity
    void SetThreadAffinity(uint mask);
	/// get thread affinity
	uint GetThreadAffinity();
    /// set stack size in bytes (default is 4 KByte)
    void SetStackSize(unsigned int s);
    /// get stack size
    unsigned int GetStackSize() const;
    /// set thread name
    void SetName(const Util::String& n);
    /// get thread name
    const Util::String& GetName() const;
    /// start executing the thread code, returns when thread has actually started
    void Start();
    /// request threading code to stop, returns when thread has actually finished
    void Stop();
    /// return true if thread has been started
    bool IsRunning() const;
    
    /// yield the thread (gives up current time slice)
    static void YieldThread();
    /// obtain name of thread from within thread code
    static const char* GetMyThreadName();
     /// get the thread ID of this thread
    static Threading::ThreadId GetMyThreadId();

protected:
    /// override this method if your thread loop needs a wakeup call before stopping
    virtual void EmitWakeupSignal();
    /// this method runs in the thread context
    virtual void DoWork();
    /// check if stop is requested, call from DoWork() to see if the thread proc should quit
    bool ThreadStopRequested();

private:
    /// internal thread proc helper function
    static void* ThreadProc(void* self);

    pthread_t threadHandle;
	cpu_set_t affinity;
    PosixEvent stopRequestEvent;
    bool running;
    Priority priority;
    unsigned int stackSize;
    Util::String name;
    static ThreadLocal const char* ThreadName;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
PosixThread::IsRunning() const
{
    return (0 != this->threadHandle);
}

//------------------------------------------------------------------------------
/**
*/
inline void
PosixThread::SetPriority(Priority p)
{
    this->priority = p;
}

//------------------------------------------------------------------------------
/**
*/
inline PosixThread::Priority
PosixThread::GetPriority() const
{
    return this->priority;
}

//------------------------------------------------------------------------------
/**
    If the derived DoWork() method is running in a loop it must regularly
    check if the process wants the thread to terminate by calling
    ThreadStopRequested() and simply return if the result is true. This
    will cause the thread to shut down.
*/
inline bool
PosixThread::ThreadStopRequested()
{
    return this->stopRequestEvent.Peek();
}

//------------------------------------------------------------------------------
/**
    Set the thread's name. To obtain the current thread's name from anywhere
    in the thread's execution context, call the static method
    Thread::GetMyThreadName().
*/
inline void
PosixThread::SetName(const Util::String& n)
{
    n_assert(n.IsValid());
    this->name = n;
}

//------------------------------------------------------------------------------
/**
    Get the thread's name. This is the vanilla method which
    returns the name member. To obtain the current thread's name from anywhere
    in the thread's execution context, call the static method
    Thread::GetMyThreadName().
*/
inline const Util::String&
PosixThread::GetName() const
{
    return this->name;
}

} // namespace Posix
//------------------------------------------------------------------------------
#endif
