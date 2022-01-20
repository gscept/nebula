#pragma once
#include "threading/thread.h"
#include "threading/event.h"
#include "util/stringatom.h"
#include "threading/interlocked.h"

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
    const Threading::AtomicCounter** waitCounters;
    SizeT numWaitCounters;
    Threading::AtomicCounter* doneCounter;
    Threading::Event* signalEvent;
};

struct JobNode
{
    JobNode* next;
    JobContext job;
    long sequenceWaitValue; // set to 0 for ordinary nodes
};

struct Jobs2Context
{
    Threading::CriticalSection jobLock;
    JobNode* head;
    JobNode* tail;
    Util::FixedArray<Ptr<JobThread>> threads;
    Util::Array<JobNode*> queuedJobs;

    SizeT numBuffers;
    IndexT iterator;
    IndexT activeBuffer;
    SizeT scratchMemorySize;
    void** scratchMemory;
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
    template <typename CTX> friend void JobDispatch(const JobFunc& func, const SizeT numInvocations, const SizeT groupSize, const CTX& context, const Util::FixedArray<const Threading::AtomicCounter*>& waitCounters, Threading::AtomicCounter* doneCounter, Threading::Event* signalEvent);
    template <typename CTX> friend void JobDispatch(const JobFunc& func, const SizeT numInvocations, const CTX& context, const Util::FixedArray<const Threading::AtomicCounter*>& waitCounters, Threading::AtomicCounter* doneCounter, Threading::Event* signalEvent);
    friend void JobEndSequence(Threading::Event* signalEvent);

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

    SizeT scratchMemorySize;
    SizeT numBuffers;

    JobSystemInitInfo()
        : numThreads(1)
        , affinity(0xFFFFFFFF)
        , priority(UINT_MAX)
        , scratchMemorySize(1_MB)
        , numBuffers(1)
    {};
};

/// Create a new job port
void JobSystemInit(const JobSystemInitInfo& info);
/// Destroy job port
void JobSystemUninit();

/// Allocate memory and progress memory iterator
char* __Alloc(SizeT size);
/// Progress to new buffer
void JobNewFrame();

/// Dispatch job
template <typename CTX> void JobDispatch(
    const JobFunc& func
    , const SizeT numInvocations
    , const SizeT groupSize
    , const CTX& context
    , const Util::FixedArray<const Threading::AtomicCounter*>& waitCounters = nullptr
    , Threading::AtomicCounter* doneCounter = nullptr
    , Threading::Event* signalEvent = nullptr
);
/// Dispatch job as a single group (to be run on a single thread)
template <typename CTX> void JobDispatch(
    const JobFunc& func
    , const SizeT numInvocations
    , const CTX& context
    , const Util::FixedArray<const Threading::AtomicCounter*>& waitCounters = nullptr
    , Threading::AtomicCounter* doneCounter = nullptr
    , Threading::Event* signalEvent = nullptr
);

extern Util::FixedArray<const Threading::AtomicCounter*> sequenceWaitCounters;

/// Begin a sequence of jobs
void JobBeginSequence(const Util::FixedArray<const Threading::AtomicCounter*>& waitCounters = nullptr);

/// Append job to sequence with an automatic dependency on the previous job
template <typename CTX> void JobAppendSequence(
    const JobFunc& func
    , const SizeT numInvocations
    , const SizeT groupSize
    , const CTX& context
);

/// Append job to sequence with an automatic dependency on the previous job, to run on a single thread
template <typename CTX> void JobAppendSequence(
    const JobFunc& func
    , const SizeT numInvocations
    , const CTX& context
);

/// Flush queued jobs
void JobEndSequence(Threading::Event* signalEvent = nullptr);

