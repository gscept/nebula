#pragma once
#include "ids/id.h"
#include "ids/idallocator.h"
#include "threading/thread.h"
#include "threading/event.h"
#include "util/stringatom.h"
//------------------------------------------------------------------------------
/**
    The Jobs2 system provides a set of threads and a pool of jobs from which 
    threads can pickup work.

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
namespace Threading
{
class Event;

}

namespace Jobs2 
{

class JobThread;

typedef volatile long CompletionCounter;
using JobFunc = void(*)(SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx);

struct JobContext
{
    JobFunc func;
    int remainingGroups;
    SizeT numInvocations;
    SizeT groupSize;
    volatile long groupCompletionCounter;
    void* data;
    const volatile long** waitCounters;
    SizeT numWaitCounters;
    volatile long* doneCounter;
    Threading::Event* signalEvent;
};

struct JobNode
{
    JobNode* next;
    JobContext job;
};

struct Jobs2Context
{
    Threading::CriticalSection jobLock;
    Memory::ArenaAllocator<0x100> nodeAllocator;
    JobNode* head;
    Util::FixedArray<Ptr<JobThread>> threads;

};

extern Jobs2Context ctx;

class JobThread : public Threading::Thread
{
    __DeclareClass(JobThread);
public:

    /// constructor
    JobThread();
    /// destructor
    virtual ~JobThread();

protected:
    template <typename CTX> friend void JobDispatch(const JobFunc& func, const SizeT numInvocations, const SizeT groupSize, const CTX& context, Util::FixedArray<const Jobs2::CompletionCounter*> waitCounters, Jobs2::CompletionCounter* doneCounter, Threading::Event* signalEvent);
    template <typename CTX> friend void JobDispatch(const JobFunc& func, const SizeT numInvocations, const CTX& context, Util::FixedArray<const Jobs2::CompletionCounter*> waitCounters, Jobs2::CompletionCounter* doneCounter, Threading::Event* signalEvent);

    /// override this method if your thread loop needs a wakeup call before stopping
    virtual void EmitWakeupSignal() override;
    /// this method runs in the thread context
    virtual void DoWork() override;

private:
    Threading::Event wakeupEvent;
};

struct JobSystemInitInfo
{
    Util::StringAtom name;
    SizeT numThreads;
    uint affinity;
    uint priority;

    JobSystemInitInfo()
        : numThreads(1)
        , affinity(0xFFFFFFFF)
        , priority(UINT_MAX)
    {};
};

/// Create a new job port
void JobSystemInit(const JobSystemInitInfo& info);
/// Destroy job port
void JobSystemUninit();

/// Dispatch job, job system takes ownership of the context and deletes it
template <typename CTX> void JobDispatch(
    const JobFunc& func
    , const SizeT numInvocations
    , const SizeT groupSize
    , const CTX& context
    , Util::FixedArray<const volatile long*> waitCounters = nullptr
    , volatile long* doneCounter = nullptr
    , Threading::Event* signalEvent = nullptr
);
/// Dispatch job as a single group (to be run on a single thread)
template <typename CTX> void JobDispatch(
    const JobFunc& func
    , const SizeT numInvocations
    , const CTX& context
    , Util::FixedArray<const volatile long*> waitCounters = nullptr
    , volatile long* doneCounter = nullptr
    , Threading::Event* signalEvent = nullptr
);

//------------------------------------------------------------------------------
/**
*/
template <typename CTX> void
JobDispatch(const JobFunc& func, const SizeT numInvocations, const SizeT groupSize, const CTX& context, Util::FixedArray<const Jobs2::CompletionCounter*> waitCounters, Jobs2::CompletionCounter* doneCounter, Threading::Event* signalEvent)
{
    static_assert(std::is_trivially_destructible<CTX>::value, "Job context has to be trivially destructible");
    n_assert(doneCounter != nullptr ? *doneCounter > 0 : true);

    // Calculate the number of actual jobs based on invocations and group size
    SizeT numJobs = Math::ceil(numInvocations / float(groupSize));

    // Calculate allocation size which is node + counters + data context
    auto dynamicAllocSize = sizeof(JobNode) + waitCounters.Size() * sizeof(const Jobs2::CompletionCounter*) + sizeof(CTX);
    JobNode* node = (JobNode*)Memory::Alloc(Memory::ScratchHeap, dynamicAllocSize);

    // Copy over wait counters
    node->job.waitCounters = nullptr;
    if (waitCounters.Size() > 0)
    {
        node->job.waitCounters = (const Jobs2::CompletionCounter**)(((char*)node) + sizeof(JobNode));
        memcpy(node->job.waitCounters, waitCounters.Begin(), waitCounters.Size() * sizeof(const Jobs2::CompletionCounter*));
    }

    // Move context
    node->job.data = (void*)(((char*)node) + sizeof(JobNode) + waitCounters.Size() * sizeof(const Jobs2::CompletionCounter*));
    CTX* data = reinterpret_cast<CTX*>(node->job.data);
    *data = context;

    node->job.func = func;
    node->job.remainingGroups = numJobs;
    node->job.groupCompletionCounter = numJobs;
    node->job.numInvocations = numInvocations;
    node->job.groupSize = groupSize;
    node->job.numWaitCounters = (SizeT)waitCounters.Size();
    node->job.doneCounter = doneCounter;
    node->job.signalEvent = signalEvent;

    ctx.jobLock.Enter();
    node->next = ctx.head;
    ctx.head = node;
    ctx.jobLock.Leave();

    // Trigger threads to wake up and compete for jobs
    for (Ptr<JobThread>& thread : ctx.threads)
    {
        thread->EmitWakeupSignal();
    }
}

