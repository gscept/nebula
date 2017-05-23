//------------------------------------------------------------------------------
//  serialjobport.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "jobs/serial/serialjobport.h"
#include "jobs/jobsystem.h"

namespace Jobs
{
__ImplementClass(Jobs::SerialJobPort, 'SRJP', Base::JobPortBase);

//------------------------------------------------------------------------------
/**
*/
SerialJobPort::SerialJobPort()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
SerialJobPort::~SerialJobPort()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This executes the job immediately in the current thread and returns
    afterwards.
*/
void
SerialJobPort::PushJob(const Ptr<Job>& job)
{
    const JobUniformDesc& uniformDesc = job->GetUniformDesc();
    const JobDataDesc& inputDesc = job->GetInputDesc();
    const JobDataDesc& outputDesc = job->GetOutputDesc();
    const JobFuncDesc& funcDesc = job->GetFuncDesc();

    // setup a job func context structure and call the job function for each slice

    // setup uniform data
    JobFuncContext ctx;
    if (uniformDesc.GetScratchSize() > 0)
    {
        n_assert(uniformDesc.GetScratchSize() < SerialJobSystem::MaxScratchSize);
        ctx.scratch = (ubyte*) JobSystem::Instance()->GetScratchBuffer();
    }
    else
    {
        ctx.scratch = 0;
    }
    ctx.numUniforms = uniformDesc.GetNumBuffers();
    IndexT bufIndex;
    for (bufIndex = 0; bufIndex < uniformDesc.GetNumBuffers(); ++bufIndex)
    {
        ubyte* buf = (ubyte*) uniformDesc.GetPointer(bufIndex);
        n_assert(0 != buf);
        ctx.uniforms[bufIndex] = buf;
        ctx.uniformSizes[bufIndex] = uniformDesc.GetBufferSize(bufIndex);
    }

	// setup intput/output buffer-numbers
	ctx.numInputs = inputDesc.GetNumBuffers();
	ctx.numOutputs = outputDesc.GetNumBuffers();

	// for each slice: set input-/output-buffers and call the Job-Function
    // calc number of slices
    const SizeT numSlices = (inputDesc.GetBufferSize(0) + (inputDesc.GetSliceSize(0) - 1)) / inputDesc.GetSliceSize(0);
    // input-slices must be the same number as output-slices
	n_assert((numSlices) == ((outputDesc.GetBufferSize(0) + (outputDesc.GetSliceSize(0) - 1)) / outputDesc.GetSliceSize(0)));
	IndexT slice;
	for(slice = 0; slice < numSlices; ++slice)
	{
		// input data
		for (bufIndex = 0; bufIndex < inputDesc.GetNumBuffers(); ++bufIndex)
		{
			ubyte* buf = ((ubyte*)inputDesc.GetPointer(bufIndex)) + (inputDesc.GetSliceSize(bufIndex) * slice);		
			n_assert(0 != buf);
			ctx.inputs[bufIndex] = buf;			

			const ubyte* end = ((ubyte*)inputDesc.GetPointer(bufIndex)) + inputDesc.GetBufferSize(bufIndex);
		    const SizeT sliceSize = Math::n_min(inputDesc.GetSliceSize(bufIndex), end - buf);
		    			
			ctx.inputSizes[bufIndex] = sliceSize;
		}

		// output data
		for (bufIndex = 0; bufIndex < outputDesc.GetNumBuffers(); ++bufIndex)
		{
			ubyte* buf = ((ubyte*)outputDesc.GetPointer(bufIndex)) + (outputDesc.GetSliceSize(bufIndex) * slice);
			n_assert(0 != buf);
			ctx.outputs[bufIndex] = buf;

			const ubyte* end = ((ubyte*)outputDesc.GetPointer(bufIndex)) + outputDesc.GetBufferSize(bufIndex);
		    const SizeT sliceSize = Math::n_min(outputDesc.GetSliceSize(bufIndex), end - buf);
		    
			ctx.outputSizes[bufIndex] = sliceSize;
		}

		// execute job function
		funcDesc.GetFunctionPointer()(ctx);
	}
}

//------------------------------------------------------------------------------
/**
    Push a job chain, where each job in the chain depends on the previous
    job. This will also guarantee, that the first job slice of each
    job will run on the same worker thread. In case of simple jobs
    (with only one slice) this improves load.
*/
void
SerialJobPort::PushJobChain(const Util::Array<Ptr<Job> >& jobs)
{    
    n_assert(!jobs.IsEmpty());
    IndexT i;
    for (i = 0; i < jobs.Size(); i++)
    {
        this->PushJob(jobs[i]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SerialJobPort::PushFlush()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
SerialJobPort::PushSync()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
SerialJobPort::WaitDone()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
SerialJobPort::CheckDone()
{
    return true;
}

} // namespace Jobs