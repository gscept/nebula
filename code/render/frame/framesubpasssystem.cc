//------------------------------------------------------------------------------
// framesubpasssystem.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "framesubpasssystem.h"
#include "lighting/lightserver.h"
#include "rendermodules/rt/rtpluginregistry.h"
#include "coregraphics/textrenderer.h"
#include "coregraphics/shaperenderer.h"
#include "lighting/shadowserver.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/framebatchtype.h"

using namespace CoreGraphics;
using namespace Lighting;
namespace Frame2
{

__ImplementClass(Frame2::FrameSubpassSystem, 'FRSS', Frame2::FrameOp);
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
void
FrameSubpassSystem::Setup()
{

}

//------------------------------------------------------------------------------
/**
	Perhaps this should also launch RT plugins, but RT plugins really need to 
	be manageable as to where they should be rendered.
*/
void
FrameSubpassSystem::Run(const IndexT frameIndex)
{
	CoreGraphics::RenderDevice* renderDev = CoreGraphics::RenderDevice::Instance();
	switch (this->call)
	{
	case Lights:
		renderDev->BeginBatch(FrameBatchType::System);
		LightServer::Instance()->RenderLights();
		renderDev->EndBatch();
		break;
	case LightProbes:
		renderDev->BeginBatch(FrameBatchType::System);
		LightServer::Instance()->RenderLightProbes();
		renderDev->EndBatch();
		break;
	case LocalShadowsSpot:	// shadows implement their own batching
		ShadowServer::Instance()->UpdateSpotLightShadowBuffers();
		break;
	case LocalShadowsPoint:
		ShadowServer::Instance()->UpdatePointLightShadowBuffers();
		break;
	case GlobalShadows:
		ShadowServer::Instance()->UpdateGlobalLightShadowBuffers();
		break;
	case UI:
		//RenderModules::RTPluginRegistry::Instance()->OnRender(FrameBatchType::UI);
		break;
	case Text:
		renderDev->BeginBatch(FrameBatchType::System);
		TextRenderer::Instance()->DrawTextElements();
		renderDev->EndBatch();
		break;
	case Shapes:
		renderDev->BeginBatch(FrameBatchType::System);
		ShapeRenderer::Instance()->DrawShapes();
		renderDev->EndBatch();
		break;
	default:
		n_error("Invalid system call!");
	}
}

} // namespace Frame2