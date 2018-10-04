//------------------------------------------------------------------------------
// frameglobalstate.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
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
FrameGlobalState::AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameGlobalState::CompiledImpl::Run(const IndexT frameIndex)
{
}

} // namespace Frame2