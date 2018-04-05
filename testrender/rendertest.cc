//------------------------------------------------------------------------------
// blockallocatortest.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/refcounted.h"
#include "memory/sliceallocatorpool.h"
#include "timing/timer.h"
#include "io/console.h"
#include "rendertest.h"
#include "graphics/graphicsserver.h"
#include "resources/resourcemanager.h"
#include "coregraphics/window.h"


using namespace Timing;
using namespace Graphics;
namespace Test
{


__ImplementClass(RenderTest, 'RETE', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
void
RenderTest::Run()
{
	GraphicsServer* gfxServer = GraphicsServer::Create();
	Resources::ResourceManager* resMgr = Resources::ResourceManager::Create();
	resMgr->Open();
	gfxServer->Open();

	CoreGraphics::WindowCreateInfo wndInfo = 
	{
		CoreGraphics::DisplayMode{0, 0, 640, 480},
		"Render test!", "", CoreGraphics::AntiAliasQuality::None, true, true, false
	};
	CoreGraphics::WindowId wnd = CreateWindow(wndInfo);
	GraphicsEntityId ent = Graphics::CreateEntity();
	

	gfxServer->Close();
	resMgr->Close();
}

} // namespace Test