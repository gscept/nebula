//------------------------------------------------------------------------------
// framemipmap.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framemipmap.h"
#include "coregraphics/graphicsdevice.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameMipmap::FrameMipmap()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameMipmap::~FrameMipmap()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameMipmap::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
    CompiledImpl* ret = allocator.Alloc<CompiledImpl>();

#if NEBULA_GRAPHICS_DEBUG
    ret->name = this->name;
#endif

    ret->tex = this->tex;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameMipmap::CompiledImpl::Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex)
{
#if NEBULA_GRAPHICS_DEBUG
    CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_RED, this->name.Value());
#endif

    CoreGraphics::TextureGenerateMipmaps(cmdBuf, this->tex);
#if NEBULA_GRAPHICS_DEBUG
    CoreGraphics::CmdEndMarker(cmdBuf);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
FrameMipmap::CompiledImpl::Discard()
{
    this->tex = InvalidTextureId;
}

} // namespace Frame2