//------------------------------------------------------------------------------
/**
*/
template <typename T> T*
__Alloc(SizeT size)
{
    n_assert(ctx.iterator + size < ctx.scratchMemorySize);
    T* ret = (T*)ctx.scratchMemory[ctx.activeBuffer] + ctx.iterator;
    ctx.iterator += size;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
template <typename CTX> void
JobDispatch(const JobFunc& func, const SizeT numInvocations, const SizeT groupSize, const CTX& context, const Util::FixedArray<const Threading::AtomicCounter*>& waitCounters, Threading::AtomicCounter* doneCounter, Threading::Event* signalEvent)
{
    static_assert(std::is_trivially_destructible<CTX>::value, "Job context has to be trivially destructible");
    n_assert(doneCounter != nullptr ? *doneCounter > 0 : true);

    // Calculate the number of actual jobs based on invocations and group size
    SizeT numJobs = Math::ceil(numInvocations / float(groupSize));

    // Calculate allocation size which is node + counters + data context
    auto dynamicAllocSize = sizeof(JobNode) + sizeof(CTX) + waitCounters.Size() * sizeof(const Threading::AtomicCounter*);
    auto mem = __Alloc<char>(dynamicAllocSize);
    auto node = (JobNode*)mem;

    // Copy over wait counters
    node->job.waitCounters = nullptr;
    if (waitCounters.Size() > 0)
    {
        node->job.waitCounters = (const Threading::AtomicCounter**)(mem + sizeof(JobNode) + sizeof(CTX));
        memcpy(node->job.waitCounters, waitCounters.Begin(), waitCounters.Size() * sizeof(const Threading::AtomicCounter*));
    }

    // Move context
    node->job.data = (void*)(mem + sizeof(JobNode));
    auto data = reinterpret_cast<CTX*>(node->job.data);
    *data = context;

    node->job.func = func;
    node->job.remainingGroups = numJobs;
    node->job.groupCompletionCounter = numJobs;
    node->job.numInvocations = numInvocations;
    node->job.groupSize = groupSize;
    node->job.numWaitCounters = (SizeT)waitCounters.Size();
    node->job.doneCounter = doneCounter;
    node->job.signalEvent = signalEvent;
    node->sequenceWaitValue = 0;

    // Add to end of linked list
    ctx.jobLock.Enter();

    // First, set head node if nullptr
    if (ctx.head == nullptr)
        ctx.head = node;

    // Then add node to end of list
    node->next = nullptr;
    if (ctx.tail != nullptr)
        ctx.tail->next = node;
    ctx.tail = node;

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
JobDispatch(const JobFunc& func, const SizeT numInvocations, const CTX& context, const Util::FixedArray<const Threading::AtomicCounter*>& waitCounters, Threading::AtomicCounter* doneCounter, Threading::Event* signalEvent)
{
    JobDispatch(func, numInvocations, numInvocations, context, waitCounters, doneCounter, signalEvent);
}

//------------------------------------------------------------------------------
/**
*/
template<typename CTX> void 
JobAppendSequence(const JobFunc& func, const SizeT numInvocations, const SizeT groupSize, const CTX& context)
{
    static_assert(std::is_trivially_destructible<CTX>::value, "Job context has to be trivially destructible");

    // Calculate the number of actual jobs based on invocations and group size
    SizeT numJobs = Math::ceil(numInvocations / float(groupSize));

    // Calculate allocation size which is node + counters + data context
    auto dynamicAllocSize = sizeof(JobNode) + sizeof(CTX);
    if (sequenceWaitCounters.IsEmpty())
        dynamicAllocSize += sizeof(Threading::AtomicCounter);
    else
        dynamicAllocSize += sequenceWaitCounters.ByteSize();

    auto mem = __Alloc<char>(dynamicAllocSize);
    auto node = (JobNode*)mem;

    // Setup pointer to wait counters
    node->job.waitCounters = (const Threading::AtomicCounter**)(mem + sizeof(JobNode) + sizeof(CTX));

    // If we have sequence wait counters, add them
    if (!sequenceWaitCounters.IsEmpty())
    {
        node->job.numWaitCounters = sequenceWaitCounters.Size();
        memcpy(node->job.waitCounters, sequenceWaitCounters.Begin(), sequenceWaitCounters.ByteSize());

        // Then clear them, we only allow wait counters for the first job in the sequence
        sequenceWaitCounters.Clear();
    }

    // Move context
    node->job.data = (void*)(mem + sizeof(JobNode));
    auto data = reinterpret_cast<CTX*>(node->job.data);
    *data = context;

    node->job.func = func;
    node->job.remainingGroups = numJobs;
    node->job.groupCompletionCounter = numJobs;
    node->job.numInvocations = numInvocations;
    node->job.groupSize = groupSize;
    node->job.numWaitCounters = sequenceWaitCounters.IsEmpty() ? 0 : sequenceWaitCounters.Size();
    node->job.waitCounters = nullptr;
    node->job.doneCounter = nullptr;
    node->job.signalEvent = nullptr;

    if (!ctx.queuedJobs.IsEmpty())
    {
        node->job.numWaitCounters++;
        node->job.waitCounters = (const Threading::AtomicCounter**)&ctx.queuedJobs.Back()->job.doneCounter;
    }

    // Queue job
    ctx.queuedJobs.Append(node);
}

//------------------------------------------------------------------------------
/**
*/
template<typename CTX> void 
JobAppendSequence(const JobFunc& func, const SizeT numInvocations, const CTX& context)
{
    JobAppendSequence(func, numInvocations, numInvocations, context);
}

} // namespace Jobs2
