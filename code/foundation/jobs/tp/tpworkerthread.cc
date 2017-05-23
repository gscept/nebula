//------------------------------------------------------------------------------
//  tpworkerthread.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "jobs/tp/tpworkerthread.h"
#include "jobs/job.h"
#include "jobs/jobfuncdesc.h"
#include "debug/debugserver.h"

namespace Jobs
{
__ImplementClass(Jobs::TPWorkerThread, 'TPWT', Threading::Thread);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
TPWorkerThread::TPWorkerThread() :
    scratchBuffer(0)
{
    this->jobQueue.SetSignalOnEnqueueEnabled(true);
}

//------------------------------------------------------------------------------
/**
*/
TPWorkerThread::~TPWorkerThread()
{
    n_assert(this->jobQueue.IsEmpty());
}

//------------------------------------------------------------------------------
/**
*/
void
TPWorkerThread::EmitWakeupSignal()
{
    this->jobQueue.Signal();
}

//------------------------------------------------------------------------------
/**
*/
void
TPWorkerThread::PushJobCommand(const TPJobCommand& cmd)
{
    this->jobQueue.Enqueue(cmd);
}

//------------------------------------------------------------------------------
/**
*/
void
TPWorkerThread::DoWork()
{
    // allocate the scratch buffer
    n_assert(0 == this->scratchBuffer);
    this->scratchBuffer = (ubyte*) Memory::Alloc(Memory::ScratchHeap, MaxScratchSize);

    Array<TPJobCommand> curCommands;
    while (!this->ThreadStopRequested())
    {
        // dequeue current work entries
        this->jobQueue.DequeueAll(curCommands);

        // process work entries
        IndexT i;
        for (i = 0; i < curCommands.Size(); i++)
        {
            const TPJobCommand& curCmd = curCommands[i];
            switch (curCmd.GetCode())
            {
                case TPJobCommand::Run:
                    this->ProcessJobSlices(curCmd.GetFirstSlice(), curCmd.GetNumSlices(), curCmd.GetStride());
                    break;

                case TPJobCommand::Sync:
                    curCmd.GetSyncEvent()->Wait();
                    break;
            }
        }

        // wait for new jobs
        this->jobQueue.Wait();
    }   

    // free scratch buffer
    Memory::Free(Memory::ScratchHeap, this->scratchBuffer);
    this->scratchBuffer = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
TPWorkerThread::ProcessJobSlices(TPJobSlice* firstSlice, ushort numSlices, ushort stride)
{
    TPJobSlice* curSlice = firstSlice;
    TPJob* job = curSlice->GetJob();
    const JobUniformDesc& uniformDesc = job->GetUniformDesc();
    const JobDataDesc& inputDesc = job->GetInputDesc();
    const JobDataDesc& outputDesc = job->GetOutputDesc();
    const JobFuncDesc& funcDesc = job->GetFuncDesc();

    IndexT i;
    for (i = 0; i < numSlices; i++)
    {
        n_assert(curSlice->GetJob() == job);
        IndexT sliceIndex = curSlice->GetSliceIndex();

        JobFuncContext ctx = { 0 };

        // setup uniform data
        unsigned int bufIndex;
        ctx.numUniforms = uniformDesc.GetNumBuffers();
        for (bufIndex = 0; bufIndex < ctx.numUniforms; bufIndex++)
        {
            ctx.uniforms[bufIndex] = (ubyte*) uniformDesc.GetPointer(bufIndex);
            ctx.uniformSizes[bufIndex] = uniformDesc.GetBufferSize(bufIndex);
        }

        // setup scratch buffer
        if (uniformDesc.GetScratchSize() > 0)
        {
            n_assert(uniformDesc.GetScratchSize() < MaxScratchSize);
            ctx.scratch = this->scratchBuffer;
        }
        else
        {
            ctx.scratch = 0;
        }

        // setup slice inputs
        ctx.numInputs = inputDesc.GetNumBuffers();
        for (bufIndex = 0; bufIndex < ctx.numInputs; bufIndex++)
        {
            ubyte* buf = (ubyte*) inputDesc.GetPointer(bufIndex);
            n_assert(0 != buf);
            IndexT offset = sliceIndex * inputDesc.GetSliceSize(bufIndex);
            ctx.inputs[bufIndex] = buf + offset;
            ctx.inputSizes[bufIndex] = Math::n_min(inputDesc.GetSliceSize(bufIndex), inputDesc.GetBufferSize(bufIndex) - offset);
        }
        ctx.numOutputs = outputDesc.GetNumBuffers();
        for (bufIndex = 0; bufIndex < ctx.numOutputs; bufIndex++)
        {
            ubyte* buf = (ubyte*) outputDesc.GetPointer(bufIndex);
            n_assert(0 != buf);
            IndexT offset = sliceIndex * outputDesc.GetSliceSize(bufIndex);
            ctx.outputs[bufIndex] = buf + offset;
            ctx.outputSizes[bufIndex] = Math::n_min(outputDesc.GetSliceSize(bufIndex), outputDesc.GetBufferSize(bufIndex) - offset);
        }
    
        // invoke processing function
        n_assert(0 != ctx.inputs[0]);
        n_assert(0 != ctx.outputs[0]);
#if NEBULA3_ENABLE_PROFILING
        if (Debug::DebugServer::HasInstance())
        {   
            if (!this->debugTimer.isvalid())
            {
                this->debugTimer = Debug::DebugTimer::Create();
                this->debugTimer->Setup(this->GetName());
            }
            this->debugTimer->Start();       
        }
#endif
        funcDesc.GetFunctionPointer()(ctx);
#if NEBULA3_ENABLE_PROFILING
        if (this->debugTimer.isvalid())
        {
            this->debugTimer->Stop();    
        }           
#endif
        // proceed to next slice
        curSlice += stride;
    }
    job->NotifySlicesComplete(numSlices);
}

//------------------------------------------------------------------------------
/**
*/
void 
TPWorkerThread::Stop()
{     
#if NEBULA3_ENABLE_PROFILING
    if (this->debugTimer.isvalid())
    {
        _discard_timer(this->debugTimer);    
    }       
#endif
    Threading::Thread::Stop();    
}
} // namespace Jobs
