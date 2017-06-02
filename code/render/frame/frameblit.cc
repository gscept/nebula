//------------------------------------------------------------------------------
// frameblit.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "frameblit.h"
#include "coregraphics/renderdevice.h"

using namespace CoreGraphics;
namespace Frame
{

__ImplementClass(Frame::FrameBlit, 'FRBL', Frame::FrameOp);
//------------------------------------------------------------------------------
/**
*/
FrameBlit::FrameBlit()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameBlit::~FrameBlit()
{
	// empty
}


//------------------------------------------------------------------------------
/**
*/
void
FrameBlit::Discard()
{
	FrameOp::Discard();
	this->from = 0;
	this->to = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameBlit::Run(const IndexT frameIndex)
{
	RenderDevice* renderDev = RenderDevice::Instance();

	// setup regions
	Math::rectangle<SizeT> fromRegion;
	fromRegion.left = 0;
	fromRegion.top = 0;
	fromRegion.right = this->from->GetWidth();
	fromRegion.bottom = this->from->GetHeight();
	Math::rectangle<SizeT> toRegion;
	toRegion.left = 0;
	toRegion.top = 0;
	toRegion.right = this->to->GetWidth();
	toRegion.bottom = this->to->GetHeight();
	renderDev->Blit(this->from, fromRegion, 0, this->to, toRegion, 0);
}

} // namespace Frame2