//------------------------------------------------------------------------------
// framesubpassplugins.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framesubpassplugins.h"
#include "coregraphics/graphicsdevice.h"
#include "graphics/graphicsserver.h"
#include "frame/framebatchtype.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameSubpassPlugins::FrameSubpassPlugins()// :
	//pluginRegistry(NULL)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameSubpassPlugins::~FrameSubpassPlugins()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassPlugins::Discard()
{
	FrameOp::Discard();	
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameSubpassPlugins::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->pluginFilter = this->pluginFilter;	
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassPlugins::Setup()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassPlugins::CompiledImpl::Run(const IndexT frameIndex)
{
	CoreGraphics::BeginBatch(FrameBatchType::System);
    Graphics::GraphicsServer::Instance()->RenderPlugin(pluginFilter);	
	CoreGraphics::EndBatch();
}

} // namespace Frame2