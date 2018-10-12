//------------------------------------------------------------------------------
// framesubpasssystem.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framesubpasssystem.h"
#include "rendermodules/rt/rtpluginregistry.h"
#include "coregraphics/textrenderer.h"
#include "coregraphics/shaperenderer.h"
#include "coregraphics/graphicsdevice.h"
#include "frame/framebatchtype.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameSubpassSystem::FrameSubpassSystem()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameSubpassSystem::~FrameSubpassSystem()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameSubpassSystem::AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator)
{
	CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
	ret->call = this->call;
	return ret;
}

//------------------------------------------------------------------------------
/**
	Perhaps this should also launch RT plugins, but RT plugins really need to 
	be manageable as to where they should be rendered.
*/
void
FrameSubpassSystem::CompiledImpl::Run(const IndexT frameIndex)
{
	switch (this->call)
	{
	case Lights:
		CoreGraphics::BeginBatch(FrameBatchType::System);
		//LightServer::Instance()->RenderLights();
		CoreGraphics::EndBatch();
		break;
	case LightProbes:
		CoreGraphics::BeginBatch(FrameBatchType::System);
		//LightServer::Instance()->RenderLightProbes();
		CoreGraphics::EndBatch();
		break;
	case LocalShadowsSpot:	// shadows implement their own batching
		//ShadowServer::Instance()->UpdateSpotLightShadowBuffers();
		break;
	case LocalShadowsPoint:
		//ShadowServer::Instance()->UpdatePointLightShadowBuffers();
		break;
	case GlobalShadows:
		//ShadowServer::Instance()->UpdateGlobalLightShadowBuffers();
		break;
	case UI:
		//RenderModules::RTPluginRegistry::Instance()->OnRender(FrameBatchType::UI);
		break;
	case Text:
		CoreGraphics::BeginBatch(FrameBatchType::System);
		//TextRenderer::Instance()->DrawTextElements();
		CoreGraphics::EndBatch();
		break;
	case Shapes:
		CoreGraphics::BeginBatch(FrameBatchType::System);
		//ShapeRenderer::Instance()->DrawShapes();
		CoreGraphics::EndBatch();
		break;
	default:
		n_error("Invalid system call!");
	}
}

} // namespace Frame2