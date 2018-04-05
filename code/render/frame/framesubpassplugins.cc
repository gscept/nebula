//------------------------------------------------------------------------------
// framesubpassplugins.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framesubpassplugins.h"
#include "coregraphics/renderdevice.h"
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
FrameSubpassPlugins::Run(const IndexT frameIndex)
{
	RenderDevice* renderDev = RenderDevice::Instance();
	renderDev->BeginBatch(FrameBatchType::System);
	//this->pluginRegistry->OnRender(this->pluginFilter);
	renderDev->EndBatch();
}

} // namespace Frame2