//------------------------------------------------------------------------------
// framecopy.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framecopy.h"
#include "coregraphics/renderdevice.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameCopy::FrameCopy()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameCopy::~FrameCopy()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameCopy::Discard()
{
	FrameOp::Discard();
	this->from = CoreGraphics::RenderTextureId::Invalid();
	this->to = CoreGraphics::RenderTextureId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameCopy::Run(const IndexT frameIndex)
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
	renderDev->Copy(this->from, fromRegion, this->to, toRegion);
}

} // namespace Frame2