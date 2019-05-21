//------------------------------------------------------------------------------
// frameswapbuffers.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "frameswapbuffers.h"

namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameSwapbuffers::FrameSwapbuffers()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameSwapbuffers::~FrameSwapbuffers()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSwapbuffers::Discard()
{
	FrameOp::Discard();

	this->tex = CoreGraphics::RenderTextureId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameSwapbuffers::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->tex = this->tex;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSwapbuffers::CompiledImpl::Run(const IndexT frameIndex)
{
	CoreGraphics::RenderTextureSwapBuffers(this->tex);
}

} // namespace Frame2