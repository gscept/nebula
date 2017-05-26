//------------------------------------------------------------------------------
// framecopy.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "framecopy.h"
#include "coregraphics/renderdevice.h"

using namespace CoreGraphics;
namespace Frame2
{

__ImplementClass(Frame2::FrameCopy, 'FRCO', Frame2::FrameOp);
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
	this->from = 0;
	this->to = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameCopy::Run(const IndexT frameIndex)
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
	renderDev->Copy(this->from->GetTexture(), fromRegion, this->to->GetTexture(), toRegion);
}

} // namespace Frame2