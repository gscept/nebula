//------------------------------------------------------------------------------
// framebarrier.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "framebarrier.h"
#include "coregraphics/renderdevice.h"

using namespace CoreGraphics;
namespace Frame2
{

__ImplementClass(Frame2::FrameBarrier, 'FMBR', Frame2::FrameOp);
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
void
FrameBarrier::Run(const IndexT frameIndex)
{
	RenderDevice* dev = RenderDevice::Instance();
	dev->InsertBarrier(this->barrier);
}

} // namespace Frame2