//------------------------------------------------------------------------------
// framebarrier.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framebarrier.h"
#include "coregraphics/graphicsdevice.h"

using namespace CoreGraphics;
namespace Frame
{

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
	CoreGraphics::BarrierInsert(this->barrier, this->queue);
}

} // namespace Frame2