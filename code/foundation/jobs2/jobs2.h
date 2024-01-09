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

#define JOB_SIGNATURE (SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset)
#define JOB_BEGIN_LOOP \
for (IndexT i = 0; i < groupSize; i++)\
{\
    IndexT __INDEX = i + invocationOffset;\
    if (__INDEX >= totalJobs)\
        return;\

#define JOB_END_LOOP \
}

#define JOB_ITEM_INDEX __INDEX


class JobThread;

void* JobAlloc(SizeT bytes);
typedef volatile long CompletionCounter;
using JobFunc = void(*)(SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx);

template <typename... ARGS>
struct CallableStub
{
    virtual void invoke(ARGS... args) = 0;
};

template<typename LAMBDA, typename... ARGS>
struct Callable : CallableStub<ARGS...>
{
    LAMBDA l;

    Callable(LAMBDA l) : l(std::move(l)) {};

    void invoke(ARGS... args) override
    {
        l(args...);
    }
};

struct Lambda
{
    CallableStub<SizeT, SizeT, IndexT, SizeT>* callable;

    template<typename LAMBDA>
    Lambda(LAMBDA l)
    {
        using CallableType = Callable<LAMBDA, SizeT, SizeT, IndexT, SizeT>;
        this->callable = (CallableType*)Jobs2::JobAlloc(sizeof(CallableType));
        new (this->callable) CallableType(l);
    };

    void operator()(SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset)
    {
        callable->invoke(totalJobs, groupSize, groupIndex, invocationOffset);
    }
};

struct JobContext
{
    JobFunc func;
    Lambda l;
    int remainingGroups;
    Threading::AtomicCounter groupCompletionCounter;
    SizeT numInvocations;
    SizeT groupSize;
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
    JobNode* sequence; // set to nullptr for ordinary nodes
};

struct Jobs2Context
{
    Threading::CriticalSection jobLock;
    JobNode* head = nullptr;
    JobNode* tail = nullptr;
    Util::FixedArray<Ptr<JobThread>> threads;
    Util::Array<JobNode*> queuedJobs;

    SizeT numBuffers;
    IndexT iterator;
    IndexT activeBuffer;
    Util::FixedArray<byte*> scratchMemory;
    SizeT scratchMemorySize;
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

    /// Signal new work available
    void SignalWorkAvailable();
    
    bool enableIo;
    bool enableProfiling;
protected:

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

    bool enableIo;
    bool enableProfiling;

    JobSystemInitInfo()
        : numThreads(1)
        , affinity(0xFFFFFFFF)
        , priority(UINT_MAX)
        , scratchMemorySize(1_MB)
        , numBuffers(1)
        , enableIo(false)
        , enableProfiling(true)
    {};
};

/// Create a new job port
void JobSystemInit(const JobSystemInitInfo& info);
/// Destroy job port
void JobSystemUninit();

/// Allocate memory and progress memory iterator
template <typename T> T* JobAlloc(SizeT count);
/// Allocate memory
void* JobAlloc(SizeT bytes);
/// Progress to new buffer
void JobNewFrame();

extern JobNode* sequenceNode;
extern JobNode* sequenceTail;
extern const Threading::AtomicCounter* prevDoneCounter;
extern Threading::ThreadId sequenceThread;

/// Begin a sequence of jobs
void JobBeginSequence(const Util::FixedArray<const Threading::AtomicCounter*>& waitCounters = nullptr
    , Threading::AtomicCounter* doneCounter = nullptr
    , Threading::Event* signalEvent = nullptr);

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
JobAlloc(SizeT count)
{
    return (T*)JobAlloc(count * sizeof(T));
}

//------------------------------------------------------------------------------
/**
*/
template <typename LAMBDA> void
JobDispatch(
    LAMBDA&& func
    , const SizeT numInvocations
    , const SizeT groupSize
    , const Util::FixedArray<const Threading::AtomicCounter*>& waitCounters = nullptr
    , Threading::AtomicCounter* doneCounter = nullptr
    , Threading::Event* signalEvent = nullptr
)
{
    n_assert(numInvocations > 0);
    n_assert(doneCounter != nullptr ? *doneCounter > 0 : true);

    // Calculate the number of actual jobs based on invocations and group size
    SizeT numJobs = Math::ceil(numInvocations / float(groupSize));

    // Calculate allocation size which is node + counters + data context
    SizeT dynamicAllocSize = sizeof(JobNode) + waitCounters.Size() * sizeof(const Threading::AtomicCounter*);
    auto mem = JobAlloc<char>(dynamicAllocSize);
    auto node = (JobNode*)mem;

    // Copy over wait counters
    node->job.waitCounters = nullptr;
    if (waitCounters.Size() > 0)
    {
        node->job.waitCounters = (const Threading::AtomicCounter**)(mem + sizeof(JobNode));
        memcpy(node->job.waitCounters, waitCounters.Begin(), waitCounters.Size() * sizeof(const Threading::AtomicCounter*));
    }

    node->job.l = std::move(func);
    node->job.func = nullptr;
    node->job.remainingGroups = numJobs;
    node->job.groupCompletionCounter = numJobs;
    node->job.numInvocations = numInvocations;
    node->job.groupSize = groupSize;
    node->job.numWaitCounters = (SizeT)waitCounters.Size();
    node->job.doneCounter = doneCounter;
    node->job.signalEvent = signalEvent;
    node->sequence = nullptr;

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
        thread->SignalWorkAvailable();
    }
}

