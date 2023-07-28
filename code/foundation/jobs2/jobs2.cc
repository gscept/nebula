//------------------------------------------------------------------------------
//  @file jobs2.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "profiling/profiling.h"
#include "io/ioserver.h"
#include "jobs2.h"

namespace Jobs2
{

Jobs2Context ctx;

__ImplementClass(Jobs2::JobThread, 'J2TH', Threading::Thread);
//------------------------------------------------------------------------------
/**
*/
JobThread::JobThread()
    : wakeupEvent{ false }
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
JobThread::~JobThread()
{
    if (this->IsRunning())
    {
        this->Stop();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
JobThread::SignalWorkAvailable()
{
    this->wakeupEvent.Signal();
}

//------------------------------------------------------------------------------
/**
*/
void
JobThread::EmitWakeupSignal()
{
    this->wakeupEvent.Signal();
}

//------------------------------------------------------------------------------
/**
*/
void
JobThread::DoWork()
{
    if (this->enableIo)
        IO::IoServer::Create();
    if (this->enableProfiling)
        Profiling::ProfilingRegisterThread();
    while (true)
    {
wait:
        // Wait for jobs to come
        this->wakeupEvent.Wait();
        if (this->ThreadStopRequested())
            return;

next:
        ctx.jobLock.Enter();
        auto node = ctx.head;
        auto dragging = ctx.head;
        Jobs2::JobContext* job = nullptr;

        //auto it = ctx.jobs.Begin();
        int64_t jobIndex = -1;
        while (true)
        {
            // If head is nullptr, we either lost the race to grab a job
            // or we traversed the whole list but found no job which has it's dependency satisfied,
            // so let's wait 
            if (ctx.head == nullptr || node == nullptr)
            {
                ctx.jobLock.Leave();
                goto wait;
            }

            bool dependencyDone = true;

            // Check dependencies
            for (IndexT i = 0; i < node->job.numWaitCounters; i++)
                dependencyDone &= *node->job.waitCounters[i] == 0;

            // If dependency is not done, update iterators and progress
            if (!dependencyDone)
            {
                dragging = node;
                node = node->next;
                continue;
            }

            if (node->sequence != nullptr)
            {
                job = &node->sequence->job;
                n_assert(job->remainingGroups > 0);

                // If wait hasn't been met, continue to next node in the main list
                if (job->numWaitCounters == 1 && *job->waitCounters[0] != 0)
                {
                    dragging = node;
                    node = node->next;
                    continue;
                }

                // If we are consuming the last packet in the sequence, simply point the sequence to the next node
                jobIndex = --job->remainingGroups;
                if (job->remainingGroups == 0)
                {
                    node->sequence = node->sequence->next;
                }
            }
            else
            {
                job = &node->job;
                n_assert(job->remainingGroups > 0);

                // If we are consuming the last job packet in this run, disconnect the node
                jobIndex = --job->remainingGroups;
                if (job->remainingGroups == 0)
                {
                    // Update node pointers
                    auto next = node->next;

                    // If node is not head, unlink the node between dragging and the current
                    if (node != ctx.head)
                    {
                        // If node is the last one in the list, update tail
                        if (node == ctx.tail)
                            ctx.tail = dragging;
                        dragging->next = next;
                    }
                    else
                    {
                        // If head, then just unlink the head node
                        ctx.head = next;

                        // If tail and node point to the same node, unlink tail
                        if (node == ctx.tail)
                            ctx.tail = nullptr;
                    }
                }
            }

            // If we fall through it means we found a job
            break;
        }
        ctx.jobLock.Leave();

        // If jobIndex is -1, it means we're trying to run a function on a finished job
        n_assert(jobIndex != -1);

        // Run function
        job->func(job->numInvocations, job->groupSize, jobIndex, jobIndex * job->groupSize, job->data);

        // Decrement number of finished jobs, and if this was the last one, signal the finished event
        if (Threading::Interlocked::Decrement(&job->groupCompletionCounter) == 0)
        {
            // If we have a job counter, only signal the event when the counter reaches 0
            if (job->doneCounter != nullptr)
            {
                long numDispatchesLeft = Threading::Interlocked::Decrement(job->doneCounter);

                if (job->signalEvent != nullptr && numDispatchesLeft == 0)
                    job->signalEvent->Signal();
            }
            else
            {
                // If we don't have a counter, just signal it when we're done with this dispatch
                if (job->signalEvent != nullptr)
                    job->signalEvent->Signal();
            }

            // Since other threads might be waiting for this job to finish, trigger other threads to wake up
            for (Ptr<JobThread>& thread : ctx.threads)
            {
                thread->EmitWakeupSignal();
            }
        }

        // Do another
        goto next;
    }
}

N_DECLARE_COUNTER(N_JOBS2_MEMORY_COUNTER, Jobs2RingBufferMemory)

//------------------------------------------------------------------------------
/**
*/
void
JobSystemInit(const JobSystemInitInfo& info)
{
    // Setup job system threads
    ctx.threads.Resize(info.numThreads);
    for (IndexT i = 0; i < info.numThreads; i++)
    {
        Ptr<JobThread> thread = JobThread::Create();
        thread->enableIo = info.enableIo;
        thread->enableProfiling = info.enableProfiling;
        thread->SetName(Util::String::Sprintf("%s #%d", info.name.Value(), i));
        thread->SetThreadAffinity(info.affinity);
        thread->Start();
        ctx.threads[i] = thread;
    }

    ctx.numBuffers = info.numBuffers;
    ctx.iterator = 0;
    ctx.activeBuffer = 0;
    ctx.scratchMemory.Resize(info.numBuffers);
    ctx.scratchMemorySize = info.scratchMemorySize;
    for (IndexT i = 0; i < info.numBuffers; i++)
    {
        ctx.scratchMemory[i] = (byte*)Memory::Alloc(Memory::ObjectHeap, info.scratchMemorySize);
    }
    N_BUDGET_COUNTER_SETUP(N_JOBS2_MEMORY_COUNTER, info.scratchMemorySize);
    ctx.tail = nullptr;
    ctx.head = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
JobSystemUninit()
{
    for (Ptr<JobThread>& thread : ctx.threads)
    {
        thread->Stop();
    }
    ctx.threads.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
JobNewFrame()
{
    ctx.iterator = 0;
    ctx.activeBuffer = (ctx.activeBuffer + 1) % ctx.numBuffers;
    N_BUDGET_COUNTER_RESET(N_JOBS2_MEMORY_COUNTER);
}

//------------------------------------------------------------------------------
/**
*/
void*
JobAlloc(SizeT bytes)
{
    // make sure to always pad to next 16 byte alignment in case the 
    // context used needs to be aligned
    bytes = Math::alignptr(bytes, 16);
    n_assert((ctx.iterator + bytes) < ctx.scratchMemorySize);
    void* ret = (ctx.scratchMemory[ctx.activeBuffer] + ctx.iterator);
    ctx.iterator += bytes;
    N_BUDGET_COUNTER_INCR(N_JOBS2_MEMORY_COUNTER, bytes);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
JobSequencePlaceholder(SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx)
{
    // Do nothing, this is just held by a sequence node to provide validity
}

Jobs2::JobNode* sequenceNode = nullptr;
Jobs2::JobNode* sequenceTail = nullptr;
const Threading::AtomicCounter* prevDoneCounter = nullptr;
Threading::ThreadId sequenceThread;

//------------------------------------------------------------------------------
/**
*/
void
JobBeginSequence(
    const Util::FixedArray<const Threading::AtomicCounter*>& waitCounters
    , Threading::AtomicCounter* doneCounter
    , Threading::Event* signalEvent)
{
    n_assert(sequenceNode == nullptr);
    n_assert(sequenceTail == nullptr);
    sequenceThread = Threading::Thread::GetMyThreadId();

    // Calculate allocation size which is node + counters
    SizeT dynamicAllocSize = sizeof(JobNode) + waitCounters.Size() * sizeof(const Threading::AtomicCounter*);
    auto mem = JobAlloc<char>(dynamicAllocSize);
    sequenceNode = (JobNode*)mem;
    sequenceNode->next = nullptr;
    sequenceNode->sequence = nullptr;
    sequenceTail = nullptr;

    // Copy over wait counters
    sequenceNode->job.waitCounters = nullptr;
    if (waitCounters.Size() > 0)
    {
        sequenceNode->job.waitCounters = (const Threading::AtomicCounter**)(mem + sizeof(JobNode));
        memcpy(sequenceNode->job.waitCounters, waitCounters.Begin(), waitCounters.Size() * sizeof(const Threading::AtomicCounter*));
    }

    // Setup sequence node with a dummy function
    sequenceNode->job.func = JobSequencePlaceholder;
    sequenceNode->job.remainingGroups = 1;
    sequenceNode->job.numInvocations = 0;
    sequenceNode->job.groupSize = 0;
    sequenceNode->job.data = (void*)(mem + sizeof(JobNode));
    sequenceNode->job.numWaitCounters = (SizeT)waitCounters.Size();
    sequenceNode->job.doneCounter = doneCounter;
    sequenceNode->job.signalEvent = signalEvent;
}

//------------------------------------------------------------------------------
/**
*/
void
JobEndSequence(Threading::Event* signalEvent)
{
    n_assert(sequenceNode != nullptr);
    n_assert(sequenceThread == Threading::Thread::GetMyThreadId());
    if (sequenceNode->sequence != nullptr)
    {
        // Move head pointer to last element in the list
        ctx.jobLock.Enter();

        sequenceTail->job.doneCounter = sequenceNode->job.doneCounter;
        sequenceNode->job.doneCounter = nullptr;
        sequenceTail->job.signalEvent = sequenceNode->job.signalEvent;
        sequenceNode->job.signalEvent = nullptr;

        // When sequence is chained, set the head pointer
        if (ctx.head == nullptr)
            ctx.head = sequenceNode;

        // Then add sequence to the end of the current list
        if (ctx.tail != nullptr)
            ctx.tail->next = sequenceNode;

        // Finally repoint tail
        ctx.tail = sequenceNode;

        ctx.jobLock.Leave();
    }
    prevDoneCounter = nullptr;
    sequenceNode = nullptr;
    sequenceTail = nullptr;
}

} // namespace Jobs2
