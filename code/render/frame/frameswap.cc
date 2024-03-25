//------------------------------------------------------------------------------
// framecopy.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "frameswap.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/swapchain.h"

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
    ret->from = this->from;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSwap::CompiledImpl::Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex)
{
    CoreGraphics::QueueBeginMarker(GraphicsQueueType, NEBULA_MARKER_GRAPHICS, "Swap");
    CoreGraphics::SwapchainId swapchain = WindowGetSwapchain(CoreGraphics::CurrentWindow);
    CoreGraphics::SwapchainSwap(swapchain);
    CoreGraphics::SwapchainCopy(swapchain, cmdBuf, this->from);
    CoreGraphics::QueueEndMarker(GraphicsQueueType);
}

} // namespace Frame2
