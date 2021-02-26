//------------------------------------------------------------------------------
//  framesubmission.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framesubmission.h"
#include "coregraphics/graphicsdevice.h"
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameSubmission::FrameSubmission() :
    queue(CoreGraphics::InvalidQueueType),
    waitQueue(CoreGraphics::InvalidQueueType),
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
FrameSubmission::CompiledImpl::RunJobs(const IndexT frameIndex, const IndexT bufferIndex)
{
    for (IndexT i = 0; i < this->compiled.Size(); i++)
    {
        this->compiled[i]->RunJobs(frameIndex, bufferIndex);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubmission::CompiledImpl::Run(const IndexT frameIndex, const IndexT bufferIndex)
{
    CoreGraphics::BeginSubmission(this->queue, this->waitQueue);
    for (IndexT i = 0; i < this->compiled.Size(); i++)
    {
		this->compiled[i]->QueuePreSync();
		this->compiled[i]->Run(frameIndex, bufferIndex);
		this->compiled[i]->QueuePostSync();
	}

    // I will admit, this is a little hacky, and in the future we might put this in its own command...
    if (this->resourceResetBarriers && this->resourceResetBarriers->Size() > 0)
    {
        IndexT i;
        for (i = 0; i < this->resourceResetBarriers->Size(); i++)
        {
            // make sure to transition resources back to their original state in preparation for the next frame
            CoreGraphics::BarrierReset((*this->resourceResetBarriers)[i]);
            CoreGraphics::BarrierInsert((*this->resourceResetBarriers)[i], this->queue);
        }
    }
    CoreGraphics::EndSubmission(this->queue, this->waitQueue);
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled* 
FrameSubmission::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
    CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
    ret->queue = this->queue;
    ret->waitQueue = this->waitQueue;
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
    Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures,
    CoreGraphics::CommandBufferPoolId commandBufferPool)
{
    CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(allocator);

    // build ops
    for (IndexT i = 0; i < this->ops.Size(); i++)
    {
        this->ops[i]->Build(allocator, myCompiled->compiled, events, barriers, rwBuffers, textures, commandBufferPool);
    }

    this->compiled = myCompiled;
    compiledOps.Append(this->compiled);
}

} // namespace Frame
