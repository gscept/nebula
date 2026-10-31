//------------------------------------------------------------------------------
// visibilitytest.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/refcounted.h"
#include "timing/timer.h"
#include "io/console.h"
#include "jobs2test.h"
#include "app/application.h"
#include "io/ioserver.h"
#include "util/hashtable.h"
#include "core/sysfunc.h"
#include "system/systeminfo.h"

#include "jobs2/jobs2.h"

using namespace Timing;
using namespace Jobs2;
namespace Test
{

class EventManual : public Threading::Event
{
public:
    EventManual() : Threading::Event(true) {};
};

/// counts live copies, used to check that job lambda captures are destroyed
struct CaptureTracker
{
    static Threading::Interlocked::AtomicCounter alive;
    CaptureTracker() { alive.Increment(); }
    CaptureTracker(const CaptureTracker&) { alive.Increment(); }
    CaptureTracker(CaptureTracker&&) { alive.Increment(); }
    ~CaptureTracker() { alive.Decrement(); }
};
Threading::Interlocked::AtomicCounter CaptureTracker::alive;


__ImplementClass(Jobs2Test, 'RET2', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
void
Jobs2Test::Run()
{
    // create a job port
    JobSystemInitInfo portInfo;
    portInfo.name = "TestJob2System";
    portInfo.numThreads = System::NumCpuCores;
    portInfo.priority = UINT_MAX;
    JobSystemInit(portInfo);

    Threading::Interlocked::AtomicCounter waitCounters[3] = { 1,1,1 };
    EventManual finishedEvent[3];
    Threading::Event hostEvent;
    struct Context
    {
        Math::vec4* inout;
        Math::vec4* input2;
    } ctx;

    // Setup thread context
    const uint NumInputs = 100000;
    ctx.inout = new Math::vec4[NumInputs];
    ctx.input2 =new Math::vec4[NumInputs];

    for (uint i = 0; i < NumInputs; i++)
    {
        ctx.inout[i] = Math::vec4(1, 2, 3, 4);
        ctx.input2[i] = Math::vec4(5, 6, 7, 8);
    }

    auto fun = [](SizeT totalJobs, SizeT groupSize, IndexT invocationIndex, SizeT invocationOffset, void* ctx)
    {
        Context* context = static_cast<Context*>(ctx);
        
        for (IndexT i = 0; i < groupSize; i++)
        {
            IndexT index = i + invocationOffset;
            if (index >= totalJobs)
                break;
            context->inout[index] = Math::cross3(context->inout[index], context->input2[index]);
        }
    };
    
    // Run functions in lockstep
    JobDispatch(fun, NumInputs, 1024, ctx, nullptr, &waitCounters[0]);
    JobDispatch(fun, NumInputs, 1024, ctx, { &waitCounters[0] }, &waitCounters[1]);
    JobDispatch(fun, NumInputs, 1024, ctx, { &waitCounters[1] }, &waitCounters[2]);
    JobDispatch(fun, NumInputs, 1024, ctx, { &waitCounters[2] }, nullptr, & hostEvent); // Last dispatch triggers a waitable event

    // Wait for jobs to finish
    bool didFinish = hostEvent.WaitTimeout(10000000);
    VERIFY(didFinish);

    bool result = true;
    for (uint i = 0; i < NumInputs; i++)
    {
        // Result is calculated using https://crossproductcalculator.org/
        result &= (ctx.inout[i] == Math::vec4(-8800, -880, 7040, 0));
    }
    VERIFY(result);

    delete[] ctx.inout;
    delete[] ctx.input2;

    // Lambda captures used to be leaked, the callable living in the scratch
    // buffer was never destroyed
    {
        CaptureTracker::alive = 0;
        Threading::Event lambdaDone;
        {
            CaptureTracker tracker;
            JobDispatch([tracker](SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset)
            {
            }, 64, 8, nullptr, nullptr, &lambdaDone);
            VERIFY(lambdaDone.WaitTimeout(10000));
            VERIFY(CaptureTracker::alive == 1);
        }
        VERIFY(CaptureTracker::alive == 0);
    }

    // JobAlloc used to bump the scratch iterator without synchronization,
    // handing out overlapping memory when called from several threads
    {
        const SizeT NumAllocs = 8192;
        void** allocations = new void*[NumAllocs];
        Threading::Event allocDone;
        JobDispatch([allocations](SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset)
        {
            for (IndexT i = 0; i < groupSize; i++)
            {
                IndexT index = i + invocationOffset;
                if (index >= totalJobs)
                    return;
                allocations[index] = JobAlloc(16);
            }
        }, NumAllocs, 16, nullptr, nullptr, &allocDone);
        VERIFY(allocDone.WaitTimeout(10000));

        Util::Array<void*> sorted(allocations, NumAllocs);
        sorted.Sort();
        bool allUnique = sorted[0] != nullptr;
        for (IndexT i = 1; i < NumAllocs; i++)
            allUnique &= sorted[i] != sorted[i - 1];
        VERIFY(allUnique);
        delete[] allocations;
    }
}

} // namespace Test