//------------------------------------------------------------------------------
// framebarrier.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "framebarrier.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameBarrier::FrameBarrier()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameBarrier::~FrameBarrier()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled* 
FrameBarrier::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
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
FrameBarrier::CompiledImpl::Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex)
{
    N_CMD_SCOPE(cmdBuf, NEBULA_MARKER_GRAY, this->name.Value());
}


} // namespace Frame2
