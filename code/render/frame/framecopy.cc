//------------------------------------------------------------------------------
// framecopy.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "framecopy.h"
#include "coregraphics/graphicsdevice.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameCopy::FrameCopy()
    : fromBits(CoreGraphics::ImageBits::ColorBits)
    , toBits(CoreGraphics::ImageBits::ColorBits)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameCopy::~FrameCopy()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameCopy::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
    CompiledImpl* ret = allocator.Alloc<CompiledImpl>();

#if NEBULA_GRAPHICS_DEBUG
    ret->name = this->name;
#endif

    ret->fromBits = this->fromBits;
    ret->toBits = this->toBits;
    ret->from = this->from;
    ret->to = this->to;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameCopy::CompiledImpl::Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex)
{
    // get dimensions
    CoreGraphics::TextureDimensions fromDims = TextureGetDimensions(this->from);
    CoreGraphics::TextureDimensions toDims = TextureGetDimensions(this->to);
    N_CMD_SCOPE(cmdBuf, NEBULA_MARKER_TRANSFER, this->name.Value());

    CoreGraphics::TextureCopy from{ .region = { 0, 0, fromDims.width, fromDims.height }, .mip = 0, .layer = 0, .bits = this->fromBits };
    CoreGraphics::TextureCopy to{ .region = { 0, 0, toDims.width, toDims.height }, .mip = 0, .layer = 0, .bits = this->toBits };
    CoreGraphics::CmdCopy(cmdBuf, this->from, { from }, this->to, { to });
}

} // namespace Frame2
