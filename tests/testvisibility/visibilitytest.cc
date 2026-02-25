//------------------------------------------------------------------------------
// visibilitytest.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/refcounted.h"
#include "timing/timer.h"
#include "io/console.h"
#include "visibilitytest.h"
#include "app/application.h"
#include "input/inputserver.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "io/ioserver.h"
#include "graphics/cameracontext.h"
#include "graphics/view.h"

#include "graphics/graphicsserver.h"
#include "resources/resourceserver.h"

#include "visibility/visibilitycontext.h"
#include "models/modelcontext.h"
#include "clustering/clustercontext.h"
#include "lighting/lightcontext.h"
#include "characters/charactercontext.h"
#include "dynui/im3d/im3dcontext.h"

#include "renderutil/mayacamerautil.h"
#include "debug/debuginterface.h"
#include "profiling/profiling.h"

#include "dynui/imguicontext.h"
#include "imgui.h"

#include "frame/default.h"

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
    Ptr<Resources::ResourceServer> resMgr = Resources::ResourceServer::Create();

    App::Application app;

    Ptr<Input::InputServer> inputServer = Input::InputServer::Create();
    Ptr<IO::IoServer> ioServer = IO::IoServer::Create();

    app.SetAppTitle("RenderTest!");
    app.SetCompanyName("gscept");
    app.Open();
    auto debugInterface = Debug::DebugInterface::Create();
    debugInterface->Open();
    Profiling::ProfilingRegisterThread();
    
    resMgr->Open();
    inputServer->Open();
    gfxServer->Open();

    CoreGraphics::WindowCreateInfo wndInfo =
    {
        CoreGraphics::DisplayMode{ 100, 100, 1024, 768 },
        "Render test!", "", CoreGraphics::AntiAliasQuality::None, nullptr, true, true, false
    };
    CoreGraphics::WindowId wnd = CreateMainWindow(wndInfo);

    Ptr<View> view = gfxServer->CreateView("mainview", FrameScript_default::Run, Math::rectangle<int>(0, 0, 1024, 768));
    Ptr<Stage> stage = gfxServer->CreateStage("stage1", true);

    // create contexts, this could and should be bundled together
    CameraContext::Create();
    ModelContext::Create();
    ObserverContext::Create();
    ObservableContext::Create();
    Clustering::ClusterContext::Create(0.1f, 1000.0f, wnd);
    Im3d::Im3dContext::Create();
    Dynui::ImguiContext::Create();
    Lighting::LightContext::Create();

    // setup camera and view
    GraphicsEntityId cam = Graphics::CreateEntity();
    CameraContext::RegisterEntity(cam);
    CameraContext::SetupProjectionFov(cam, 16.f / 9.f, Math::deg2rad(60.f), 0.01f, 1000.0f);
    view->SetCamera(cam);
    view->SetStage(stage);

    // setup scene
    GraphicsEntityId ent = Graphics::CreateEntity();
    
    // create model and move it to the front
    ModelContext::RegisterEntity(ent);
    ObservableContext::RegisterEntity(ent);
    ModelContext::Setup(ent, "mdl:Buildings/castle_tower.n3", "NotA", [ent]()
        {
            ObservableContext::Setup(ent, VisibilityEntityType::Model);
        });
    ModelContext::SetTransform(ent, Math::translation(Math::vector(0, 0, 10)));

    // setup scene
    GraphicsEntityId ent2 = Graphics::CreateEntity();

    // create model and move it to the front
    ModelContext::RegisterEntity(ent2);
    ObservableContext::RegisterEntity(ent2);
    ModelContext::Setup(ent2, "mdl:Buildings/castle_tower.n3", "NotA", [ent2]()
        {
            ObservableContext::Setup(ent2, VisibilityEntityType::Model);
        });
    ModelContext::SetTransform(ent2, Math::translation(Math::vector(0, 0, -5)));

    GraphicsEntityId globalLight = Graphics::CreateEntity();
    Lighting::LightContext::RegisterEntity(globalLight);
    Lighting::LightContext::SetupGlobalLight(globalLight, Math::vec3(1, 1, 1), 1.0f, Math::vec3(0, 0, 0), 85_rad, 0_rad, true);

    // register visibility system
    ObserverContext::CreateBruteforceSystem({});

    // now add both to visibility
    ObserverContext::RegisterEntity(cam);
    ObserverContext::Setup(cam, VisibilityEntityType::Camera);

    Util::Array<Graphics::GraphicsEntityId> models;
    ModelContext::BeginBulkRegister();
    ObservableContext::BeginBulkRegister();
    static const int NumModels = 10;
    for (IndexT i = -NumModels; i < NumModels; i++)
    {
        for (IndexT j = -NumModels; j < NumModels; j++)
        {
            Graphics::GraphicsEntityId ent = Graphics::CreateEntity();

            // create model and move it to the front
            ModelContext::RegisterEntity(ent);
            ObservableContext::RegisterEntity(ent);
            ModelContext::Setup(ent, "mdl:Buildings/castle_tower.n3", "NotA", [ent]()
                {
                    ObservableContext::Setup(ent, VisibilityEntityType::Model);
                });
            ModelContext::SetTransform(ent, translation(Math::vector(i*10, 0, -j*10)));

            models.Append(ent);
        }
    }
    ModelContext::EndBulkRegister();
    ObservableContext::EndBulkRegister();

    RenderUtil::MayaCameraUtil mayaCamera;
    mayaCamera.Setup(Math::point(0, 0, 0), Math::point(0, 0, 10), Math::vector::upvec());
    const Ptr<Input::Keyboard>& kdb = inputServer->GetDefaultKeyboard();
    const Ptr<Input::Mouse>& mouse = inputServer->GetDefaultMouse();

    Math::vec2 panning(0.0f, 0.0f);
    float zoomIn = 0.0f;
    float zoomOut = 0.0f;

    Timer timer;
    IndexT frameIndex = -1;
    double previousTime = 0.0f;
    bool run = true;
    while (run && !inputServer->IsQuitRequested())
    {
        using namespace Input;

        timer.Reset();
        timer.Start();

        resMgr->Update(frameIndex);
        CameraContext::SetView(cam, Math::inverse(mayaCamera.GetCameraTransform()));

        inputServer->BeginFrame();
        inputServer->OnFrame();

        gfxServer->RunPreLogic();
        
        ImGui::Begin("VisibilityTest", NULL);
        ImGui::Text("FPS: %.2f", 1 / (previousTime /1000.0f));
        ImGui::End();
        // put game code which doesn't need visibility data or animation here

        gfxServer->RunPostLogic();

        // put game code which need visibility data here

        gfxServer->Render();

        // do stuff after rendering is done

        gfxServer->EndFrame();
        
        // force wait immediately
        WindowPresent(wnd, frameIndex);
        if (kdb->KeyPressed(Input::Key::Escape)) run = false;

        gfxServer->NewFrame();

        // standard input handling: manipulate camera
        mayaCamera.SetOrbitButton(mouse->ButtonPressed(MouseButton::LeftButton));
        mayaCamera.SetPanButton(mouse->ButtonPressed(MouseButton::MiddleButton));
        mayaCamera.SetZoomButton(mouse->ButtonPressed(MouseButton::RightButton));
        mayaCamera.SetZoomInButton(mouse->WheelForward());
        mayaCamera.SetZoomOutButton(mouse->WheelBackward());

        if (kdb->KeyPressed(Input::Key::LeftMenu))
        {
            mayaCamera.SetMouseMovement(mouse->GetMovement());
        }

        // handle keyboard input
        if (kdb->KeyDown(Key::Space))
        {
            mayaCamera.Reset();
        }
        if (kdb->KeyPressed(Key::Right))
        {
            panning.x -= 0.1f;
        }
        if (kdb->KeyPressed(Key::Left))
        {
            panning.x += 0.1f;
        }
        if (kdb->KeyPressed(Key::Up))
        {
            panning.y += 0.1f;
        }
        if (kdb->KeyPressed(Key::Down))
        {
            panning.y -= 0.1f;
        }

        // update panning, orbiting, zooming speeds
        mayaCamera.SetPanning(panning);
        mayaCamera.SetOrbiting(Math::vec2(0.0f));

        mayaCamera.SetZoomIn(0.0f);
        mayaCamera.SetZoomOut(0.0f);
        mayaCamera.Update(0.01667f);

        inputServer->EndFrame();
        frameIndex++;

        timer.Stop();
        previousTime = timer.GetTime() * 1000;
        n_printf("Frame took %f ms\n", previousTime);
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