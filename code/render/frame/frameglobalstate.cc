//------------------------------------------------------------------------------
// frameglobalstate.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "frameglobalstate.h"
#include "coregraphics/graphicsdevice.h"

namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameGlobalState::FrameGlobalState()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameGlobalState::~FrameGlobalState()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled* 
FrameGlobalState::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
    CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameGlobalState::CompiledImpl::Run(const IndexT frameIndex, const IndexT bufferIndex)
{
}

} // namespace Frame2
