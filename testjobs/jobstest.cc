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
#include "input/inputserver.h"
#include "input/keyboard.h"
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
	App::Application app;

	Ptr<IO::IoServer> ioServer = IO::IoServer::Create();

	app.SetAppTitle("Jobs test!");
	app.SetCompanyName("gscept");
	app.Open();

	// create a job port
	CreateJobPortInfo portInfo;
	portInfo.name = "TestJobPort";
	portInfo.numThreads = 2;
	portInfo.priority = FLT_MAX;
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
		Math::float4* input1 = (Math::float4*)ctx.inputs[0];
		Math::float4* input2 = (Math::float4*)ctx.inputs[1];
		Math::float4* output = (Math::float4*)ctx.outputs[0];

		output[0] = Math::float4::cross3(input1[0], input2[0]);
	};
	JobId job = CreateJob(jobInfo);

	const uint NumInputs = 1000000;
	Math::float4* inputs1 = n_new_array(Math::float4, NumInputs);
	Math::float4* inputs2 = n_new_array(Math::float4, NumInputs);
	Math::float4* outputs = n_new_array(Math::float4, NumInputs);

	for (uint i = 0; i < NumInputs; i++)
	{
		inputs1[i] = Math::float4(1, 2, 3, 4);
		inputs2[i] = Math::float4(5, 6, 7, 8);
	}

	// schedule job to be executed, distributing the workload on 8 threads
	JobContext ctx;
	ctx.input.numBuffers = 2;									// number of input buffers
	ctx.input.sliceSize[0] = sizeof(Math::float4);				// size of data for a single job function call
	ctx.input.data[0] = inputs1;								// pointer to buffer of data
	ctx.input.dataSize[0] = sizeof(Math::float4) * NumInputs;	// size of entire data

	ctx.input.sliceSize[1] = sizeof(Math::float4);				// size of data for a single job function call
	ctx.input.data[1] = inputs2;								// pointer to buffer of data
	ctx.input.dataSize[1] = sizeof(Math::float4) * NumInputs;	// size of entire data

	ctx.output.numBuffers = 1;
	ctx.output.sliceSize[0] = sizeof(Math::float4);				// size of data for a single job function call
	ctx.output.data[0] = outputs;								// pointer to buffer of data
	ctx.output.dataSize[0] = sizeof(Math::float4) * NumInputs;	// size of entire data

	ctx.uniform.numBuffers = 0;									// use 0 uniform data

	// run job
	JobSchedule(job, port, ctx);

    JobSyncSignal(jobSync, port);

	// wait for job to finish
    JobSyncHostWait(jobSync);

    bool result = true;
	for (uint i = 0; i < NumInputs; i++)
	{
	    result &= (outputs[i] == Math::float4(-4, 8, -4, 0));
	}
    VERIFY(result);
    DestroyJob(job);
    DestroyJobPort(port);
    DestroyJobSync(jobSync);

	app.Close();
}

} // namespace Test