//------------------------------------------------------------------------------
//  jobs.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "jobs.h"
namespace Jobs
{

JobPortAllocator jobPortAllocator(0xFFFF);
JobAllocator jobAllocator(0xFFFFFFFF);
//------------------------------------------------------------------------------
/**
*/
JobPortId
CreateJobPort(const CreateJobPortInfo& info)
{
	Ids::Id32 port = jobPortAllocator.AllocObject();
	jobPortAllocator.Get<0>(port) = info.name;
	
	Util::FixedArray<Ptr<JobThread>> threads(info.numThreads);
	for (IndexT i = 0; i < info.numThreads; i++)
	{
		Ptr<JobThread> thread = JobThread::Create();
		thread->SetName(Util::String::Sprintf("%s%d", info.name.Value(), i));
		thread->Start();
		threads[i] = thread;
	}
	jobPortAllocator.Get<1>(port) = threads;
	jobPortAllocator.Get<2>(port) = 0;

	// we limit the id count to be ushort max
	JobPortId id;
	id.id = (Ids::Id16)port;
	return id;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyJobPort(const JobPortId& id)
{
	Util::FixedArray<Ptr<JobThread>>& threads = jobPortAllocator.Get<1>((Ids::Id32)id.id);
	for (IndexT i = 0; i < threads.Size(); i++)
	{
		threads[i]->Stop();
	}
}

//------------------------------------------------------------------------------
/**
*/
bool
JobPortBusy(const JobPortId& id)
{
	const JobId job = jobPortAllocator.Get<3>((Ids::Id32)id.id);
	if (job != JobId::Invalid())
	{
		const Threading::Event* ev = jobAllocator.Get<2>(job.id);
		return !ev->Peek();
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
void
JobPortSync(const JobPortId& id)
{
	const JobId job = jobPortAllocator.Get<3>((Ids::Id32)id.id);
	const Util::FixedArray<Ptr<JobThread>>& threads = jobPortAllocator.Get<1>((Ids::Id32)id.id);
	if (job != JobId::Invalid())
	{
		JobThread::JobThreadCommand cmd;
		cmd.ev = JobThread::Wait;
		cmd.sync.ev = jobAllocator.Get<2>(job.id);

		// push to all threads
		IndexT i;
		for (i = 0; i < threads.Size(); i++)
			threads[i]->PushCommand(cmd);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
JobPortWait(const JobPortId& id)
{
	const JobId job = jobPortAllocator.Get<3>((Ids::Id32)id.id);
	if (job != JobId::Invalid())
	{
		const Threading::Event* ev = jobAllocator.Get<2>(job.id);
		ev->Wait();
	}
}

//------------------------------------------------------------------------------
/**
*/
JobId
CreateJob(const CreateJobInfo& info)
{
	Ids::Id32 job = jobAllocator.AllocObject();
	jobAllocator.Get<0>(job) = info;
	jobAllocator.Get<2>(job) = n_new(Threading::Event(true));
	jobAllocator.Get<3>(job) = n_new(std::atomic_uint);

	JobId id;
	id.id = job;
	return id;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyJob(const JobId& id)
{
	// wait for job to finish before deleting
	jobAllocator.Get<2>(id.id)->Wait();
	n_assert(jobAllocator.Get<3>(id.id)->load() == 0);
	n_delete(jobAllocator.Get<3>(id.id));
	jobAllocator.DeallocObject(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
JobSchedule(const JobId& job, const JobPortId& port, const JobContext& ctx)
{
	JobSchedule(job, port, ctx, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
void
JobSchedule(const JobId& job, const JobPortId& port, const JobContext& ctx, const std::function<void()>& callback)
{
	n_assert(ctx.input.numBuffers > 0);
	n_assert(ctx.output.numBuffers > 0);

	// set this job to be the last pushed one
	jobPortAllocator.Get<3>((Ids::Id32)port.id) = job;

	// port related stuff
	Util::FixedArray<Ptr<JobThread>>& threads = jobPortAllocator.Get<1>((Ids::Id32)port.id);
	uint& threadIndex = jobPortAllocator.Get<2>((Ids::Id32)port.id);

	// job related stuff
	const CreateJobInfo& info = jobAllocator.Get<0>(job.id);	
	Threading::Event* completionEvent = jobAllocator.Get<2>(job.id);
	std::atomic_uint* completionCounter = jobAllocator.Get<3>(job.id);

	SizeT numInputSlices = (ctx.input.dataSize[0] + (ctx.input.sliceSize[0] - 1)) / ctx.input.sliceSize[0];
	SizeT numOutputSlices = (ctx.output.dataSize[0] + (ctx.output.sliceSize[0] - 1)) / ctx.output.sliceSize[0];
	n_assert(numInputSlices == numOutputSlices);

	IndexT i;
	uint* numWorkUnitSlices = (uint*)alloca(sizeof(uint) * threads.Size());
	for (i = 0; i < threads.Size(); i++)
	{
		numWorkUnitSlices[i] = numInputSlices / threads.Size();
	}

	SizeT remainder = numInputSlices % threads.Size();
	ushort stride = threads.Size();
	for (i = 0; i < remainder; i++)
	{
		numWorkUnitSlices[i] += 1;
	}

	// reset completion counter to number of slices and event to unset
	completionCounter->exchange(numInputSlices);
	completionEvent->Reset();

	for (IndexT i = 0; i < threads.Size(); i++)
	{
		// go through slices and send jobs to threads
		if (numWorkUnitSlices[i] > 0)
		{
			const Ptr<JobThread>& thread = threads[i];
			JobThread::JobThreadCommand cmd;
			cmd.ev = JobThread::RunJob;
			cmd.run.slice = i;
			cmd.run.numSlices = numWorkUnitSlices[i];
			cmd.run.stride = stride;
			cmd.run.context = ctx;
			cmd.run.JobFunc = info.JobFunc;
			cmd.run.completionCounter = completionCounter;
			cmd.run.completionEvent = completionEvent;
			cmd.run.callback = callback ? &callback : nullptr;
			threads[threadIndex]->PushCommand(cmd);
			threadIndex = (threadIndex + 1) % threads.Size();
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
JobWait(const JobId& job)
{
	Threading::Event* completionEvent = jobAllocator.Get<2>(job.id);
	completionEvent->Wait();
}

__ImplementClass(Jobs::JobThread, 'JBTH', Threading::Thread);
//------------------------------------------------------------------------------
/**
*/
JobThread::JobThread() :
	busy(false),
	scratchBuffer(nullptr)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
JobThread::~JobThread()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
JobThread::EmitWakeupSignal()
{
	this->commands.Signal();
}

//------------------------------------------------------------------------------
/**
*/
void
JobThread::DoWork()
{
	// allocate the scratch buffer
	n_assert(0 == this->scratchBuffer);
	this->scratchBuffer = (ubyte*)Memory::Alloc(Memory::ScratchHeap, MaxScratchSize);

	Util::Array<JobThreadCommand> curCommands;
	curCommands.Reserve(1000);
	while (!this->ThreadStopRequested())
	{
		// dequeue all commands, this ensures we don't gain any new commands this thread loop
		this->commands.DequeueAll(curCommands);

		this->busy = true;
		IndexT i;
		for (i = 0; i < curCommands.Size(); i++)
		{
			JobThreadCommand& cmd = curCommands[i];

			switch (cmd.ev)
			{
			case RunJob:
				this->PushJobSlices(cmd.run.slice, cmd.run.numSlices, cmd.run.stride, cmd.run.context, cmd.run.JobFunc, cmd.run.completionCounter, cmd.run.completionEvent, cmd.run.callback);
				break;
			case Wait:
				cmd.sync.ev->Wait();
				break;
			}
		}

		// reset commands, but don't destroy them
		curCommands.Reset();
		this->busy = false;
		this->commands.Wait();
	}

	// free scratch buffer
	Memory::Free(Memory::ScratchHeap, this->scratchBuffer);
	this->scratchBuffer = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
JobThread::PushJobSlices(uint firstSliceIndex, uint numSlices, uint stride, const JobContext ctx, void(*JobFunc)(const JobFuncContext& ctx), std::atomic_uint* completionCounter, Threading::Event* completionEvent, const std::function<void()>* callback)
{
	uint sliceIndex = firstSliceIndex;

	uint i;
	for (i = 0; i < numSlices; i++)
	{
		// start setting up the job slice function context
		JobFuncContext tctx = { 0 };

		// setup uniforms which are the same for all threads
		uint bufIdx;
		tctx.numUniforms = ctx.uniform.numBuffers;
		for (bufIdx = 0; bufIdx < tctx.numUniforms; bufIdx++)
		{
			tctx.uniforms[bufIdx] = (ubyte*)ctx.uniform.data[bufIdx];
			tctx.uniformSizes[bufIdx] = ctx.uniform.dataSize[bufIdx];
		}

		if (ctx.uniform.scratchSize > 0)
		{
			n_assert(ctx.uniform.scratchSize < JobThread::MaxScratchSize);
			tctx.scratch = this->scratchBuffer;
		}
		else
		{
			tctx.scratch = nullptr;
		}

		// setup inputs
		tctx.numInputs = ctx.input.numBuffers;
		for (bufIdx = 0; bufIdx < tctx.numInputs; bufIdx++)
		{
			ubyte* buf = (ubyte*)ctx.input.data[bufIdx];
			n_assert(buf != nullptr);
			const IndexT offset = sliceIndex * ctx.input.sliceSize[bufIdx];
			tctx.inputs[bufIdx] = buf + offset;
			tctx.inputSizes[bufIdx] = Math::n_min(ctx.input.sliceSize[bufIdx], ctx.input.dataSize[bufIdx] - offset);
		}

		// setup outputs
		tctx.numOutputs = ctx.output.numBuffers;
		for (bufIdx = 0; bufIdx < tctx.numOutputs; bufIdx++)
		{
			ubyte* buf = (ubyte*)ctx.output.data[bufIdx];
			n_assert(buf != nullptr);
			const IndexT offset = sliceIndex * ctx.output.sliceSize[bufIdx];
			tctx.outputs[bufIdx] = buf + offset;
			tctx.outputSizes[bufIdx] = Math::n_min(ctx.output.sliceSize[bufIdx], ctx.output.dataSize[bufIdx] - offset);
		}

		// run job
		JobFunc(tctx);

		// offset slice index by calculated stride
		sliceIndex += stride;
	}

	// add number of slices to completion counter
	uint prev = completionCounter->fetch_sub(numSlices);
	prev -= numSlices;
	n_assert(prev >= 0);
	if (prev == 0)
	{
		// signal event and run callback (if present) once count hits 0
		completionEvent->Signal();
		if (callback)
			(*callback)();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
JobThread::PushCommand(const JobThreadCommand& command)
{
	this->commands.Enqueue(command);
}

//------------------------------------------------------------------------------
/**
*/
void
JobThread::PushCommands(const Util::Array<JobThreadCommand>& commands)
{
	this->commands.EnqueueArray(commands);
}

} // namespace Jobs
