//------------------------------------------------------------------------------
//  framesubmission.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "framesubmission.h"
#include "coregraphics/shaderserver.h"

#include "graphics/globalconstants.h"
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameSubmission::FrameSubmission() :
    queue(CoreGraphics::InvalidQueueType),
    resourceResetBarriers(nullptr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameSubmission::~FrameSubmission()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubmission::OnWindowResized()
{
    FrameOp::OnWindowResized();
    for (IndexT i = 0; i < this->children.Size(); i++)
    {
        this->children[i]->OnWindowResized();
    }
    if (this->resourceResetBarriers != nullptr)
    {
        IndexT i;
        for (i = 0; i < this->resourceResetBarriers->Size(); i++)
        {
            CoreGraphics::BarrierReset(this->resourceResetBarriers->Get(i));
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubmission::CompiledImpl::Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex)
{
    n_assert(cmdBuf == CoreGraphics::InvalidCmdBufferId);
    N_SCOPE_DYN(this->name.Value(), Graphics);
    CoreGraphics::CmdBufferCreateInfo cmdBufInfo;
    cmdBufInfo.pool = this->commandBufferPool;
    cmdBufInfo.usage = this->queue;
    cmdBufInfo.queryTypes = CoreGraphics::CmdBufferQueryBits::Timestamps;
    CoreGraphics::CmdBufferId submissionBuffer = CoreGraphics::CreateCmdBuffer(cmdBufInfo);

    // Begin recording command buffer
    CoreGraphics::CmdBufferBeginInfo beginInfo = { true, false, false };
    CoreGraphics::CmdBeginRecord(submissionBuffer, beginInfo);
    CoreGraphics::CmdBeginMarker(submissionBuffer, NEBULA_MARKER_PURPLE, this->name.Value());

    // Setup any constants required by the ops
    for (IndexT i = 0; i < this->compiled.Size(); i++)
    {
        this->compiled[i]->SetupConstants(bufferIndex);
    }

    // No more constant updates from this point
    CoreGraphics::LockConstantUpdates();

    // First thing, flush all constant updates
    CoreGraphics::FlushConstants(submissionBuffer, this->queue);
    CoreGraphics::FlushUpload();

    // Before starting the submission, flush updates
    Graphics::FlushUpdates(submissionBuffer, this->queue);

    for (IndexT i = 0; i < this->compiled.Size(); i++)
    {
        this->compiled[i]->QueuePreSync(submissionBuffer);
        this->compiled[i]->Run(submissionBuffer, frameIndex, bufferIndex);
    }

    // Before we start the new frame, we need to reset all the image layouts to their original state
    if (this->resourceResetBarriers && this->resourceResetBarriers->Size() > 0)
    {
        IndexT i;
        for (i = 0; i < this->resourceResetBarriers->Size(); i++)
        {
            // make sure to transition resources back to their original state in preparation for the next frame
            CoreGraphics::BarrierReset((*this->resourceResetBarriers)[i]);
            CoreGraphics::CmdBarrier(submissionBuffer, (*this->resourceResetBarriers)[i]);
        }
    }

    // Put the last frame marker
    CoreGraphics::CmdEndMarker(submissionBuffer);

    // Last thing, finish up the queries
    CoreGraphics::CmdFinishQueries(submissionBuffer);

    // End the recording and submit
    CoreGraphics::CmdEndRecord(submissionBuffer);
    this->submissionId = CoreGraphics::SubmitCommandBuffer(submissionBuffer, this->queue);

    // If a wait submission is present, append a wait for the subission we just did
    for (IndexT i = 0; i < this->waitSubmissions.Size(); i++)
        CoreGraphics::WaitForSubmission(this->waitSubmissions[i]->submissionId, this->queue, this->waitSubmissions[i]->queue);

    for (IndexT i = 0; i < this->waitQueues.Size(); i++)
        CoreGraphics::WaitForLastSubmission(this->queue, this->waitQueues[i]);

    // Delete command buffer
    CoreGraphics::DestroyCmdBuffer(submissionBuffer);

    // Open up for constant updates after waiting
    CoreGraphics::UnlockConstantUpdates();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubmission::CompiledImpl::Discard()
{
    for (IndexT i = 0; i < this->compiled.Size(); i++)
    {
        this->compiled[i]->Discard();
    }
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled* 
FrameSubmission::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
    CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
    ret->compiled = {};
    ret->name = this->name;
    ret->queue = this->queue;
    ret->waitQueues = this->waitQueues;
    ret->resourceResetBarriers = this->resourceResetBarriers;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubmission::Build(
    Memory::ArenaAllocator<BIG_CHUNK>& allocator,
    Util::Array<FrameOp::Compiled*>& compiledOps,
    Util::Array<CoreGraphics::EventId>& events,
    Util::Array<CoreGraphics::BarrierId>& barriers,
    Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& rwBuffers,
    Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures)
{
    // if not enable, abort early
    if (!this->enabled)
        return;

    CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(allocator);

    // build ops
    for (IndexT i = 0; i < this->children.Size(); i++)
    {
        this->children[i]->Build(allocator, myCompiled->compiled, events, barriers, rwBuffers, textures);
    }

    this->compiled = myCompiled;
    for (IndexT i = 0; i < this->waitSubmissions.Size(); i++)
        myCompiled->waitSubmissions.Append(static_cast<FrameSubmission::CompiledImpl*>(this->waitSubmissions[i]->compiled));
    myCompiled->commandBufferPool = this->commandBufferPool;
    compiledOps.Append(this->compiled);
}

} // namespace Frame
