#pragma once
//------------------------------------------------------------------------------
/**
	Job system allows for scheduling and execution of a parallel task.

	The job system works as follows. First one creates a job port with a name
	and some threads. The threads will be named as the port but with their index appended.

	The job port has a priority, meaning all jobs sent to this port will inherit said priority.
	The job ports are then updated every frame to schedule work packages.

	A job is not single-threaded, but spreads its work by chunking its slices and scheduling
	an equal amount of work on several threads. 

	How to setup a job:
		

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idallocator.h"
#include "threading/thread.h"
#include "threading/event.h"
#include "threading/safequeue.h"
#include "util/stringatom.h"
#include "util/queue.h"
#include <atomic>
namespace Jobs
{

#define JOBFUNCCONTEXT_MAXINPUTS 8
#define JOBFUNCCONTEXT_MAXUNIFORMS 4
#define JOBFUNCCONTEXT_MAXOUTPUTS 8

//------------------------------------------------------------------------------
/**
	This type explains the data used by a single thread execution of a job
*/
struct JobFuncContext
{
	ubyte* scratch;

	uint numUniforms;								// number of used slots (max 4)
	ubyte* uniforms[JOBFUNCCONTEXT_MAXUNIFORMS];
	uint uniformSizes[JOBFUNCCONTEXT_MAXUNIFORMS];

	uint numInputs;									// number of used slots (max 4)
	ubyte* inputs[JOBFUNCCONTEXT_MAXINPUTS];
	uint inputSizes[JOBFUNCCONTEXT_MAXINPUTS];

	uint numOutputs;								// number of used slots (max 4)
	ubyte* outputs[JOBFUNCCONTEXT_MAXOUTPUTS];
	uint outputSizes[JOBFUNCCONTEXT_MAXOUTPUTS];
};

struct JobIOData
{
	static const SizeT MaxNumBuffers = 8;

	SizeT numBuffers;
	void* data[MaxNumBuffers];			// pointers to data
	SizeT dataSize[MaxNumBuffers];		// size of entire array
	SizeT sliceSize[MaxNumBuffers];		// size of singular work package
};

struct JobUniformData
{
	static const SizeT MaxNumBuffers = 4;

	SizeT numBuffers;
	void* data[MaxNumBuffers];			// pointers to data
	SizeT dataSize[MaxNumBuffers];		// size of entire array
	SizeT scratchSize;					// scratch memory
};

struct JobContext
{
	JobIOData input;
	JobIOData output;
	JobUniformData uniform;
};

struct CreateJobInfo
{
	void(*JobFunc)(const JobFuncContext& ctx);
};

class JobThread : public Threading::Thread
{
	__DeclareClass(JobThread);
public:

	enum JobThreadCommandType
	{
		RunJob,
		Wait
	};

	struct JobThreadCommand
	{
		JobThreadCommandType ev;
		union
		{
			struct // execute job
			{
				uint slice;
				uint numSlices;
				uint stride;
				const JobContext* context;
				void(*JobFunc)(const JobFuncContext& ctx);
				std::atomic_uint* completionCounter;
				Threading::Event* completionEvent;
				const std::function<void()>* callback;
			} run;

			struct // synchronize
			{
				Threading::Event* ev;
			} sync;
		};
	};

	/// constructor
	JobThread();
	/// destructor
	virtual ~JobThread();

	/// called if thread needs a wakeup call before stopping
	void EmitWakeupSignal();
	/// this method runs in the thread context
	void DoWork();

	/// push a set of job slices
	void PushJobSlices(uint sliceIndex, uint numSlices, uint stride, const JobContext* ctx, void(*JobFunc)(const JobFuncContext& ctx), std::atomic_uint* completionCounter, Threading::Event* completionEvent, const std::function<void()>* callback);
	/// push command buffer work
	void PushCommand(const JobThreadCommand& command);
	/// push command buffer work
	void PushCommands(const Util::Array<JobThreadCommand>& commands);

private:

	static const SizeT MaxScratchSize = (64 * 1024);    // 64 kB max scratch size

	bool busy;
	Threading::SafeQueue<JobThreadCommand> commands;
	ubyte* scratchBuffer;
};


ID_32_TYPE(JobId);
ID_16_TYPE(JobPortId);
ID_32_16_16_TYPE(JobScheduleId); // combined JobId, JobPortId and execution index

struct CreateJobPortInfo
{
	Util::StringAtom name;
	SizeT numThreads;
	uint priority;
};

/// create a new job port
JobPortId CreateJobPort(const CreateJobPortInfo& info);
/// destroy job port
void DestroyJobPort(const JobPortId& id);

typedef Ids::IdAllocator<
	Util::StringAtom,						// 0 - name
	Util::FixedArray<Ptr<JobThread>>,		// 1 - threads
	uint									// 2 - next thread index
> JobPortAllocator;
extern JobPortAllocator jobPortAllocator;

/// create job
JobId CreateJob(const CreateJobInfo& info);
/// delete job
void DestroyJob(const JobId& id);

/// schedule job to be executed
void JobSchedule(const JobId& job, const JobPortId& port, const JobContext& ctx);
/// schedule job with callback when finished
void JobSchedule(const JobId& job, const JobPortId& port, const JobContext& ctx, const std::function<void()>& callback);
/// wait for the job
void JobWait(const JobId& job);

typedef Ids::IdAllocator<
	CreateJobInfo,				// 0 - job info
	std::function<void()>,		// 1 - callback
	Threading::Event*,			// 2 - event to trigger when job is done
	std::atomic_uint*			// 3 - completion counter
> JobAllocator;
extern JobAllocator jobAllocator;


} // namespace Jobs
