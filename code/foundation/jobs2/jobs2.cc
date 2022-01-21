//------------------------------------------------------------------------------
//  @file jobs2.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "threading/criticalsection.h"
#include "memory/arenaallocator.h"
#include "profiling/profiling.h"
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
    Profiling::ProfilingRegisterThread();
    while (!this->ThreadStopRequested())
    {
wait:
        // Wait for jobs to come
        this->wakeupEvent.Wait();

next:
        ctx.jobLock.Enter();
        auto node = ctx.head;
        auto dragging = ctx.head;

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
                dependencyDone &= *node->job.waitCounters[i] == node->sequenceWaitValue;

            if (!dependencyDone)
            {
                dragging = node;
                node = node->next;
                continue;
            }
            n_assert(node->job.remainingGroups > 0);

            // If we are consuming the last job packet in this run, disconnect the node
            jobIndex = --node->job.remainingGroups;
            if (node->job.remainingGroups == 0)
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

            // If we fall through it means we found a job
            break;
        }
        ctx.jobLock.Leave();

        // If jobIndex is -1 it means this thread didn't win the race
        n_assert(jobIndex != -1);

        // Run function
        node->job.func(node->job.numInvocations, node->job.groupSize, jobIndex, jobIndex * node->job.groupSize, node->job.data);

        // Decrement number of finished jobs, and if this was the last one, signal the finished event
        long numJobsLeft = Threading::Interlocked::Decrement(&node->job.groupCompletionCounter);
        if (numJobsLeft == 0)
        {
            // If we have a job counter, only signal the event when the counter reaches 0
            if (node->job.doneCounter != nullptr)
            {
                long numDispatchesLeft = Threading::Interlocked::Decrement(node->job.doneCounter);

                if (node->job.signalEvent != nullptr && numDispatchesLeft == 0)
                    node->job.signalEvent->Signal();
            }
            else
            {
                // If we don't have a counter, just signal it when we're done with this dispatch
                if (node->job.signalEvent != nullptr)
                    node->job.signalEvent->Signal();
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
        thread->SetName(Util::String::Sprintf("%s #%d", info.name.Value(), i));
        thread->SetThreadAffinity(info.affinity);
        thread->Start();
        ctx.threads[i] = thread;
    }

    ctx.numBuffers = info.numBuffers;
    ctx.iterator = 0;
    ctx.activeBuffer = 0;
    ctx.scratchMemory = (void**)Memory::Alloc(Memory::ObjectHeap, info.numBuffers * sizeof(void*));
    ctx.scratchMemorySize = info.scratchMemorySize;
    for (IndexT i = 0; i < info.numBuffers; i++)
    {
        ctx.scratchMemory[i] = Memory::Alloc(Memory::ObjectHeap, info.scratchMemorySize);
    }
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
}

Util::FixedArray<const Threading::AtomicCounter*> sequenceWaitCounters;
Threading::AtomicCounter* sequenceCompletionCounter;
//------------------------------------------------------------------------------
/**
*/
void
JobBeginSequence(const Util::FixedArray<const Threading::AtomicCounter*>& waitCounters)
{
    n_assert_fmt(sequenceWaitCounters.IsEmpty(), "JobBeginSequence called twice, missing JobEndSequence");
    sequenceWaitCounters = waitCounters;
    sequenceCompletionCounter = JobAlloc<Threading::AtomicCounter>(1);
    *sequenceCompletionCounter = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
JobEndSequence(Threading::Event* signalEvent)
{
    if (!ctx.queuedJobs.IsEmpty())
    {
        // Set signaling event on the first job, which will end up being the last in the chain
        ctx.queuedJobs.Back()->job.signalEvent = signalEvent;

        // Setup sequence completion counter
        n_assert(*sequenceCompletionCounter == 0);
        *sequenceCompletionCounter = ctx.queuedJobs.Size();

        ctx.jobLock.Enter();
        JobNode* node = nullptr;

        for (IndexT i = 0; i < ctx.queuedJobs.Size(); i++)
        {
            node = ctx.queuedJobs[i];
            node->job.doneCounter = sequenceCompletionCounter;

            // Add wait value for the sequence
            if (i > 0)
            {
                node->job.waitCounters[0] = sequenceCompletionCounter;
                // We want to wait for the nth minus i job to finish to progress this job
                node->sequenceWaitValue = *sequenceCompletionCounter - i; 
            }

            // First, set head node if nullptr
            if (ctx.head == nullptr)
                ctx.head = node;

            // Then add node to end of list
            node->next = nullptr;
            if (ctx.tail != nullptr)
                ctx.tail->next = node;
            ctx.tail = node;
        }
        ctx.queuedJobs.Clear();

        // Move head pointer to last element in the list
        ctx.jobLock.Leave();

        // Trigger threads to wake up and compete for jobs
        for (Ptr<JobThread>& thread : ctx.threads)
        {
            thread->EmitWakeupSignal();
        }
    }
}

} // namespace Jobs2
