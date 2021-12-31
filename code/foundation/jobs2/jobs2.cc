//------------------------------------------------------------------------------
//  @file jobs2.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "threading/criticalsection.h"
#include "threading/interlocked.h"
#include "memory/arenaallocator.h"
#include "jobs2.h"
namespace Jobs2
{

struct JobContext
{
    JobFunc func;
    int remainingGroups;
    SizeT numInvocations;
    SizeT groupSize;
    volatile long groupCompletionCounter;
    void* data;
    Threading::Event* waitEvent;
    Threading::Event* signalEvent;
};

struct JobNode
{
    JobNode* next;
    JobContext job;
};

struct
{
    Threading::CriticalSection jobLock;
    Util::Array<JobContext> jobs;
    Memory::ArenaAllocator<0x100> nodeAllocator;
    JobNode* head;
    Util::FixedArray<Ptr<JobThread>> threads;

} ctx;

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
            // If there are no more jobs, it means this thread lost
            // the race to find more work, so go back to waiting for work again
            if (ctx.head == nullptr)
            {
                ctx.jobLock.Leave();
                goto wait;
            }

            // We traversed the whole list, so let's go back to where we began
            if (node == nullptr)
            {
                ctx.jobLock.Leave();
                goto next;
            }

            bool dependencyDone = true;
            if (node->job.waitEvent != nullptr)
                dependencyDone = node->job.waitEvent->Peek();

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
                    dragging->next = next;
                }
                else
                {
                    // If head, then just unlink the head node
                    ctx.head = next;
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
            if (node->job.signalEvent != nullptr)
                node->job.signalEvent->Signal();

            // Delete node
            delete node;
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
JobDispatch(const JobFunc& func, const SizeT numInvocations, const SizeT groupSize, void* context, Threading::Event* waitEvent, Threading::Event* signalEvent)
{
    // Calculate the number of actual jobs based on invocations and group size
    if (waitEvent != nullptr)
        n_assert_fmt(waitEvent->IsManual(), "Wait event must be manual reset because job system needs to peek without changing it's state");
    SizeT numJobs = Math::ceil(numInvocations / float(groupSize));

    // Setup job context
    JobContext jctx;
    jctx.func = func;
    jctx.remainingGroups = numJobs;
    jctx.groupCompletionCounter = numJobs;
    jctx.numInvocations = numInvocations;
    jctx.groupSize = groupSize;
    jctx.waitEvent = waitEvent;
    jctx.signalEvent = signalEvent;
    jctx.data = context;

    JobNode* node = new JobNode;
    node->job = jctx;

    ctx.jobLock.Enter();
    node->next = ctx.head;
    ctx.head = node;
    ctx.jobLock.Leave();

    /*
    // Push job to job list
    ctx.jobLock.Enter();
    ctx.jobs.Append(jctx);
    ctx.jobLock.Leave();
    */

    // Trigger threads to wake up and compete for jobs
    for (Ptr<JobThread>& thread : ctx.threads)
    {
        thread->EmitWakeupSignal();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
JobDispatch(const JobFunc& func, const SizeT numInvocations, void* context, Threading::Event* waitEvent, Threading::Event* signalEvent)
{
    // Setup job context
    if (waitEvent != nullptr)
        n_assert_fmt(waitEvent->IsManual(), "Wait event must be manual reset because job system needs to peek without changing it's state");
    JobContext jctx;
    jctx.func = func;
    jctx.remainingGroups = 1;
    jctx.groupCompletionCounter = 1;
    jctx.numInvocations = numInvocations;
    jctx.groupSize = numInvocations;
    jctx.waitEvent = waitEvent;
    jctx.signalEvent = signalEvent;
    jctx.data = context;

    JobNode* node = new JobNode;
    node->job = jctx;

    ctx.jobLock.Enter();
    node->next = ctx.head;
    ctx.head = node;
    ctx.jobLock.Leave();

    // Push job to job list
    /*
    ctx.jobLock.Enter();
    ctx.jobs.Append(jctx);
    ctx.jobLock.Leave();
    */

    // Trigger threads to wake up and compete for jobs
    for (Ptr<JobThread>& thread : ctx.threads)
    {
        thread->EmitWakeupSignal();
    }
}

} // namespace Jobs2