//------------------------------------------------------------------------------
/**
*/
template <typename LAMBDA> void
JobDispatch(
    LAMBDA&& func
    , const SizeT numInvocations
    , const Util::FixedArray<const Threading::AtomicCounter*>& waitCounters = nullptr
    , Threading::AtomicCounter* doneCounter = nullptr
    , Threading::Event* signalEvent = nullptr
)
{
    JobDispatch(func, numInvocations, numInvocations, waitCounters, doneCounter, signalEvent);
}

//------------------------------------------------------------------------------
/**
*/
template <typename CTX> void
JobDispatch(
    const JobFunc& func
    , const SizeT numInvocations
    , const SizeT groupSize
    , const CTX& context
    , const Util::FixedArray<const Threading::AtomicCounter*>& waitCounters = nullptr
    , Threading::AtomicCounter* doneCounter = nullptr
    , Threading::Event* signalEvent = nullptr
)
{
    static_assert(std::is_trivially_destructible<CTX>::value, "Job context has to be trivially destructible");
    n_assert(numInvocations > 0);
    n_assert(doneCounter != nullptr ? *doneCounter > 0 : true);

    // Calculate the number of actual jobs based on invocations and group size
    SizeT numJobs = Math::ceil(numInvocations / float(groupSize));

    // Calculate allocation size which is node + counters + data context
    auto dynamicAllocSize = sizeof(JobNode) + sizeof(CTX) + waitCounters.Size() * sizeof(const Threading::AtomicCounter*);
    auto mem = JobAlloc<char>(dynamicAllocSize);
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

    node->job.l.callable = nullptr;
    node->job.func = func;
    node->job.remainingGroups = numJobs;
    node->job.groupCompletionCounter = numJobs;
    node->job.numInvocations = numInvocations;
    node->job.groupSize = groupSize;
    node->job.numWaitCounters = (SizeT)waitCounters.Size();
    node->job.doneCounter = doneCounter;
    node->job.signalEvent = signalEvent;
    node->sequence = nullptr;

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
        thread->SignalWorkAvailable();
    }
}

//------------------------------------------------------------------------------
/**
*/
template <typename CTX> void
JobDispatch(
    const JobFunc& func
    , const SizeT numInvocations
    , const CTX& context
    , const Util::FixedArray<const Threading::AtomicCounter*>& waitCounters = nullptr
    , Threading::AtomicCounter* doneCounter = nullptr
    , Threading::Event* signalEvent = nullptr
)
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
    n_assert(numInvocations > 0);
    n_assert(sequenceThread == Threading::Thread::GetMyThreadId());
    n_assert(sequenceNode != nullptr);

    // Calculate the number of actual jobs based on invocations and group size
    SizeT numJobs = Math::ceil(numInvocations / float(groupSize));

    // Calculate allocation size which is node + counters + data context
    SizeT dynamicAllocSize = sizeof(JobNode) + sizeof(CTX) + sizeof(Threading::AtomicCounter);
    if (prevDoneCounter != nullptr)
        dynamicAllocSize += sizeof(Threading::AtomicCounter*);
    auto mem = JobAlloc<char>(dynamicAllocSize);
    auto node = (JobNode*)mem;

    if (prevDoneCounter != nullptr)
    {
        node->job.numWaitCounters = 1;
        node->job.waitCounters = (const Threading::AtomicCounter**)(mem + sizeof(JobNode) + sizeof(CTX) + sizeof(Threading::AtomicCounter));
        memcpy(node->job.waitCounters, &prevDoneCounter, sizeof(Threading::AtomicCounter*));
    }
    else
    {
        node->job.numWaitCounters = 0;
        node->job.waitCounters = nullptr;
    }

    // Move context
    node->job.data = (void*)(mem + sizeof(JobNode));
    auto data = reinterpret_cast<CTX*>(node->job.data);
    *data = context;

    node->job.l.callable = nullptr;
    node->job.func = func;
    node->job.remainingGroups = numJobs;
    node->job.groupCompletionCounter = numJobs;
    node->job.numInvocations = numInvocations;
    node->job.groupSize = groupSize;
    
    node->job.doneCounter = (Threading::AtomicCounter*)(mem + sizeof(JobNode) + sizeof(CTX));
    *node->job.doneCounter = 1;
    prevDoneCounter = node->job.doneCounter;
    node->job.signalEvent = nullptr;
    node->next = nullptr;

    // The remainingGroups counter for the sequence node is the length of the sequence chain
    if (sequenceTail == nullptr)
        sequenceNode->sequence = node;
    else
        sequenceTail->next = node;

    sequenceTail = node;

    // Queue job
    //ctx.queuedJobs.Append(node);
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
