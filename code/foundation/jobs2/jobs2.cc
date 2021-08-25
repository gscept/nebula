//------------------------------------------------------------------------------
//  @file jobs2.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "threading/criticalsection.h"
#include "threading/interlocked.h"
#include "jobs2.h"
namespace Jobs2
{

struct JobContext
{
    JobFunc func;
    SizeT remainingGroups;
    SizeT numInvocations;
    SizeT groupSize;
    volatile long groupCompletionCounter;
    void* data;
    Threading::Event* waitEvent;
    Threading::Event* signalEvent;
};

struct
{
    Threading::CriticalSection jobLock;
    Util::Array<JobContext> jobs;
    Util::FixedArray<Ptr<JobThread>> threads;
} ctx;

//------------------------------------------------------------------------------
/**
*/
JobThread::JobThread()
    : wakeupEvent{ false }
{
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
start:
        // Wait for jobs to come
        this->wakeupEvent.Wait();

        // Enter critical section for picking a job
        ctx.jobLock.Enter();
        auto it = ctx.jobs.Begin();
        int64_t jobIndex = -1;
        while (true)
        {
            // If there are no more jobs, go back to waiting for more jobs
            if (it == ctx.jobs.End())
                goto start;

            bool dependencyDone = true;
            if (it->waitEvent != nullptr)
                dependencyDone = it->waitEvent->Peek();

            // If there are no remaining groups or the dependency is not done, go to next job
            if (it->remainingGroups == 0 || !dependencyDone)
            {
                auto next = it + 1;

                // In the event there are no more groups left to execute, remove this job
                if (it->remainingGroups == 0)
                    ctx.jobs.Erase(it);
                it = next;
                continue;
            }

            jobIndex = --it->remainingGroups;
            break;
        }
        ctx.jobLock.Leave();

        // If jobIndex is -1 it means this thread didn't win the race
        assert(jobIndex != -1);

        // Run function
        it->func(it->numInvocations, it->groupSize, jobIndex, it->data);

        // Decrement number of finished jobs, and if this was the last one, signal the finished event
        long numJobsLeft = Threading::Interlocked::Decrement(&it->groupCompletionCounter);
        if (numJobsLeft == 0)
            it->signalEvent->Signal();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CreateJobSystem(const CreateJobSystemInfo& info)
{
    // Setup job system threads
    ctx.threads.Resize(info.numThreads);
    for (IndexT i = 0; i < info.numThreads; i++)
    {
        Ptr<JobThread> thread = JobThread::Create();
        thread->SetName(Util::String::Sprintf("%s #%d", info.name.Value(), i));
        thread->SetThreadAffinity(info.affinity);
        thread->Start();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyJobSystem()
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
JobPortDispatch(const JobFunc& func, const SizeT numInvocations, const SizeT groupSize, void* context, Threading::Event* waitEvent, Threading::Event* signalEvent)
{
    // Calculate the number of actual jobs based on invocations and group size
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

    // Push job to job list
    ctx.jobLock.Enter();
    ctx.jobs.Append(jctx);
    ctx.jobLock.Leave();

    // Trigger threads to wake up and compete for jobs
    for (Ptr<JobThread>& thread : ctx.threads)
    {
        thread->EmitWakeupSignal();
    }
}

} // namespace Jobs2
