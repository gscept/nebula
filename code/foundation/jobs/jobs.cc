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
JobSyncAllocator jobSyncAllocator(0xFFFFFFFF);
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
		thread->SetThreadAffinity(info.affinity);
		threads[i] = thread;
	}
	jobPortAllocator.Get<PortThreads>(port) = threads;
	jobPortAllocator.Get<PortNextThreadIndex>(port) = 0;

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
	Util::FixedArray<Ptr<JobThread>>& threads = jobPortAllocator.Get<PortThreads>((Ids::Id32)id.id);
	for (IndexT i = 0; i < threads.Size(); i++)
	{
		threads[i]->Stop();
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

	// ugh, so ugly, would rather have these in the allocator, but atomic_uint is not copyable, and events don't implement copy constructors or moves yet
	jobAllocator.Get<JobScratchMemory>(job) = { Memory::HeapType::ScratchHeap, 0, nullptr };

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
	PrivateMemory& mem = jobAllocator.Get<JobScratchMemory>(id.id);
	if (mem.memory != nullptr)
		Memory::Free(mem.heapType, mem.memory);
	jobAllocator.DeallocObject(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
JobSchedule(const JobId& job, const JobPortId& port, const JobContext& ctx, const bool cycleThreads)
{
	JobSchedule(job, port, ctx, nullptr, cycleThreads);
}

//------------------------------------------------------------------------------
/**
*/
void
JobSchedule(const JobId& job, const JobPortId& port, const JobContext& ctx, const std::function<void()>& callback, const bool cycleThreads)
{
	n_assert(ctx.input.numBuffers > 0);
	n_assert(ctx.output.numBuffers > 0);

	// set this job to be the last pushed one
	jobPortAllocator.Get<PortLastJobId>((Ids::Id32)port.id) = job;

	// port related stuff
	Util::FixedArray<Ptr<JobThread>>& threads = jobPortAllocator.Get<PortThreads>((Ids::Id32)port.id);
	uint& threadIndex = jobPortAllocator.Get<PortNextThreadIndex>((Ids::Id32)port.id);

	// job related stuff
	const CreateJobInfo& info = jobAllocator.Get<0>(job.id);	

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
			cmd.run.callback = callback ? &callback : nullptr;
			threads[threadIndex]->PushCommand(cmd);

			// if we cycle threads (default) put work slices on different threads
			if (cycleThreads)
				threadIndex = (threadIndex + 1) % threads.Size();
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
JobScheduleSequence(const Util::Array<JobId>& jobs, const JobPortId& port, const Util::Array<JobContext>& contexts)
{
	// push jobs to same thread
	IndexT i;
	for (i = 0; i < jobs.Size(); i++)
	{
		n_assert(contexts[i].input.numBuffers > 0);
		n_assert(contexts[i].output.numBuffers > 0);
		JobSchedule(jobs[i], port, contexts[i], false);
	}

	// cycle thread index after all jobs are pushed, this guarantees the sequence will run in order
	uint& threadIndex = jobPortAllocator.Get<PortNextThreadIndex>((Ids::Id32)port.id);
	const SizeT numThreads = jobPortAllocator.Get<PortThreads>((Ids::Id32)port.id).Size();
	threadIndex = (threadIndex + 1) % numThreads;
}

//------------------------------------------------------------------------------
/**
*/
void 
JobScheduleSequence(const Util::Array<JobId>& jobs, const JobPortId & port, const Util::Array<JobContext>& contexts, const std::function<void()>& callback)
{
	// push jobs to same thread
	IndexT i;
	for (i = 0; i < jobs.Size(); i++)
	{
		n_assert(contexts[i].input.numBuffers > 0);
		n_assert(contexts[i].output.numBuffers > 0);
		JobSchedule(jobs[i], port, contexts[i], callback, false);
	}
	
	// cycle thread index after all jobs are pushed
	uint& threadIndex = jobPortAllocator.Get<PortNextThreadIndex>((Ids::Id32)port.id);
	const SizeT numThreads = jobPortAllocator.Get<PortThreads>((Ids::Id32)port.id).Size();
	threadIndex = (threadIndex + 1) % numThreads;
}

//------------------------------------------------------------------------------
/**
*/
void*
JobAllocateScratchMemory(const JobId& job, const Memory::HeapType heap, const SizeT size)
{
	// setup scratch memory
	n_assert(jobAllocator.Get<JobScratchMemory>(job.id).memory == nullptr);
	void* ret = Memory::Alloc(heap, size);
	jobAllocator.Get<JobScratchMemory>(job.id) = { heap, size, ret };

	// return pointer in case we want to fill it
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
JobSyncId 
CreateJobSync(const CreateJobSyncInfo& info)
{
	Ids::Id32 id = jobSyncAllocator.AllocObject();
	jobSyncAllocator.Get<SyncCallback>(id) = info.callback;
	jobSyncAllocator.Get<SyncCompletionEvent>(id) = n_new(Threading::Event(true));
	jobSyncAllocator.Get<SyncCompletionCounter>(id) = n_new(std::atomic_uint);
	jobSyncAllocator.Get<SyncPendingSignal>(id) = false;

	// start with it signaled
	jobSyncAllocator.Get<SyncCompletionEvent>(id)->Signal();
	JobSyncId ret;
	ret.id = id;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
DestroyJobSync(const JobSyncId id)
{
	n_delete(jobSyncAllocator.Get<SyncCompletionEvent>(id.id));
	n_delete(jobSyncAllocator.Get<SyncCompletionCounter>(id.id));
	jobSyncAllocator.DeallocObject(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void 
JobSyncSignal(const JobSyncId id, const JobPortId port)
{
	Util::FixedArray<Ptr<JobThread>>& threads = jobPortAllocator.Get<PortThreads>((Ids::Id32)port.id);
	Threading::Event* event = jobSyncAllocator.Get<SyncCompletionEvent>(id.id);
	std::atomic_uint* counter = jobSyncAllocator.Get<SyncCompletionCounter>(id.id);
	const std::function<void()>& callback = jobSyncAllocator.Get<SyncCallback>(id.id);
	jobSyncAllocator.Get<SyncPendingSignal>(id.id) = true;
	uint completionCount = threads.Size();

	// set counter and issue sync points if the jobs haven't finished yet
	if (completionCount > 0)
	{
		// set the counter to finish all threads, and reset the event
		counter->exchange(completionCount);
		event->Reset();

		IndexT i;
		for (i = 0; i < threads.Size(); i++)
		{
			// setup job
			auto thread = threads[i];
			JobThread::JobThreadCommand cmd;
			cmd.ev = JobThread::Signal;
			cmd.sync.completionCounter = counter;
			cmd.sync.ev = event;
			cmd.sync.callback = callback ? &callback : nullptr;
			thread->PushCommand(cmd);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
JobSyncHostWait(const JobSyncId id)
{
	Threading::Event* event = jobSyncAllocator.Get<SyncCompletionEvent>(id.id);
	event->Wait();
}

//------------------------------------------------------------------------------
/**
*/
void
JobSyncThreadWait(const JobSyncId id, const JobPortId port)
{
	Util::FixedArray<Ptr<JobThread>>& threads = jobPortAllocator.Get<PortThreads>((Ids::Id32)port.id);
	Threading::Event* event = jobSyncAllocator.Get<SyncCompletionEvent>(id.id);
	
	IndexT i;
	for (i = 0; i < threads.Size(); i++)
	{
		// setup job
		auto thread = threads[i];
		JobThread::JobThreadCommand cmd;
		cmd.ev = JobThread::Wait;
		cmd.sync.completionCounter = nullptr;
		cmd.sync.ev = event;
		thread->PushCommand(cmd);
	}
}

//------------------------------------------------------------------------------
/**
*/
bool 
JobSyncSignaled(const JobSyncId id)
{
	Threading::Event* event = jobSyncAllocator.Get<SyncCompletionEvent>(id.id);
	return event->Peek();
}

__ImplementClass(Jobs::JobThread, 'JBTH', Threading::Thread);
//------------------------------------------------------------------------------
/**
*/
JobThread::JobThread() :
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
	curCommands.Reserve(500);
	while (!this->ThreadStopRequested())
	{
		// dequeue all commands, this ensures we don't gain any new commands this thread loop
		this->commands.DequeueAll(curCommands);

		IndexT i;
		for (i = 0; i < curCommands.Size(); i++)
		{
			JobThreadCommand& cmd = curCommands[i];

			switch (cmd.ev)
			{
			case RunJob:
				this->PushJobSlices(cmd.run.slice, cmd.run.numSlices, cmd.run.stride, cmd.run.context, cmd.run.JobFunc, cmd.run.callback);
				break;
			case Signal:
			{
				// subtract 1 from completion counter
				uint prev = cmd.sync.completionCounter->fetch_sub(1);
				if (prev == 1)
				{
					cmd.sync.ev->Signal();
					if (cmd.sync.callback)
						(*cmd.sync.callback)();
				}
				break;
			}
			case Wait:
				cmd.sync.ev->Wait();
				break;
			}
		}

		// reset commands, but don't destroy them
		this->commands.Wait();
	}

	// free scratch buffer
	Memory::Free(Memory::ScratchHeap, this->scratchBuffer);
	this->scratchBuffer = 0;
}

//------------------------------------------------------------------------------
/**
*/
bool 
JobThread::HasWork()
{
	return this->commands.Size() > 0;
}

//------------------------------------------------------------------------------
/**
*/
void
JobThread::PushJobSlices(uint firstSliceIndex, uint numSlices, uint stride, const JobContext ctx, void(*JobFunc)(const JobFuncContext& ctx), const std::function<void()>* callback)
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
