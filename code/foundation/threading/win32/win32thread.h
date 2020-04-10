#pragma once
//------------------------------------------------------------------------------
/**
    @class Win32::Win32Thread
    
    Win32/Xbox360 implementation of thread class.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "threading/win32/win32event.h"
#include "threading/threadid.h"
#include "system/cpu.h"
#include "util/localstringatomtable.h"

//------------------------------------------------------------------------------
namespace Win32
{
class Win32Thread : public Core::RefCounted
{
    __DeclareClass(Win32Thread);
public:
    /// thread priorities
    enum Priority
    {
        Low,
        Normal,
        High,
    };

    /// constructor
    Win32Thread();
    /// destructor
    virtual ~Win32Thread();
    /// set the thread priority
    void SetPriority(Priority p);
    /// get the thread priority
    Priority GetPriority() const;
    /// set stack size in bytes (default is 4 KByte)
    void SetStackSize(SizeT s);
    /// get stack size
    SizeT GetStackSize() const;
    /// set thread name
    void SetName(const Util::String& n);
    /// get thread name
    const Util::String& GetName() const;
	/// set the thread affinity
	void SetThreadAffinity(const uint mask);
	/// get the thread affinity
	uint GetThreadAffinity();

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
    
    #if NEBULA_DEBUG
    struct ThreadDebugInfo
    {
        Util::String threadName;
        Win32Thread::Priority threadPriority;
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
    Win32Event threadStartedEvent;
    Win32Event stopRequestEvent;
    Priority priority;
	uint affinityMask;
    SizeT stackSize;
    Util::String name;
    ThreadLocal static const char* ThreadName;
 
    #if NEBULA_DEBUG
    static Threading::CriticalSection criticalSection;
    static Util::List<Win32Thread*> ThreadList;
    Util::List<Win32Thread*>::Iterator threadListIterator;
    #endif
};

//------------------------------------------------------------------------------
/**
*/
inline void
Win32Thread::SetPriority(Priority p)
{
    this->priority = p;
}

//------------------------------------------------------------------------------
/**
*/
inline Win32Thread::Priority
Win32Thread::GetPriority() const
{
    return this->priority;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win32Thread::SetStackSize(SizeT s)
{
    this->stackSize = s;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
Win32Thread::GetStackSize() const
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
Win32Thread::ThreadStopRequested() const
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
Win32Thread::SetName(const Util::String& n)
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
Win32Thread::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Win32Thread::SetThreadAffinity(const uint mask)
{
	this->affinityMask = mask;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
Win32Thread::GetThreadAffinity()
{
	return this->affinityMask;
}

}; // namespace Win32
//------------------------------------------------------------------------------
