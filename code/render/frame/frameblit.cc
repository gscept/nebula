//------------------------------------------------------------------------------
// frameblit.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "frameblit.h"
#include "coregraphics/graphicsdevice.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameBlit::FrameBlit()
    : fromBits(CoreGraphics::ImageBits::ColorBits)
    , toBits(CoreGraphics::ImageBits::ColorBits)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameBlit::~FrameBlit()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameBlit::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
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
FrameBlit::CompiledImpl::Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex)
{
    // get dimensions
    CoreGraphics::TextureDimensions fromDims = TextureGetDimensions(this->from);
    CoreGraphics::TextureDimensions toDims = TextureGetDimensions(this->to);

    // setup regions
    Math::rectangle<SizeT> fromRegion;
    fromRegion.left = 0;
    fromRegion.top = 0;
    fromRegion.right = fromDims.width;
    fromRegion.bottom = fromDims.height;
    Math::rectangle<SizeT> toRegion;
    toRegion.left = 0;
    toRegion.top = 0;
    toRegion.right = toDims.width;
    toRegion.bottom = toDims.height;

    N_CMD_SCOPE(cmdBuf, NEBULA_MARKER_TRANSFER, this->name.Value());

    CoreGraphics::CmdBlit(cmdBuf, this->from, fromRegion, this->fromBits, 0, 0, this->to, toRegion, this->toBits, 0, 0);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameBlit::CompiledImpl::Discard()
{
    this->from = InvalidTextureId;
    this->to = InvalidTextureId;
}

} // namespace Frame2
