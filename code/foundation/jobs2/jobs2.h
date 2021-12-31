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
using JobFunc = void(*)(SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx);

class JobThread : public Threading::Thread
{
    __DeclareClass(JobThread);
public:

    /// constructor
    JobThread();
    /// destructor
    virtual ~JobThread();

protected:
    friend void JobDispatch(const JobFunc& func, const SizeT numInvocations, const SizeT groupSize, void* context, Threading::Event* waitEvent, Threading::Event* signalEvent);
    friend void JobDispatch(const JobFunc& func, const SizeT numInvocations, void* context, Threading::Event* waitEvent, Threading::Event* signalEvent);

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

/// Dispatch job
void JobDispatch(const JobFunc& func, const SizeT numInvocations, const SizeT groupSize, void* context, Threading::Event* waitEvent = nullptr, Threading::Event* signalEvent = nullptr);
/// Dispatch job as a single group (to be run on a single thread)
void JobDispatch(const JobFunc& func, const SizeT numInvocations, void* context, Threading::Event* waitEvent = nullptr, Threading::Event* signalEvent = nullptr);

} // namespace Jobs2
