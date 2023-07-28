//------------------------------------------------------------------------------
// visibilitytest.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/refcounted.h"
#include "timing/timer.h"
#include "io/console.h"
#include "jobstest.h"
#include "app/application.h"
#include "io/ioserver.h"

#include "jobs/jobs.h"

using namespace Timing;
using namespace Jobs;
namespace Test
{


__ImplementClass(JobsTest, 'RETE', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
void
JobsTest::Run()
{
    // create a job port
    CreateJobPortInfo portInfo;
    portInfo.name = "TestJobPort";
    portInfo.numThreads = 2;
    portInfo.priority = UINT_MAX;
    JobPortId port = CreateJobPort(portInfo);

    // create a job sync
    Jobs::CreateJobSyncInfo sinfo =
    {
        nullptr
    };
    JobSyncId jobSync = Jobs::CreateJobSync(sinfo);

    // create a job
    CreateJobInfo jobInfo;
    jobInfo.JobFunc = [](const JobFuncContext& ctx)
    {
        for (ptrdiff sliceIdx = 0; sliceIdx < ctx.numSlices; sliceIdx++)
        {
            Math::vec4* input1 = (Math::vec4*)N_JOB_INPUT(ctx, sliceIdx, 0);
            Math::vec4* input2 = (Math::vec4*)N_JOB_INPUT(ctx, sliceIdx, 1);
            Math::vec4* output = (Math::vec4*)N_JOB_OUTPUT(ctx, sliceIdx, 0);

            output[0] = Math::cross3(input1[0], input2[0]);
        }
    };
    JobId job = CreateJob(jobInfo);

    const uint NumInputs = 1000000;
    Math::vec4* inputs1 = new Math::vec4[NumInputs];
    Math::vec4* inputs2 = new Math::vec4[NumInputs];
    Math::vec4* outputs = new Math::vec4[NumInputs];

    for (uint i = 0; i < NumInputs; i++)
    {
        inputs1[i] = Math::vec4(1, 2, 3, 4);
        inputs2[i] = Math::vec4(5, 6, 7, 8);
    }

    // schedule job to be executed, distributing the workload on 8 threads
    JobContext ctx;
    ctx.input.numBuffers = 2;                                   // number of input buffers
    ctx.input.sliceSize[0] = sizeof(Math::vec4);                // size of data for a single job function call
    ctx.input.data[0] = inputs1;                                // pointer to buffer of data
    ctx.input.dataSize[0] = sizeof(Math::vec4) * NumInputs; // size of entire data

    ctx.input.sliceSize[1] = sizeof(Math::vec4);                // size of data for a single job function call
    ctx.input.data[1] = inputs2;                                // pointer to buffer of data
    ctx.input.dataSize[1] = sizeof(Math::vec4) * NumInputs; // size of entire data

    ctx.output.numBuffers = 1;
    ctx.output.sliceSize[0] = sizeof(Math::vec4);               // size of data for a single job function call
    ctx.output.data[0] = outputs;                               // pointer to buffer of data
    ctx.output.dataSize[0] = sizeof(Math::vec4) * NumInputs;    // size of entire data

    ctx.uniform.numBuffers = 0;                                 // use 0 uniform data

    // run job
    JobSchedule(job, port, ctx);

    JobSyncThreadSignal(jobSync, port);

    // wait for job to finish
    JobSyncHostWait(jobSync);

    bool result = true;
    for (uint i = 0; i < NumInputs; i++)
    {
        result &= (outputs[i] == Math::vec4(-4, 8, -4, 0));
    }
    VERIFY(result);
    DestroyJob(job);
    DestroyJobPort(port);
    DestroyJobSync(jobSync);
}

} // namespace Test