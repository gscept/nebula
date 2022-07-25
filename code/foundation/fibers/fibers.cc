//------------------------------------------------------------------------------
//  jobs.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "system/cpu.h"
#include "profiling/profiling.h"
#include "fibers.h"
namespace Fibers
{

thread_local FiberThread* currentThread;

__ImplementClass(Fibers::FiberThread, 'FITH', Threading::Thread);
//------------------------------------------------------------------------------
/**
*/
FiberThread::FiberThread()
    : currentFiber{ nullptr, nullptr }
{
}

//------------------------------------------------------------------------------
/**
*/
FiberThread::~FiberThread()
{

}

//------------------------------------------------------------------------------
/**
*/
void
FiberThread::EmitWakeupSignal()
{
}

//------------------------------------------------------------------------------
/**
*/
void
FiberThread::DoWork()
{
#if NEBULA_ENABLE_PROFILING
    Profiling::ProfilingRegisterThread();
#endif

    // convert thread to fiber
    Fibers::Fiber::ThreadToFiber(this->threadFiber);

    currentThread = this;
    while (!this->ThreadStopRequested())
    {
        NewFiber();
    }

    // convert back to thread and destroy fiber
    Fibers::Fiber::FiberToThread(this->threadFiber);
}

//------------------------------------------------------------------------------
/**
*/
bool
FiberThread::HasWork()
{
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
FiberThread::SleepFiber(Threading::AtomicCounter* counter, int value)
{
    FiberQueue::Sleep(FiberWaitContext{ this->currentFiber.fiber, counter, value });

    // go back to main loop
    this->SwitchFiber();
}

//------------------------------------------------------------------------------
/**
*/
void
FiberThread::NewFiber()
{
    // wakeup waiting fibers
    if (!FiberQueue::WakeupFiber(this->currentFiber))
    {
        // if not, try dequeue pending fiber
        if (!FiberQueue::Dequeue(this->currentFiber))
        {
            // if all fails, return
            return;
        }
    }

    // set the current fiber, start it and when finished, decrement counter
    this->currentFiber.fiber->SwitchToFiber(this->threadFiber);
}

//------------------------------------------------------------------------------
/**
*/
void
FiberThread::SwitchFiber()
{
    this->threadFiber.SwitchToFiber(*this->currentFiber.fiber);
}

Threading::LockFreeQueue<FiberQueue::Job> FiberQueue::PendingJobsQueue;
Threading::LockFreeQueue<uint> FiberQueue::FiberIdQueue;
Util::FixedArray<Fibers::Fiber> FiberQueue::Fibers;
Util::FixedArray<FiberQueue::Job> FiberQueue::FiberContexts;

Threading::CriticalSection FiberQueue::SleepLock;
Util::Array<FiberWaitContext> FiberQueue::SleepingFibers;
Util::FixedArray<Ptr<FiberThread>> FiberQueue::Threads;

//------------------------------------------------------------------------------
/**
*/
FiberQueue::FiberQueue()
{
}

//------------------------------------------------------------------------------
/**
*/
FiberQueue::~FiberQueue()
{
}

//------------------------------------------------------------------------------
/**
*/
void
FiberQueue::Setup(const FiberQueueCreateInfo& info)
{
    FiberQueue::Threads.Resize(info.numThreads);
    for (uint i = 0; i < info.numThreads; i++)
    {
        FiberQueue::Threads[i] = Fibers::FiberThread::Create();
        FiberQueue::Threads[i]->SetName(Util::String::Sprintf("%s #%d", "Fiber Thread", i));
        FiberQueue::Threads[i]->SetThreadAffinity(System::Cpu::Core0 << i);
        FiberQueue::Threads[i]->Start();
    }
    FiberQueue::Fibers.Resize(info.numFibers);
    FiberQueue::FiberContexts.Resize(info.numFibers);
    FiberQueue::PendingJobsQueue.Resize(65535);
    FiberQueue::FiberIdQueue.Resize(info.numFibers);
    for (int i = info.numFibers - 1; i >= 0; i--)
        FiberQueue::FiberIdQueue.Enqueue(i);
}

//------------------------------------------------------------------------------
/**
*/
void
FiberQueue::Discard()
{
    for (Ptr<FiberThread>& thread : FiberQueue::Threads)
    {
        thread->Stop();
    }
    FiberQueue::Threads.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
FiberQueue::Free(uint id)
{
    FiberQueue::FiberIdQueue.Enqueue(id);
}

//------------------------------------------------------------------------------
/**
*/
void
FiberQueue::Sleep(FiberWaitContext context)
{
    Threading::CriticalScope lock(&FiberQueue::SleepLock);
    FiberQueue::SleepingFibers.Append(context);
}

//------------------------------------------------------------------------------
/**
*/
bool
FiberQueue::WakeupFiber(Fibers::FiberContext& fiber)
{
    // go through fibers and check their wait condition
    Threading::CriticalScope lock(&FiberQueue::SleepLock);
    for (IndexT i = 0; i < FiberQueue::SleepingFibers.Size(); i++)
    {
        const FiberWaitContext& waitFiber = FiberQueue::SleepingFibers[i];

        // fiber has it's wait condition satisfied, return it
        if (*waitFiber.counter == waitFiber.value)
        {
            fiber.fiber = waitFiber.fiber;
            fiber.counter = waitFiber.counter;
            FiberQueue::SleepingFibers.EraseIndex(i);
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
FiberFunction(void* param)
{
    // get the job information
    FiberQueue::Job* job = (FiberQueue::Job*)param;

    // run the job function with the job context
    job->function(job->context);

    // decrement the counter
    Threading::Interlocked::Decrement(job->counter);
    n_assert(*job->counter >= 0);

    // free up context
    FiberQueue::Free(job->id);

    // continue thread work
    currentThread->SwitchFiber();
}

//------------------------------------------------------------------------------
/**
*/
bool
FiberQueue::Dequeue(Fibers::FiberContext& fiber)
{
    Job job;
    uint id;

    // if there are free jobs to pick up, do it
    if (FiberQueue::PendingJobsQueue.Dequeue(job))
    {
        if (FiberQueue::FiberIdQueue.Dequeue(id))
        {
            job.id = id;

            // update context and run the constructor on the fiber
            FiberQueue::FiberContexts[id] = job;
            new (&FiberQueue::Fibers[id]) Fibers::Fiber{ Fibers::FiberFunction, &FiberQueue::FiberContexts[id] };

            fiber.counter = job.counter;
            fiber.fiber = &FiberQueue::Fibers[id];
            return true;
        }
        else
        {
            // if fiber pool fails, enqueue job again
            FiberQueue::PendingJobsQueue.Enqueue(job);
        }
    }

    return false;
}

} // namespace Fibers
