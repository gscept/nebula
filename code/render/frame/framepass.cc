//------------------------------------------------------------------------------
// framepass.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framepass.h"
#include "coregraphics/graphicsdevice.h"
#include "profiling/profiling.h"

using namespace CoreGraphics;

namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FramePass::FramePass()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FramePass::~FramePass()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
FramePass::Discard()
{
    FrameOp::Discard();

    DestroyPass(this->pass);
    this->pass = InvalidPassId;

    IndexT i;
    for (i = 0; i < this->children.Size(); i++) this->children[i]->Discard();
    this->children.Clear();
}

//------------------------------------------------------------------------------
/**
    Builds jobs in parallel in preparation for the Run() function later
*/
void 
FramePass::CompiledImpl::RunJobs(const IndexT frameIndex, const IndexT bufferIndex)
{
    // begin pass
    PassBegin(this->pass, PassRecordMode::Record);

    // run subpasses
    IndexT i;
    for (i = 0; i < this->subpasses.Size(); i++)
    {
        N_SCOPE(RunSubpassRecord, Render);

        // start subpass commands
        CoreGraphics::BeginSubpassCommands(this->subpassBuffers[i][bufferIndex]);
        // progress to next subpass if not on first iteration
        if (i > 0)
            PassNextSubpass(this->pass);

        // execute contents of this subpass and synchronize
        // note that we overload the cross queue sync so we do it outside the render pass
        this->subpasses[i]->QueuePreSync();
        this->subpasses[i]->Run(frameIndex, bufferIndex);
        this->subpasses[i]->QueuePostSync();
        
        // finish the draw thread
        CoreGraphics::EndSubpassCommands();
    }

    PassEnd(this->pass);
}

//------------------------------------------------------------------------------
/**
*/
void
FramePass::CompiledImpl::Run(const IndexT frameIndex, const IndexT bufferIndex)
{
#if NEBULA_ENABLE_MT_DRAW
    n_assert(this->subpassBuffers.Size() == this->subpasses.Size());
#endif

#if NEBULA_GRAPHICS_DEBUG
    CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_GREEN, this->name.Value());
#endif

    // begin pass
    PassBegin(this->pass, PassRecordMode::ExecuteRecorded);
    IndexT bufferedIndex = CoreGraphics::GetBufferedFrameIndex();

    // run subpasses
    IndexT i;
    for (i = 0; i < this->subpasses.Size(); i++)
    {
        // progress to next subpass if not on first iteration
        if (i > 0) 
            PassNextSubpass(this->pass);

#if NEBULA_ENABLE_MT_DRAW
        CoreGraphics::ExecuteCommands(this->subpassBuffers[i][bufferedIndex]);
#else
        // execute contents of this subpass and synchronize
        // note that we overload the cross queue sync so we do it outside the render pass
        this->subpasses[i]->QueuePreSync();
        this->subpasses[i]->Run(frameIndex, bufferedIndex);
        this->subpasses[i]->QueuePostSync();
#endif
    }

    // end pass
    PassEnd(this->pass);

#if NEBULA_GRAPHICS_DEBUG
    CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
FramePass::CompiledImpl::Discard()
{
    for (IndexT i = 0; i < this->subpasses.Size(); i++)
    {
        this->subpasses[i]->Discard();

        // free up subpass buffers
        for (IndexT j = 0; j < this->subpassBuffers[i].Size(); j++)
        {
            CoreGraphics::DestroyCommandBuffer(this->subpassBuffers[i][j]);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FramePass::OnWindowResized()
{
    // resize pass
    PassWindowResizeCallback(this->pass);

    IndexT i;
    for (i = 0; i < this->children.Size(); i++)
    {
        this->children[i]->OnWindowResized();
    }
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled* 
FramePass::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
    CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
    ret->pass = this->pass;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
FramePass::Build(
    Memory::ArenaAllocator<BIG_CHUNK>& allocator,
    Util::Array<FrameOp::Compiled*>& compiledOps, 
    Util::Array<CoreGraphics::EventId>& events,
    Util::Array<CoreGraphics::BarrierId>& barriers,
    Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& rwBuffers,
    Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures,
    CoreGraphics::CommandBufferPoolId commandBufferPool)
{
    // if not enable, abort early
    if (!this->enabled)
        return;

    CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(allocator);

#if NEBULA_GRAPHICS_DEBUG
    myCompiled->name = this->name;
#endif

    Util::Array<FrameOp::Compiled*> subpassOps;
    for (IndexT i = 0; i < this->children.Size(); i++)
    {
        this->children[i]->Build(allocator, subpassOps, events, barriers, rwBuffers, textures, commandBufferPool);
    }
    myCompiled->subpasses = subpassOps;
    this->compiled = myCompiled;
    this->SetupSynchronization(allocator, events, barriers, rwBuffers, textures);
    compiledOps.Append(myCompiled);

#if NEBULA_ENABLE_MT_DRAW
    myCompiled->subpassBuffers.Resize(myCompiled->subpasses.Size());
    for (IndexT j = 0; j < myCompiled->subpasses.Size(); j++)
    {
        CoreGraphics::CommandBufferCreateInfo cmdInfo =
        {
            true,
            commandBufferPool
        };

        // allocate a subpass buffer for each buffered frame
        SizeT numBufferedFrames = CoreGraphics::GetNumBufferedFrames();
        myCompiled->subpassBuffers[j].Resize(numBufferedFrames);
        for (IndexT k = 0; k < numBufferedFrames; k++)
        {
            myCompiled->subpassBuffers[j][k] = CoreGraphics::CreateCommandBuffer(cmdInfo);
        }
    }
#endif
}

} // namespace Frame2
