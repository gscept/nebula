//------------------------------------------------------------------------------
// frameswapbuffers.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "frameswapbuffers.h"

namespace Frame
{

__ImplementClass(Frame::FrameSwapbuffers, 'FRSW', Frame::FrameOp);
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

	this->tex = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSwapbuffers::Run(const IndexT frameIndex)
{
	this->tex->SwapBuffers();
}

} // namespace Frame2