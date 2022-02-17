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
#if NEBULA_GRAPHICS_DEBUG
    CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_GREEN, this->name.Value());
#endif

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

#if NEBULA_GRAPHICS_DEBUG
    CoreGraphics::CmdEndMarker(cmdBuf);
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
FramePass::Build(
    Memory::ArenaAllocator<BIG_CHUNK>& allocator,
    Util::Array<FrameOp::Compiled*>& compiledOps, 
    Util::Array<CoreGraphics::EventId>& events,
    Util::Array<CoreGraphics::BarrierId>& barriers,
    Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& buffers,
    Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures)
{
    // if not enable, abort early
    if (!this->enabled)
        return;

    CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(allocator);

#if NEBULA_GRAPHICS_DEBUG
    myCompiled->name = this->name;
#endif

    for (IndexT i = 0; i < this->children.Size(); i++)
    {
        this->children[i]->Build(allocator, myCompiled->subpasses, events, barriers, buffers, textures);
    }

    this->compiled = myCompiled;
    this->SetupSynchronization(allocator, events, barriers, buffers, textures);

    // Take the barriers from the children, this should hold all the barriers after building
    for (Frame::FrameOp::Compiled* child : myCompiled->subpasses)
    {
        myCompiled->barriers.AppendArray(child->barriers);
        child->barriers.Clear();
    }

    compiledOps.Append(myCompiled);
}

} // namespace Frame2
