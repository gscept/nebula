//------------------------------------------------------------------------------
// frameop.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "framescript.h"
#include "frameop.h"

namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameOp::FrameOp()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::~FrameOp()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameOp::Run(const IndexT frameIndex)
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
FrameOp::OnWindowResized()
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
FrameOp::Setup()
{
	// if operation needs any special setup, overload and perform
}

//------------------------------------------------------------------------------
/**
*/
void
FrameOp::Discard()
{
	// if operation needs to be discarded, overload and perform
}

//------------------------------------------------------------------------------
/**
	Runs the run function, overload if the mask needs to be used.
*/
void
FrameOp::RunSegment(const FrameOp::ExecutionMask mask, const IndexT frameIndex)
{
	this->Run(frameIndex);
}

} // namespace Frame2