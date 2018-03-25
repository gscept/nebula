//------------------------------------------------------------------------------
// frameswapbuffers.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
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
void
FrameSwapbuffers::Run(const IndexT frameIndex)
{
	CoreGraphics::RenderTextureSwapBuffers(this->tex);
}

} // namespace Frame2