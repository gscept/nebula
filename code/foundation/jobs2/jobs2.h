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
using JobFunc = void(*)(SizeT totalJobs, SizeT groupSize, IndexT invocationIndex, void* ctx);

class JobThread : public Threading::Thread
{
    __DeclareClass(JobThread);
public:

    /// constructor
    JobThread();
    /// destructor
    virtual ~JobThread();

protected:
    friend void JobPortDispatch(const JobFunc& func, const SizeT numInvocations, const SizeT groupSize, void* context, Threading::Event* waitEvent, Threading::Event* signalEvent);

    /// override this method if your thread loop needs a wakeup call before stopping
    virtual void EmitWakeupSignal() override;
    /// this method runs in the thread context
    virtual void DoWork() override;

private:
    Threading::Event wakeupEvent;
};

struct CreateJobSystemInfo
{
    Util::StringAtom name;
    SizeT numThreads;
    uint affinity;
    uint priority;
};

/// Create a new job port
void CreateJobSystem(const CreateJobSystemInfo& info);
/// Destroy job port
void DestroyJobSystem();

/// Dispatch job
void JobPortDispatch(const JobFunc& func, const SizeT numInvocations, const SizeT groupSize, void* ctx, Threading::Event* waitEvent = nullptr, Threading::Event* signalEvent = nullptr);

} // namespace Jobs2
