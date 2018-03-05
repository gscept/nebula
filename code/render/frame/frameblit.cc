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
	this->from = RenderTextureId::Invalid();
	this->to = RenderTextureId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameBlit::Run(const IndexT frameIndex)
{
	RenderDevice* renderDev = RenderDevice::Instance();

	// get dimensions
	CoreGraphics::TextureDimensions fromDims = RenderTextureGetDimensions(this->from);
	CoreGraphics::TextureDimensions toDims = RenderTextureGetDimensions(this->to);

	// setup regions
	Math::rectangle<SizeT> fromRegion;
	fromRegion.left = 0;
	fromRegion.top = 0;
	fromRegion.right = fromDims.width;
	fromRegion.bottom = fromDims.height;
	Math::rectangle<SizeT> toRegion;
	toRegion.left = 0;
	toRegion.top = 0;
	toRegion.right = toDims.width;
	toRegion.bottom = toDims.height;
	renderDev->Blit(this->from, fromRegion, 0, this->to, toRegion, 0);
}

} // namespace Frame2