//------------------------------------------------------------------------------
// blockallocatortest.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/refcounted.h"
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
#include "graphics/view.h"
#include "graphics/cameracontext.h"

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

	Ptr<View> view = gfxServer->CreateView("mainview", "frame:vkdebug.json");
	Ptr<Stage> stage = gfxServer->CreateStage("stage1", true);
	
	GraphicsEntityId cam = Graphics::CreateEntity();
	CameraContext::RegisterEntity(cam);
	CameraContext::SetupProjectionFov(cam, 16.f / 9.f, Math::n_deg2rad(60.f), 1.0f, 1000.0f);
	view->SetCamera(cam);
	view->SetStage(stage);
	
	IndexT frameIndex = -1;
	bool run = true;
	while (run && !inputServer->IsQuitRequested())
	{
		inputServer->OnFrame();
		resMgr->Update(frameIndex);
		gfxServer->BeginFrame();
		WindowPresent(wnd, frameIndex);
		if (inputServer->GetDefaultKeyboard()->KeyPressed(Input::Key::Escape)) run = false;
		frameIndex++;
	}

	DestroyWindow(wnd);

	// clean up entities
	CameraContext::DeregisterEntity(cam);
	Graphics::DestroyEntity(cam);

	gfxServer->DiscardStage(stage);
	gfxServer->DiscardView(view);

	gfxServer->Close();
	inputServer->Close();
	resMgr->Close();
	app.Close();
}

} // namespace Test