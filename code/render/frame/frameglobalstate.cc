//------------------------------------------------------------------------------
// frameglobalstate.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "frameglobalstate.h"
#include "coregraphics/shader.h"

namespace Frame
{

__ImplementClass(Frame::FrameGlobalState, 'FRGS', Frame::FrameOp);
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
void
FrameGlobalState::Discard()
{
	FrameOp::Discard();

	IndexT i;
	for (i = 0; i < this->constants.Size(); i++) CoreGraphics::ShaderDestroyState(this->state);
	this->constants.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameGlobalState::Run(const IndexT frameIndex)
{
	// commit
	CoreGraphics::ShaderStateApply(this->state);
}

} // namespace Frame2