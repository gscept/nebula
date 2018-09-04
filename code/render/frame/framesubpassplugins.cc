//------------------------------------------------------------------------------
// framesubpassplugins.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framesubpassplugins.h"
#include "coregraphics/graphicsdevice.h"
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

	//this->pluginRegistry = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameSubpassPlugins::AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator)
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
	//n_assert(!this->pluginRegistry.isvalid());
	//this->pluginRegistry = RenderModules::RTPluginRegistry::Instance();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassPlugins::CompiledImpl::Run(const IndexT frameIndex)
{
	CoreGraphics::BeginBatch(FrameBatchType::System);
	//this->pluginRegistry->OnRender(this->pluginFilter);
	CoreGraphics::EndBatch();
}

} // namespace Frame2