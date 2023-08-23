//------------------------------------------------------------------------------
// framepass.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "framepass.h"
#include "coregraphics/graphicsdevice.h"

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
    for (i = 0; i < this->children.Size(); i++)
        this->children[i]->Discard();
    this->children.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
FramePass::CompiledImpl::Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex)
{
    N_CMD_SCOPE(cmdBuf, NEBULA_MARKER_GREEN, this->name.Value());

    // begin pass
    CoreGraphics::CmdBeginPass(cmdBuf, this->pass); 
    IndexT bufferedIndex = CoreGraphics::GetBufferedFrameIndex();
    
    // run subpasses
    IndexT i;
    for (i = 0; i < this->subpasses.Size(); i++)
    {
        // progress to next subpass if not on first iteration
        if (i > 0)
            CoreGraphics::CmdNextSubpass(cmdBuf);

        // execute contents of this subpass and synchronize
        // note that we overload the cross queue sync so we do it outside the render pass
        this->subpasses[i]->QueuePreSync(cmdBuf);
        this->subpasses[i]->Run(cmdBuf, frameIndex, bufferedIndex);
    }

    // end pass
    CoreGraphics::CmdEndPass(cmdBuf);
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
    ret->subpasses = {};
    ret->pass = this->pass;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
FramePass::Build(const BuildContext& ctx)
{
    // if not enable, abort early
    if (!this->enabled)
        return;

    CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(ctx.allocator);

#if NEBULA_GRAPHICS_DEBUG
    myCompiled->name = this->name;
#endif

    BuildContext newCtx = { ctx.script, ctx.allocator, myCompiled->subpasses, ctx.events, ctx.barriers, ctx.buffers, ctx.textures };
    for (IndexT i = 0; i < this->children.Size(); i++)
    {
        this->children[i]->Build(newCtx);
    }

    this->compiled = myCompiled;
    this->SetupSynchronization(ctx.allocator, ctx.events, ctx.barriers, ctx.buffers, ctx.textures);

    // Take the barriers from the children, this should hold all the barriers after building
    for (Frame::FrameOp::Compiled* child : myCompiled->subpasses)
    {
        myCompiled->barriers.AppendArray(child->barriers);
        child->barriers.Clear();
    }

    ctx.compiledOps.Append(myCompiled);
}

} // namespace Frame2
