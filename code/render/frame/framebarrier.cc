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
FrameOp::Compiled* 
FrameBarrier::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();

#if NEBULA_GRAPHICS_DEBUG
	ret->name = this->name;
#endif

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameBarrier::CompiledImpl::Run(const IndexT frameIndex)
{
#if NEBULA_GRAPHICS_DEBUG
	CoreGraphics::CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_GRAY, this->name.Value());
	CoreGraphics::CommandBufferEndMarker(GraphicsQueueType);
#endif
}


//------------------------------------------------------------------------------
/**
*/
void
FrameBarrier::CompiledImpl::Discard()
{
	// empty 
}


} // namespace Frame2