//------------------------------------------------------------------------------
// viewerapp.cc
// (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "viewerapp.h"

#include "core/refcounted.h"
#include "system/systeminfo.h"
#include "timing/timer.h"
#include "io/console.h"
#include "io/logfileconsolehandler.h"

#include "dynui/imguicontext.h"
#include "dynui/im3d/im3dcontext.h"
#include "dynui/im3d/im3d.h"

#include "graphics/globalconstants.h"
#include "visibility/visibilitycontext.h"
#include "models/modelcontext.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "lighting/lightcontext.h"
#include "characters/charactercontext.h"
#include "decals/decalcontext.h"

#include "jobs2/jobs2.h"

#include "graphics/environmentcontext.h"
#include "fog/volumetricfogcontext.h"
#include "clustering/clustercontext.h"
#include "scenes/scenes.h"
#include "debug/framescriptinspector.h"

#include "posteffects/bloomcontext.h"
#include "posteffects/ssaocontext.h"
#include "posteffects/ssrcontext.h"
#include "posteffects/histogramcontext.h"
#include "posteffects/downsamplingcontext.h"

#include "physicsinterface.h"
#include "physics/debugui.h"

#include "terrain/terraincontext.h"
#include "vegetation/vegetationcontext.h"

#include "imgui.h"

using namespace Timing;
using namespace Graphics;
using namespace Visibility;
using namespace Models;

