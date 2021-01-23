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
FramePass::AddSubpass(FrameSubpass* subpass)
{
    this->subpasses.Append(subpass);
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
    for (i = 0; i < this->subpasses.Size(); i++) this->subpasses[i]->Discard();
    this->subpasses.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
FramePass::CompiledImpl::UpdateResources(const IndexT frameIndex, const IndexT bufferIndex)
{
	PassUpdateResources(this->pass, bufferIndex);
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
    for (i = 0; i < this->subpasses.Size(); i++)
    {
        this->subpasses[i]->OnWindowResized();
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
    Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures)
{
    CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(allocator);

#if NEBULA_GRAPHICS_DEBUG
    myCompiled->name = this->name;
#endif

    Util::Array<FrameOp::Compiled*> subpassOps;
    for (IndexT i = 0; i < this->subpasses.Size(); i++)
    {
        this->subpasses[i]->Build(allocator, subpassOps, events, barriers, rwBuffers, textures);
    }
    myCompiled->subpasses = subpassOps;
    this->compiled = myCompiled;
    this->SetupSynchronization(allocator, events, barriers, rwBuffers, textures);

    // first add dependency for color attachments
    const Util::Array<CoreGraphics::TextureViewId>& attachments = CoreGraphics::PassGetAttachments(this->pass);
    for (IndexT i = 0; i < attachments.Size(); i++)
    {
        TextureId tex = TextureViewGetTexture(attachments[i]);
        IndexT idx = textures.FindIndex(tex);
        n_assert(idx != InvalidIndex);
        Util::Array<TextureDependency>& deps = textures.ValueAtIndex(idx);
        uint layers = CoreGraphics::TextureGetNumLayers(tex);
        uint mips = CoreGraphics::TextureGetNumMips(tex);
        CoreGraphics::ImageSubresourceInfo subres{ 
            CoreGraphics::ImageAspect::ColorBits,
            0, mips, 0, layers };
        TextureDependency dep{
            this->compiled, 
            this->queue, 
            CoreGraphics::ImageLayout::ShaderRead,
            CoreGraphics::BarrierStage::PassOutput,
            CoreGraphics::BarrierAccess::ColorAttachmentWrite,
            DependencyIntent::Write, 
            this->index,
            subres};
        deps.Append(dep);
    }

    // then add potential dependency for depth-stencil attachment
    CoreGraphics::TextureViewId depthStencilAttachment = CoreGraphics::PassGetDepthStencilAttachment(this->pass);
    if (depthStencilAttachment != CoreGraphics::InvalidTextureViewId)
    {
        TextureId tex = TextureViewGetTexture(depthStencilAttachment);
        IndexT idx = textures.FindIndex(tex);
        n_assert(idx != InvalidIndex);
        Util::Array<TextureDependency>& deps = textures.ValueAtIndex(idx);
        uint layers = CoreGraphics::TextureGetNumLayers(tex);
        uint mips = CoreGraphics::TextureGetNumMips(tex);
        CoreGraphics::ImageSubresourceInfo subres{
            CoreGraphics::ImageAspect::DepthBits | CoreGraphics::ImageAspect::StencilBits,
            0, mips, 0, layers };
        TextureDependency dep{
            this->compiled,
            this->queue,
            CoreGraphics::ImageLayout::DepthStencilRead,
            CoreGraphics::BarrierStage::LateDepth,
            CoreGraphics::BarrierAccess::DepthAttachmentWrite,
            DependencyIntent::Write,
            this->index,
            subres };
        deps.Append(dep);
    }
    compiledOps.Append(myCompiled);
}

} // namespace Frame2
