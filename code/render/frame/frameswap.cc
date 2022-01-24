//------------------------------------------------------------------------------
// framecopy.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "frameswap.h"
#include "coregraphics/graphicsdevice.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameSwap::FrameSwap()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameSwap::~FrameSwap()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameSwap::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
    CompiledImpl* ret = allocator.Alloc<CompiledImpl>();

#if NEBULA_GRAPHICS_DEBUG
    ret->name = this->name;
#endif
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSwap::CompiledImpl::Run(const IndexT frameIndex, const IndexT bufferIndex)
{
    CoreGraphics::QueueBeginMarker(GraphicsQueueType, NEBULA_MARKER_GRAPHICS, "Swap");
    CoreGraphics::Swap(0);
    CoreGraphics::QueueEndMarker(GraphicsQueueType);
}

} // namespace Frame2