namespace Tests
{

__ImplementSingleton(Tests::SimpleViewerApplication);
//------------------------------------------------------------------------------
/**
*/
const char* stateToString(Resources::Resource::State state)
{
    switch (state)
    {
    case Resources::Resource::State::Pending: return "Pending";
    case Resources::Resource::State::Loaded: return "Loaded";
    case Resources::Resource::State::Failed: return "Failed";
    case Resources::Resource::State::Unloaded: return "Unloaded";
    }
    return "Unknown";
}

//------------------------------------------------------------------------------
/**
*/
SimpleViewerApplication::SimpleViewerApplication()
{
    __ConstructSingleton;
    this->SetAppTitle("Viewer App");
    this->SetCompanyName("Nebula");
}

//------------------------------------------------------------------------------
/**
*/
SimpleViewerApplication::~SimpleViewerApplication()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool 
SimpleViewerApplication::Open()
{
    if (Application::Open())
    {
#if __NEBULA_HTTP__

        // setup debug subsystem
        this->debugInterface = Debug::DebugInterface::Create();
        this->debugInterface->Open();
#endif
        this->gfxServer = GraphicsServer::Create();
        this->resMgr = Resources::ResourceServer::Create();
        this->inputServer = Input::InputServer::Create();
        this->ioServer = IO::IoServer::Create();

#ifdef USE_GITHUB_DEMO        
        this->ioServer->MountArchive("root:export");
#endif

#if NEBULA_ENABLE_PROFILING
        Profiling::ProfilingRegisterThread();
#endif
        
#if __WIN32__
        //Ptr<IO::LogFileConsoleHandler> logFileHandler = IO::LogFileConsoleHandler::Create();
        //IO::Console::Instance()->AttachHandler(logFileHandler.upcast<IO::ConsoleHandler>());
#endif

        this->resMgr->Open();
        this->inputServer->Open();
        this->gfxServer->Open();

        const System::SystemInfo* systemInfo = Core::SysFunc::GetSystemInfo();

        Jobs2::JobSystemInitInfo jobSystemInfo;
        jobSystemInfo.numThreads = 8;
        jobSystemInfo.name = "JobSystem";
        jobSystemInfo.scratchMemorySize = 8_MB;
        Jobs2::JobSystemInit(jobSystemInfo);

        SizeT width = this->GetCmdLineArgs().GetInt("-w", 1280);
        SizeT height = this->GetCmdLineArgs().GetInt("-h", 1024);

        CoreGraphics::WindowCreateInfo wndInfo =
        {
            CoreGraphics::DisplayMode{ 100, 100, width, height },
            this->GetAppTitle(), "", CoreGraphics::AntiAliasQuality::None, true, true, false, false
        };
        this->wnd = CreateWindow(wndInfo);
        this->cam = Graphics::CreateEntity();

        this->view = gfxServer->CreateView("mainview", "frame:vkdefault.json"_uri);
        gfxServer->SetCurrentView(this->view);
        this->stage = gfxServer->CreateStage("stage1", true);

        // setup post effects
        Ptr<Frame::FrameScript> frameScript = this->view->GetFrameScript();

        // Create contexts, this could and should be bundled together
        CameraContext::Create();
        ModelContext::Create();
        Characters::CharacterContext::Create();
        Particles::ParticleContext::Create();

        // Setup visibility related contexts
        // The order is important, ObserverContext is dependent on any bounding box and renderable modifying code
        ObserverContext::Create();
        ObservableContext::Create();

        Graphics::RegisterEntity<CameraContext, ObserverContext>(this->cam);
        CameraContext::SetupProjectionFov(this->cam, width / (float)height, Math::deg2rad(60.f), 0.1f, 10000.0f);
        CameraContext::SetLODCamera(this->cam);

        Dynui::ImguiContext::Create();

        //Terrain::TerrainSetupSettings terSettings{
        //    0, 1024.0f,      // min/max height 
        //    //0, 0,
        //    8192, 8192,   // world size in meters
        //    256, 256,     // tile size in meters
        //    16, 16        // 1 vertex every X meters
        //};
        //Terrain::TerrainContext::Create(terSettings);

        // setup vegetation
        //Vegetation::VegetationSetupSettings vegSettings{
        //    0, 1024.0f,      // min/max height 
        //    {8192, 8192}     // world size
        //};
        //Vegetation::VegetationContext::Create(vegSettings);

        Clustering::ClusterContext::Create(0.1f, 10000.0f, this->wnd);
        Lighting::LightContext::Create(frameScript);
        Decals::DecalContext::Create();
        Im3d::Im3dContext::Create();
        Fog::VolumetricFogContext::Create(frameScript);
        PostEffects::BloomContext::Create();
        PostEffects::SSAOContext::Create();
        PostEffects::HistogramContext::Create();
        PostEffects::DownsamplingContext::Create();
        //PostEffects::SSRContext::Create();

        // setup gbuffer bindings after frame script is loaded
        Graphics::SetupBufferConstants(frameScript);
        PostEffects::BloomContext::Setup(frameScript);
        PostEffects::SSAOContext::Setup(frameScript);
        //PostEffects::SSRContext::Setup(frameScript);
        PostEffects::DownsamplingContext::Setup(frameScript);
        PostEffects::HistogramContext::Setup(frameScript);
        PostEffects::HistogramContext::SetWindow({ 0.0f, 0.0f }, { 1.0f, 1.0f }, 1);

        Im3d::Im3dContext::SetGridStatus(this->showGrid);
        Im3d::Im3dContext::SetGridSize(1.0f, 25);
        Im3d::Im3dContext::SetGridColor(Math::vec4(0.2f, 0.2f, 0.2f, 0.8f));

        this->globalLight = Graphics::CreateEntity();
        Lighting::LightContext::RegisterEntity(this->globalLight);
        Lighting::LightContext::SetupGlobalLight(
            this->globalLight,
            Math::vec3(1, 1, 1),
            10.0f,
            Math::vec3(0, 0, 0),
            Math::vec3(0, 0, 0),
            0.0f,
            60_rad,
            0_rad,
            true
        );

        this->ResetCamera();
        CameraContext::SetView(this->cam, this->mayaCameraUtil.GetCameraTransform());

        this->view->SetCamera(this->cam);
        this->view->SetStage(this->stage);

        // register visibility system
        ObserverContext::CreateBruteforceSystem({});

        ObserverContext::Setup(this->cam, VisibilityEntityType::Camera);

        // create environment context for the atmosphere effects
        EnvironmentContext::Create(this->globalLight);

        this->UpdateCamera();

        this->frametimeHistory.Fill(0, 120, 0.0f);

        Physics::Setup();

        Util::FixedArray<Graphics::ViewIndependentCall> preLogicCalls = 
        {
            Im3d::Im3dContext::NewFrame,
            Dynui::ImguiContext::NewFrame,
            CameraContext::UpdateCameras,
            ModelContext::UpdateTransforms,
            Characters::CharacterContext::UpdateAnimations,
            Fog::VolumetricFogContext::RenderUI,
            EnvironmentContext::OnBeforeFrame,
            EnvironmentContext::RenderUI,
            Particles::ParticleContext::UpdateParticles,
            //Terrain::TerrainContext::RenderUI
        };

        Util::FixedArray<Graphics::ViewDependentCall> preLogicViewCalls =
        {
            Lighting::LightContext::OnPrepareView,
            Particles::ParticleContext::OnPrepareView,
            Im3d::Im3dContext::OnPrepareView,
            PostEffects::SSAOContext::UpdateViewDependentResources,
            PostEffects::HistogramContext::UpdateViewResources,
            Decals::DecalContext::UpdateViewDependentResources,
            Fog::VolumetricFogContext::UpdateViewDependentResources,
            Lighting::LightContext::UpdateViewDependentResources,
            //Terrain::TerrainContext::CullPatches
        };

        Util::FixedArray<Graphics::ViewIndependentCall> postLogicCalls = 
        {
            Clustering::ClusterContext::UpdateResources,
            ObserverContext::RunVisibilityTests,
            ObserverContext::GenerateDrawLists,

            // At the very latest point, wait for work to finish
            Dynui::ImguiContext::Render,
            ModelContext::WaitForWork,
            Characters::CharacterContext::WaitForCharacterJobs,
            Particles::ParticleContext::WaitForParticleUpdates,
            ObserverContext::WaitForVisibility,
        };

        Util::FixedArray<Graphics::ViewDependentCall> postLogicViewCalls = 
        {

            //Terrain::TerrainContext::UpdateLOD,
            //Vegetation::VegetationContext::UpdateViewResources
        };

        this->gfxServer->SetupPreLogicCalls(preLogicCalls);
        this->gfxServer->SetupPreLogicViewCalls(preLogicViewCalls);
        this->gfxServer->SetupPostLogicCalls(postLogicCalls);
        this->gfxServer->SetupPostLogicViewCalls(postLogicViewCalls);

        this->console = Dynui::ImguiConsole::Create();
        this->consoleHandler = Dynui::ImguiConsoleHandler::Create();
        this->console->Setup();
        this->consoleHandler->Setup();
        this->profiler = Dynui::ImguiProfiler::Create();

        // Before starting the frame, build the frame script
        this->view->BuildFrameScript();

        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void 
SimpleViewerApplication::Close()
{
    Physics::ShutDown();
    Dynui::ImguiContext::Discard();
    Im3d::Im3dContext::Discard();
    Jobs2::JobSystemUninit();
    App::Application::Close();
    DestroyWindow(this->wnd);
    this->gfxServer->DiscardStage(this->stage);
    this->gfxServer->DiscardView(this->view);
    ObserverContext::Discard();
    Lighting::LightContext::Discard();
    Decals::DecalContext::Discard();
    Fog::VolumetricFogContext::Discard();

    this->gfxServer->Close();
    this->inputServer->Close();
    this->resMgr->Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
SimpleViewerApplication::Run()
{    
    bool run = true;

    const Ptr<Input::Keyboard>& keyboard = inputServer->GetDefaultKeyboard();
    const Ptr<Input::Mouse>& mouse = inputServer->GetDefaultMouse();
    
    scenes[currentScene]->Open();

    while (run && !inputServer->IsQuitRequested())
    {
        /*
        Math::mat4 globalLightTransform = Lighting::LightContext::GetTransform(globalLight);
        Math::mat4 rotY = Math::rotationy(Math::deg2rad(0.1f));
        Math::mat4 rotX = Math::rotationz(Math::deg2rad(0.05f));
        globalLightTransform = globalLightTransform * rotX;
        Lighting::LightContext::SetTransform(globalLight, globalLightTransform);
        */

        Jobs2::JobNewFrame();

#if NEBULA_ENABLE_PROFILING
        this->profiler->Capture();
        Profiling::ProfilingNewFrame();
#endif

        N_MARKER_BEGIN(Input, App);
        this->inputServer->BeginFrame();
        CoreGraphics::WindowPollEvents();
        this->inputServer->OnFrame();
        N_MARKER_END();
        this->resMgr->Update(this->frameIndex);

        if (keyboard->KeyPressed(Input::Key::Escape)) run = false;

        if (keyboard->KeyPressed(Input::Key::LeftMenu) && this->cameraMode == 0
            || this->cameraMode == 1)
            this->UpdateCamera();

        if (keyboard->KeyPressed(Input::Key::F8))
            Terrain::TerrainContext::ClearCache();

        if (keyboard->KeyDown(Input::Key::F3))
            this->profiler->TogglePause();

        // Run pre-game logic render code
        this->gfxServer->RunPreLogic();

        this->RenderUI();

        if (this->renderDebug)
        {
            this->gfxServer->RenderDebug(0);
        }

        scenes[currentScene]->Run();

        // Run post-logic render code
        this->gfxServer->RunPostLogic();

        // Render views
        this->gfxServer->Render();

        // Finish frame
        this->gfxServer->EndFrame();

        // force wait immediately
        N_MARKER_BEGIN(Present, App);
        WindowPresent(wnd, frameIndex);
        N_MARKER_END();

        // Begin a new frame
        this->gfxServer->NewFrame();


        frameIndex++;             
        this->inputServer->EndFrame();
        
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SimpleViewerApplication::RenderUI()
{
    N_SCOPE(UpdateUI, App);

    this->console->Render();
    
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, { 0,0,0,0.15f });
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Scenes"))
        {
            int i = 0;
            for (auto scene : scenes)
            {
                bool isSelected = (i == currentScene);
                if (ImGui::MenuItem(scene->name, nullptr, &isSelected))
                {
                    if (i != currentScene)
                    {
                        scenes[currentScene]->Close();
                        currentScene = i;
                        scenes[currentScene]->Open();
                    }
                }
                i++;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Windows"))
        {
            ImGui::MenuItem("Camera Window", nullptr, &this->showCameraWindow);
            ImGui::MenuItem("Frame Profiler", nullptr, &this->showFrameProfiler);
            ImGui::MenuItem("Scene UI", nullptr, &this->showSceneUI);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Settings"))
        {
            ImGui::MenuItem("Show Grid", nullptr, &this->showGrid);
            Im3d::Im3dContext::SetGridStatus(this->showGrid);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    ImGui::PopStyleColor();

    this->profiler->Render(this->gfxServer->GetFrameTime(), this->gfxServer->GetFrameIndex());

    if (this->showCameraWindow)
    {
        if (ImGui::Begin("Viewer", &showCameraWindow, 0))
        {
            ImGui::SetWindowSize(ImVec2(240, 400), ImGuiCond_Once);
            if (ImGui::CollapsingHeader("Camera mode", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::RadioButton("Maya", &this->cameraMode, 0))this->ToMaya();
                ImGui::SameLine();
                if (ImGui::RadioButton("Free", &this->cameraMode, 1))this->ToFree();
                ImGui::SameLine();
                if (ImGui::Button("Reset")) this->ResetCamera();
            }
            ImGui::Checkbox("Debug Rendering", &this->renderDebug);
        }       

        ImGui::End();
    }

    if (this->showSceneUI)
    {
        scenes[currentScene]->RenderUI();
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SimpleViewerApplication::UpdateCamera()
{
    const Ptr<Input::Keyboard>& keyboard = inputServer->GetDefaultKeyboard();
    const Ptr<Input::Mouse>& mouse = inputServer->GetDefaultMouse();

    this->mayaCameraUtil.SetOrbitButton(mouse->ButtonPressed(Input::MouseButton::LeftButton));
    this->mayaCameraUtil.SetPanButton(mouse->ButtonPressed(Input::MouseButton::MiddleButton));
    this->mayaCameraUtil.SetZoomButton(mouse->ButtonPressed(Input::MouseButton::RightButton));
    this->mayaCameraUtil.SetZoomInButton(mouse->WheelForward());
    this->mayaCameraUtil.SetZoomOutButton(mouse->WheelBackward());
    this->mayaCameraUtil.SetMouseMovement(mouse->GetMovement());

    // process keyboard input
    Math::vec3 pos(0.0f);
    if (keyboard->KeyDown(Input::Key::Space))
    {
        this->mayaCameraUtil.Reset();
    }
    if (keyboard->KeyPressed(Input::Key::Left))
    {
        panning.x -= 0.1f;
        pos.x -= 0.1f;
    }
    if (keyboard->KeyPressed(Input::Key::Right))
    {
        panning.x += 0.1f;
        pos.x += 0.1f;
    }
    if (keyboard->KeyPressed(Input::Key::Up))
    {
        panning.y -= 0.1f;
        if (keyboard->KeyPressed(Input::Key::LeftShift))
        {
            pos.y -= 0.1f;
        }
        else
        {
            pos.z -= 0.1f;
        }
    }
    if (keyboard->KeyPressed(Input::Key::Down))
    {
        panning.y += 0.1f;
        if (keyboard->KeyPressed(Input::Key::LeftShift))
        {
            pos.y += 0.1f;
        }
        else
        {
            pos.z += 0.1f;
        }
    }

    this->mayaCameraUtil.SetPanning(panning);
    this->mayaCameraUtil.SetOrbiting(orbiting);
    this->mayaCameraUtil.SetZoomIn(zoomIn);
    this->mayaCameraUtil.SetZoomOut(zoomOut);
    this->mayaCameraUtil.Update();
    
    this->freeCamUtil.SetForwardsKey(keyboard->KeyPressed(Input::Key::W));
    this->freeCamUtil.SetBackwardsKey(keyboard->KeyPressed(Input::Key::S));
    this->freeCamUtil.SetRightStrafeKey(keyboard->KeyPressed(Input::Key::D));
    this->freeCamUtil.SetLeftStrafeKey(keyboard->KeyPressed(Input::Key::A));
    this->freeCamUtil.SetUpKey(keyboard->KeyPressed(Input::Key::Q));
    this->freeCamUtil.SetDownKey(keyboard->KeyPressed(Input::Key::E));

    this->freeCamUtil.SetMouseMovement(mouse->GetMovement());
    this->freeCamUtil.SetAccelerateButton(keyboard->KeyPressed(Input::Key::LeftShift));

    this->freeCamUtil.SetRotateButton(mouse->ButtonPressed(Input::MouseButton::LeftButton));
    this->freeCamUtil.Update();
    
    switch (this->cameraMode)
    {
    case 0:
        CameraContext::SetView(this->cam, Math::inverse(this->mayaCameraUtil.GetCameraTransform()));
        break;
    case 1:
        CameraContext::SetView(this->cam, Math::inverse(this->freeCamUtil.GetTransform()));
        break;
    default:
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SimpleViewerApplication::ResetCamera()
{
    this->freeCamUtil.Setup(this->defaultViewPoint, Math::normalize(this->defaultViewPoint));
    this->freeCamUtil.Update();
    this->mayaCameraUtil.Setup(Math::vec3(0.0f, 0.0f, 0.0f), this->defaultViewPoint, Math::vec3(0.0f, 1.0f, 0.0f));
}

//------------------------------------------------------------------------------
/**
*/
void 
SimpleViewerApplication::ToMaya()
{
    this->mayaCameraUtil.Setup(this->mayaCameraUtil.GetCenterOfInterest(), xyz(this->freeCamUtil.GetTransform().position), Math::vec3(0, 1, 0));
}

//------------------------------------------------------------------------------
/**
*/
void 
SimpleViewerApplication::ToFree()
{
    Math::vec3 pos = xyz(this->mayaCameraUtil.GetCameraTransform().position);
    this->freeCamUtil.Setup(pos, Math::normalize(pos - this->mayaCameraUtil.GetCenterOfInterest()));
}

//------------------------------------------------------------------------------
/**
*/
void 
SimpleViewerApplication::Browse()
{
    this->folders = IO::IoServer::Instance()->ListDirectories("mdl:", "*");    
    this->files = IO::IoServer::Instance()->ListFiles("mdl:" + this->folders[this->selectedFolder], "*");
}

} // namespace Tests
