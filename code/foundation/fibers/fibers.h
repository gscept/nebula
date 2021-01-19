#pragma once
//------------------------------------------------------------------------------
/**
    Fibers are a more lightweight form of jobs which allows recursive job generation, 
    manual pausing and resuming, and more generic code execution to ordinary jobs.

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "fiber.h"
#include "ids/id.h"
#include "ids/idallocator.h"
#include "threading/thread.h"
#include "threading/safequeue.h"
#include "threading/lockfreequeue.h"

namespace Fibers
{

struct FiberContext
{
    Fibers::Fiber* fiber;
    volatile int* counter;
};

struct FiberWaitContext
{
    Fibers::Fiber* fiber;
    volatile int* counter;
    int value;
};

class FiberThread : public Threading::Thread
{
    __DeclareClass(FiberThread);
public:

    /// constructor
    FiberThread();
    /// destructor
    virtual ~FiberThread();

    /// called if thread needs a wakeup call before stopping
    void EmitWakeupSignal();
    /// this method runs in the thread context
    void DoWork();
    /// returns true if thread has work
    bool HasWork();

    /// sleeps the current fiber and switches it for a new one
    void SleepFiber(volatile int* counter, int value);
    /// switches the current fiber
    void NewFiber();
    /// switch back to original fiber
    void SwitchFiber();

private:
    Fibers::FiberContext currentFiber;
    Fibers::Fiber threadFiber;
};

thread_local extern FiberThread* currentThread;

struct FiberQueueCreateInfo
{
    uint numThreads;
    uint numFibers;
};

class FiberQueue
{
public:

    typedef void (*JobFunction)(void* context);
    typedef void (*FiberFunction)();

    struct Job
    {
        JobFunction function;
        uint id;
        void* context;
        AtomicCounter* counter;
    };

    /// constructor
    FiberQueue();
    /// destructor
    virtual ~FiberQueue();

    /// setup job queue
    static void Setup(const FiberQueueCreateInfo& info);
    /// discard job queue
    static void Discard();
    /// enqueue job 
    template <class T>
    static void Enqueue(JobFunction function, const Util::FixedArray<T*>& contexts, AtomicCounter* counter);
    /// free a fiber
    static void Free(uint id);
    /// sleep fiber
    static void Sleep(FiberWaitContext context);
    /// wake up sleeping fibers
    static bool WakeupFiber(Fibers::FiberContext& fiber);
    /// dequeue job as a fiber
    static bool Dequeue(Fibers::FiberContext& fiber);

private:
    /// async queues
    static Threading::LockFreeQueue<Job> PendingJobsQueue;
    static Threading::LockFreeQueue<uint> FiberIdQueue;

    /// storage
    static Util::FixedArray<Fibers::Fiber> Fibers;
    static Util::FixedArray<FiberQueue::Job> FiberContexts;
    static Util::FixedArray<Ptr<FiberThread>> Threads;

    /// sleeping fibers
    static Threading::CriticalSection SleepLock;
    static Util::Array<FiberWaitContext> SleepingFibers;
};

//------------------------------------------------------------------------------
/**
*/
template <class T>
inline void
FiberQueue::Enqueue(JobFunction function, const Util::FixedArray<T*>& contexts, AtomicCounter* counter)
{
    Threading::Interlocked::Exchange(counter, contexts.Size());

    for (uint i = 0; i < contexts.Size(); i++)
    {
        Job job;
        job.function = function;
        job.context = contexts[i];
        job.counter = counter;
        FiberQueue::PendingJobsQueue.Enqueue(job);
    }
}

//------------------------------------------------------------------------------
/**
*/
template <class T>
inline void
Enqueue(FiberQueue::JobFunction function, const Util::FixedArray<T*>& contexts, AtomicCounter* counter)
{
    FiberQueue::Enqueue(function, contexts, counter);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Wait(AtomicCounter* counter, int value)
{
    if (*counter != value)
    {
        currentThread->SleepFiber(counter, value);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
Lock(AtomicCounter* counter, int value)
{
    while (*counter != value) {};
}

} // namespace Fibers
