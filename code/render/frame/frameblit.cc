//------------------------------------------------------------------------------
// frameblit.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "frameblit.h"
#include "coregraphics/graphicsdevice.h"

using namespace CoreGraphics;
namespace Frame
{

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
FrameOp::Compiled*
FrameBlit::AllocCompiled(Memory::ChunkAllocator<0xFFFF>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->from = this->from;
	ret->to = this->to;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameBlit::CompiledImpl::Run(const IndexT frameIndex)
{
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
	CoreGraphics::Blit(this->from, fromRegion, 0, this->to, toRegion, 0);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameBlit::CompiledImpl::Discard()
{
	this->from = RenderTextureId::Invalid();
	this->to = RenderTextureId::Invalid();
}

} // namespace Frame2