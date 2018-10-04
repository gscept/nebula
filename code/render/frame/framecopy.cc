//------------------------------------------------------------------------------
// framecopy.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framecopy.h"
#include "coregraphics/graphicsdevice.h"

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
FrameOp::Compiled*
FrameCopy::AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();

#if defined(NEBULA_GRAPHICS_DEBUG)
	ret->name = this->name;
#endif

	ret->from = this->from;
	ret->to = this->to;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameCopy::CompiledImpl::Run(const IndexT frameIndex)
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

#if defined(NEBULA_GRAPHICS_DEBUG)
	CoreGraphics::CmdBufBeginMarker(GraphicsQueueType, Math::float4(0.8f, 0.6f, 0.6f, 1), this->name.Value());
#endif

	CoreGraphics::Copy(this->from, fromRegion, this->to, toRegion);

#if defined(NEBULA_GRAPHICS_DEBUG)
	CoreGraphics::CmdBufEndMarker(GraphicsQueueType);
#endif
}

} // namespace Frame2