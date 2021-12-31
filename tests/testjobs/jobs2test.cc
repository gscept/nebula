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


__ImplementClass(Jobs2Test, 'RET2', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
void
Jobs2Test::Run()
{
    auto systemInfo = Core::SysFunc::GetSystemInfo();

    // create a job port
    JobSystemInitInfo portInfo;
    portInfo.name = "TestJob2System";
    portInfo.numThreads = systemInfo->GetNumCpuCores();
    portInfo.priority = UINT_MAX;
    JobSystemInit(portInfo);

    EventManual finishedEvent[3];
    Threading::Event hostEvent;
    struct Context
    {
        Math::vec4* input1;
        Math::vec4* input2;
        Math::vec4* output;
    } ctx;

    // Setup thread context
    const uint NumInputs = 100000;
    ctx.input1 = n_new_array(Math::vec4, NumInputs);
    ctx.input2 = n_new_array(Math::vec4, NumInputs);
    ctx.output = n_new_array(Math::vec4, NumInputs);

    for (uint i = 0; i < NumInputs; i++)
    {
        ctx.input1[i] = Math::vec4(1, 2, 3, 4);
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
            context->output[index] = Math::cross3(context->input1[index], context->input2[index]);
        }
    };
    
    // Run function
    JobDispatch(fun, NumInputs, 1024, &ctx, nullptr, &finishedEvent[0]);
    JobDispatch(fun, NumInputs, 1024, &ctx, &finishedEvent[0], &finishedEvent[1]);
    JobDispatch(fun, NumInputs, 1024, &ctx, &finishedEvent[1], &finishedEvent[2]);
    JobDispatch(fun, NumInputs, 1024, &ctx, &finishedEvent[2], &hostEvent);

    // Wait for jobs to finish
    bool didFinish = hostEvent.WaitTimeout(10000000);
    VERIFY(didFinish);

    bool result = true;
    for (uint i = 0; i < NumInputs; i++)
    {
        result &= (ctx.output[i] == Math::vec4(-4, 8, -4, 0));
    }
    VERIFY(result);

    delete[] ctx.input1;
    delete[] ctx.input2;
    delete[] ctx.output;
}

} // namespace Test