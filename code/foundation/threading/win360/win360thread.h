#pragma once
//------------------------------------------------------------------------------
/**
    @class Win360::Win360Thread
    
    Win32/Xbox360 implementation of thread class.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "threading/win360/win360event.h"
#include "threading/threadid.h"
#include "system/cpu.h"
#include "util/localstringatomtable.h"

//------------------------------------------------------------------------------
namespace Win360
{
class Win360Thread : public Core::RefCounted
{
    __DeclareClass(Win360Thread);
public:
    /// thread priorities
    enum Priority
    {
        Low,
        Normal,
        High,
    };

    /// constructor
    Win360Thread();
    /// destructor
    virtual ~Win360Thread();
    /// set the thread priority
    void SetPriority(Priority p);
    /// get the thread priority
    Priority GetPriority() const;
    /// set cpu core on which the thread should be running
    void SetCoreId(System::Cpu::CoreId coreId);
    /// get the cpu core on which the thread should be running
    System::Cpu::CoreId GetCoreId() const;
    /// set stack size in bytes (default is 4 KByte)
    void SetStackSize(SizeT s);
    /// get stack size
    SizeT GetStackSize() const;
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
    /// set thread name from within thread context
    static void SetMyThreadName(const char* n);
    /// obtain name of thread from within thread context
    static const char* GetMyThreadName();
    /// get the thread ID of this thread
    static Threading::ThreadId GetMyThreadId();
    
    #if NEBULA3_DEBUG
    struct ThreadDebugInfo
    {
        Util::String threadName;
        Win360Thread::Priority threadPriority;
        System::Cpu::CoreId threadCoreId;
        SizeT threadStackSize;
    };
    /// query thread stats (debug mode only)
    static Util::Array<ThreadDebugInfo> GetRunningThreadDebugInfos();        
    #endif

protected:
    /// override this method if your thread loop needs a wakeup call before stopping
    virtual void EmitWakeupSignal();
    /// this method runs in the thread context
    virtual void DoWork();
    /// check if stop is requested, call from DoWork() to see if the thread proc should quit
    bool ThreadStopRequested() const;

private:
    /// internal thread proc helper function
    static DWORD WINAPI ThreadProc(LPVOID self);

    HANDLE threadHandle;
    Win360Event threadStartedEvent;
    Win360Event stopRequestEvent;
    Priority priority;
    SizeT stackSize;
    Util::String name;
    System::Cpu::CoreId coreId;
    ThreadLocal static const char* ThreadName;
 
    #if NEBULA3_DEBUG
    static Threading::CriticalSection criticalSection;
    static Util::List<Win360Thread*> ThreadList;
    Util::List<Win360Thread*>::Iterator threadListIterator;
    #endif
};

//------------------------------------------------------------------------------
/**
*/
inline void
Win360Thread::SetPriority(Priority p)
{
    this->priority = p;
}

//------------------------------------------------------------------------------
/**
*/
inline Win360Thread::Priority
Win360Thread::GetPriority() const
{
    return this->priority;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win360Thread::SetStackSize(SizeT s)
{
    this->stackSize = s;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
Win360Thread::GetStackSize() const
{
    return this->stackSize;
}

//------------------------------------------------------------------------------
/**
    If the derived DoWork() method is running in a loop it must regularly
    check if the process wants the thread to terminate by calling
    ThreadStopRequested() and simply return if the result is true. This
    will cause the thread to shut down.
*/
inline bool
Win360Thread::ThreadStopRequested() const
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
Win360Thread::SetName(const Util::String& n)
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
Win360Thread::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win360Thread::SetCoreId(System::Cpu::CoreId id)
{
    this->coreId = id;
}

//------------------------------------------------------------------------------
/**
*/
inline System::Cpu::CoreId
Win360Thread::GetCoreId() const
{
    return this->coreId;
}

}; // namespace Win360
//------------------------------------------------------------------------------
