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
#include "app/application.h"
#include "input/inputserver.h"
#include "input/keyboard.h"
#include "io/ioserver.h"
#include "graphics/camera.h"
#include "graphics/view.h"

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
	Ptr<GraphicsServer> gfxServer = GraphicsServer::Create();
	Ptr<Resources::ResourceManager> resMgr = Resources::ResourceManager::Create();

	App::Application app;

	Ptr<Input::InputServer> inputServer = Input::InputServer::Create();
	Ptr<IO::IoServer> ioServer = IO::IoServer::Create();

	app.SetAppTitle("RenderTest!");
	app.SetCompanyName("gscept");
	app.Open();

	resMgr->Open();
	inputServer->Open();
	gfxServer->Open();

	CoreGraphics::WindowCreateInfo wndInfo = 
	{
		CoreGraphics::DisplayMode{100, 100, 640, 480},
		"Render test!", "", CoreGraphics::AntiAliasQuality::None, true, true, false
	};
	CoreGraphics::WindowId wnd = CreateWindow(wndInfo);
	GraphicsEntityId ent = Graphics::CreateEntity();

	Ptr<View> view = gfxServer->CreateView("mainview", "frame:vkdebug.json");
	Ptr<Stage> stage = gfxServer->CreateStage("stage1", true);
	CameraId cam = CreateCamera();
	view->SetCamera(cam);
	view->SetStage(stage);
	CameraSetProjectionFov(cam, 16.f / 9.f, 90.f, 0.01f, 1000.0f);
	
	IndexT frameIndex = -1;
	bool run = true;
	while (run && !inputServer->IsQuitRequested())
	{
		inputServer->OnFrame();
		resMgr->Update(frameIndex);
		gfxServer->OnFrame();
		WindowPresent(wnd, frameIndex);
		if (inputServer->GetDefaultKeyboard()->KeyPressed(Input::Key::Escape)) run = false;
		frameIndex++;
	}

	DestroyWindow(wnd);
	gfxServer->DiscardStage(stage);
	gfxServer->DiscardView(view);

	gfxServer->Close();
	inputServer->Close();
	resMgr->Close();
	app.Close();
}

} // namespace Test