//------------------------------------------------------------------------------
// frameglobalstate.cc
// (C) 2016 Individual contributors, see AUTHORS file
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
FrameGlobalState::AllocCompiled(Memory::ChunkAllocator<0xFFFF>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->state = this->state;
	ret->constants = this->constants;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameGlobalState::CompiledImpl::Run(const IndexT frameIndex)
{
	// commit
	CoreGraphics::SetShaderState(this->state);
}

} // namespace Frame2