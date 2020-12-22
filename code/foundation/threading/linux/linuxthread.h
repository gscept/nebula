#pragma once
//------------------------------------------------------------------------------
/**
    @class Linux::LinuxThread

    Linux implementation of Threading::Thread. Uses the pthread API.

    (C) 2010 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "threading/threadid.h"
#include "threading/event.h"
#include "system/cpu.h"

//------------------------------------------------------------------------------
namespace Linux
{
class LinuxThread : public Core::RefCounted
{
    __DeclareClass(LinuxThread);
public:
    /// thread priorities
    enum Priority
    {
        Low,
        Normal,
        High,
    };

    /// constructor
    LinuxThread();
    /// destructor
    virtual ~LinuxThread();
    /// set the thread priority
    void SetPriority(Priority p);
    /// get the thread priority
    Priority GetPriority() const;
    /// set thread affinity
    void SetThreadAffinity(uint mask);
    /// set thread affinity
    uint GetThreadAffinity();
    /// set stack size in bytes
    void SetStackSize(SizeT s);
    /// get stack size
    SizeT GetStackSize() const;
    /// set maximum thread size
    void SetMaxStackSize(SizeT s);
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
    /// get name of thread 
    static const char* GetMyThreadName();
    /// get the thread ID of this thread
    static Threading::ThreadId GetMyThreadId();
    /// get the stop-requested state of this thread (not yet implemented in Linux)
    static bool GetMyThreadStopRequested();
    /// get the current actual thread-priority of this thread (platform specific!)
    static int GetMyThreadPriority();

    #if NEBULA_DEBUG
    struct ThreadDebugInfo
    {
        Util::String threadName;
        LinuxThread::Priority threadPriority;
        System::Cpu::CoreId threadCoreId;
        SizeT threadStackSize;
    };
    /// query thread stats (debug mode only)
    static Util::Array<ThreadDebugInfo> GetRunningThreadDebugInfos();
    #endif

protected:
    ///
    static void *ThreadProc(void *);

    /// override this method if your thread loop needs a wakeup call before stopping
    virtual void EmitWakeupSignal();
    /// this method runs in the thread context
    virtual void DoWork();
    /// check if stop is requested, call from DoWork() to see if the thread proc should quit
    bool ThreadStopRequested() const;

private:
    /// set thread name
    static void SetMyThreadName(const Util::String& n);

    /// thread states
    enum ThreadState
    {
        Initial,
        Running,
        Stopped,
    };

    pthread_t thread;
    cpu_set_t affinity;
    Priority priority;
    SizeT stackSize;
    Util::String name;
    System::Cpu::CoreId coreId;
    ThreadState volatile threadState;

    LinuxEvent threadStartedEvent;
    LinuxEvent stopRequestEvent;

#if NEBULA_DEBUG
    static Threading::CriticalSection criticalSection;
    static Util::List<LinuxThread*> ThreadList;
    Util::List<LinuxThread*>::Iterator threadListIterator;
#endif
};

//------------------------------------------------------------------------------
/**
*/
inline void
LinuxThread::SetPriority(Priority p)
{
    this->priority = p;
}

//------------------------------------------------------------------------------
/**
 */
inline LinuxThread::Priority
LinuxThread::GetPriority() const
{
    return this->priority;
}

//------------------------------------------------------------------------------
/**
 */
inline void
LinuxThread::SetStackSize(SizeT s)
{
    // empty
}

//------------------------------------------------------------------------------
/**
 */
inline SizeT
LinuxThread::GetStackSize() const
{
    return this->stackSize;
}

//------------------------------------------------------------------------------
/**
 */
inline void
LinuxThread::SetMaxStackSize(SizeT s)
{
    this->stackSize = s;
}

//------------------------------------------------------------------------------
/**
    If the derived DoWork() method is running in a loop it must regularly
    check if the process wants the thread to terminate by calling
    ThreadStopRequested() and simply return if the result is true. This
    will cause the thread to shut down.
 */
inline bool
LinuxThread::ThreadStopRequested() const
{
    return this->stopRequestEvent.Peek();
}

//------------------------------------------------------------------------------
/**
    Set the thread's name.
*/
inline void
LinuxThread::SetName(const Util::String& n)
{
    n_assert(n.IsValid());
    this->name = n;
}

//------------------------------------------------------------------------------
/**
    Get the thread's name. This is the vanilla method which
    returns the name member.
*/
inline const Util::String&
LinuxThread::GetName() const
{
    return this->name;
}


} // namespace Linux
//------------------------------------------------------------------------------
