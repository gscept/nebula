//------------------------------------------------------------------------------
// visibilitytest.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/refcounted.h"
#include "memory/sliceallocatorpool.h"
#include "timing/timer.h"
#include "io/console.h"
#include "visibilitytest.h"
#include "app/application.h"
#include "input/inputserver.h"
#include "input/keyboard.h"
#include "io/ioserver.h"
#include "graphics/cameracontext.h"
#include "graphics/view.h"

#include "graphics/graphicsserver.h"
#include "resources/resourcemanager.h"

#include "visibility/visibilitycontext.h"
#include "models/modelcontext.h"

using namespace Timing;
using namespace Graphics;
using namespace Visibility;
using namespace Models;

namespace Test
{


__ImplementClass(VisibilityTest, 'RETE', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
void
VisibilityTest::Run()
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
		CoreGraphics::DisplayMode{ 100, 100, 640, 480 },
		"Render test!", "", CoreGraphics::AntiAliasQuality::None, true, true, false
	};
	CoreGraphics::WindowId wnd = CreateWindow(wndInfo);

	// create contexts, this could and should be bundled together
	CameraContext::Create();
	ModelContext::Create();
	ObserverContext::Create();
	ObservableContext::Create();
	
	Ptr<View> view = gfxServer->CreateView("mainview", "frame:vkdebug.json");
	Ptr<Stage> stage = gfxServer->CreateStage("stage1", true);

	// setup camera and view
	GraphicsEntityId cam = Graphics::CreateEntity();
	CameraContext::RegisterEntity(cam);
	CameraContext::SetupProjectionFov(cam, 16.f / 9.f, 90.f, 0.01f, 1000.0f);
	view->SetCamera(cam);
	view->SetStage(stage);

	// setup scene
	GraphicsEntityId ent = Graphics::CreateEntity();
	
	// create model and move it to the front
	ModelContext::RegisterEntity(ent);
	ModelContext::Setup(ent, "mdl:Buildings/castle_tower.n3", "NotA");
	ModelContext::SetTransform(ent, Math::matrix44::translation(Math::float4(0, 0, 10, 1)));

	// setup scene
	GraphicsEntityId ent2 = Graphics::CreateEntity();

	// create model and move it to the front
	ModelContext::RegisterEntity(ent2);
	ModelContext::Setup(ent2, "mdl:Buildings/castle_tower.n3", "NotA");
	ModelContext::SetTransform(ent2, Math::matrix44::translation(Math::float4(0, 0, -5, 1)));

	// register visibility system
	ObserverContext::CreateBruteforceSystem({});

	// now add both to visibility
	ObservableContext::RegisterEntity(ent);
	ObservableContext::Setup(ent, VisibilityEntityType::Model);
	ObservableContext::RegisterEntity(ent2);
	ObservableContext::Setup(ent2, VisibilityEntityType::Model);
	ObserverContext::RegisterEntity(cam);
	ObserverContext::Setup(cam, VisibilityEntityType::Camera);

	Util::Array<Graphics::GraphicsEntityId> models;
	static const int NumModels = 100;
	for (IndexT i = -NumModels; i < NumModels; i++)
	{
		for (IndexT j = -NumModels; j < NumModels; j++)
		{
			Graphics::GraphicsEntityId ent = Graphics::CreateEntity();

			// create model and move it to the front
			ModelContext::RegisterEntity(ent);
			ModelContext::Setup(ent, "mdl:Buildings/castle_tower.n3", "NotA");
			ModelContext::SetTransform(ent, Math::matrix44::translation(Math::float4(i, 0, -j, 1)));

			ObservableContext::RegisterEntity(ent);
			ObservableContext::Setup(ent, VisibilityEntityType::Model);
			models.Append(ent);
		}
	}

	Timer timer;
	IndexT frameIndex = -1;
	bool run = true;
	while (run && !inputServer->IsQuitRequested())
	{
		timer.Reset();
		timer.Start();
		inputServer->OnFrame();
		resMgr->Update(frameIndex);
		gfxServer->OnFrame();

		// force wait immediately
		//ObserverContext::WaitForVisibility();
		WindowPresent(wnd, frameIndex);
		if (inputServer->GetDefaultKeyboard()->KeyPressed(Input::Key::Escape)) run = false;
		frameIndex++;
		timer.Stop();
		n_printf("Frame took %f ms\n", timer.GetTime()*1000);
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