//------------------------------------------------------------------------------
/**
*/
template <typename CTX> void
JobDispatch(const JobFunc& func, const SizeT numInvocations, const CTX& context, Util::FixedArray<const Jobs2::CompletionCounter*> waitCounters, Jobs2::CompletionCounter* doneCounter, Threading::Event* signalEvent)
{
    static_assert(std::is_trivially_destructible<CTX>::value, "Job context has to be trivially destructible");
    n_assert(doneCounter != nullptr ? *doneCounter > 0 : true);

    // Calculate allocation size which is node + counters + data context
    auto dynamicAllocSize = sizeof(JobNode) + waitCounters.Size() * sizeof(const Jobs2::CompletionCounter*) + sizeof(CTX);
    JobNode* node = (JobNode*)Memory::Alloc(Memory::ScratchHeap, dynamicAllocSize);

    // Copy over wait counters
    node->job.waitCounters = nullptr;
    if (waitCounters.Size() > 0)
    {
        node->job.waitCounters = (const Jobs2::CompletionCounter**)(((char*)node) + sizeof(JobNode));
        memcpy(node->job.waitCounters, waitCounters.Begin(), waitCounters.Size() * sizeof(const Jobs2::CompletionCounter*));
    }

    // Move context
    node->job.data = (void*)(((char*)node) + sizeof(JobNode) + waitCounters.Size() * sizeof(const Jobs2::CompletionCounter*));
    CTX* data = reinterpret_cast<CTX*>(node->job.data);
    *data = context;

    node->job.func = func;
    node->job.remainingGroups = 1;
    node->job.groupCompletionCounter = 1;
    node->job.numInvocations = numInvocations;
    node->job.groupSize = numInvocations;
    node->job.numWaitCounters = (SizeT)waitCounters.Size();
    node->job.doneCounter = doneCounter;
    node->job.signalEvent = signalEvent;

    ctx.jobLock.Enter();
    node->next = ctx.head;
    ctx.head = node;
    ctx.jobLock.Leave();

    // Trigger threads to wake up and compete for jobs
    for (Ptr<JobThread>& thread : ctx.threads)
    {
        thread->EmitWakeupSignal();
    }
}

} // namespace Jobs2
