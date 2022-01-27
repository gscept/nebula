//------------------------------------------------------------------------------
// framesubpass.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framesubpass.h"
#include "coregraphics/graphicsdevice.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameSubpass::FrameSubpass()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameSubpass::~FrameSubpass()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpass::Discard()
{
    FrameOp::Discard();

    IndexT i;
    for (i = 0; i < this->children.Size(); i++)
    {
        this->children[i]->Discard();
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
FrameSubpass::OnWindowResized()
{

    IndexT i;
    for (i = 0; i < this->children.Size(); i++)
    {
        this->children[i]->OnWindowResized();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpass::CompiledImpl::Run(const IndexT frameIndex, const IndexT bufferIndex)
{
    IndexT i;

#if NEBULA_GRAPHICS_DEBUG
    CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_GREEN, this->name.Value());
#endif

    // run ops
    for (i = 0; i < this->ops.Size(); i++)
    {
        this->ops[i]->Run(frameIndex, bufferIndex);
    }

#if NEBULA_GRAPHICS_DEBUG
    CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpass::CompiledImpl::Discard()
{
    for (IndexT i = 0; i < this->ops.Size(); i++)
        this->ops[i]->Discard();
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameSubpass::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
    CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
    ret->ops = {};
    ret->viewports = {};
    ret->scissors = {};
#if NEBULA_GRAPHICS_DEBUG
    ret->name = this->name;
#endif
    // don't set ops here, we have to do it when we build
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
FrameSubpass::Build(
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
    
    for (IndexT i = 0; i < this->children.Size(); i++)
    {
        this->children[i]->Build(allocator, myCompiled->ops, events, barriers, rwBuffers, textures, commandBufferPool);
    }
    this->compiled = myCompiled;
    compiledOps.Append(myCompiled);
}

} // namespace Frame2
