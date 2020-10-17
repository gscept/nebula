//------------------------------------------------------------------------------
// framecopy.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
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
FrameCopy::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();

#if NEBULA_GRAPHICS_DEBUG
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
FrameCopy::CompiledImpl::Run(const IndexT frameIndex, const IndexT bufferIndex)
{
	// get dimensions
	CoreGraphics::TextureDimensions fromDims = TextureGetDimensions(this->from);
	CoreGraphics::TextureDimensions toDims = TextureGetDimensions(this->to);

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

#if NEBULA_GRAPHICS_DEBUG
	CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_RED, this->name.Value());
#endif

	CoreGraphics::Copy(GraphicsQueueType, this->from, fromRegion, 0, 0, this->to, toRegion, 0, 0);

#if NEBULA_GRAPHICS_DEBUG
	CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
}

} // namespace Frame2