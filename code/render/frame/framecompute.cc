//------------------------------------------------------------------------------
// framecompute.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "framecompute.h"
#include "coregraphics/renderdevice.h"

using namespace CoreGraphics;
namespace Frame2
{

__ImplementClass(Frame2::FrameCompute, 'FRCM', Frame2::FrameOp);
//------------------------------------------------------------------------------
/**
*/
FrameCompute::FrameCompute() :
	state(NULL)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameCompute::~FrameCompute()
{
	// empty
}


//------------------------------------------------------------------------------
/**
*/
void
FrameCompute::Discard()
{
	FrameOp::Discard();
	this->state = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameCompute::Run(const IndexT frameIndex)
{
	n_assert(this->state.isvalid());

	RenderDevice* dev = RenderDevice::Instance();

	// apply state
	this->state->SelectActiveVariation(this->mask);
	this->state->Apply();
	this->state->Commit();

	// compute
	dev->Compute(this->x, this->y, this->z);
}

} // namespace Frame2