//------------------------------------------------------------------------------
// viewerapp.cc
// (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "viewerapp.h"

#include "core/refcounted.h"
#include "timing/timer.h"
#include "io/console.h"
#include "io/logfileconsolehandler.h"

#include "dynui/imguicontext.h"
#include "dynui/im3d/im3dcontext.h"
#include "dynui/im3d/im3d.h"

#include "visibility/visibilitycontext.h"
#include "models/streammodelcache.h"
#include "models/modelcontext.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "lighting/lightcontext.h"
#include "characters/charactercontext.h"
#include "decals/decalcontext.h"

#include "graphics/environmentcontext.h"
#include "fog/volumetricfogcontext.h"
#include "clustering/clustercontext.h"
#include "scenes/scenes.h"
#include "debug/framescriptinspector.h"

#include "posteffects/bloomcontext.h"
#include "posteffects/ssaocontext.h"
#include "posteffects/ssrcontext.h"
#include "posteffects/histogramcontext.h"

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
    : pauseProfiling(false)
    , profileFixedFps(false)
    , fixedFps(60)
{
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

        SizeT width = this->GetCmdLineArgs().GetInt("-w", 1280);
        SizeT height = this->GetCmdLineArgs().GetInt("-h", 1024);

        CoreGraphics::WindowCreateInfo wndInfo =
        {
            CoreGraphics::DisplayMode{ 100, 100, width, height },
            this->GetAppTitle(), "", CoreGraphics::AntiAliasQuality::None, true, true, false
        };
        this->wnd = CreateWindow(wndInfo);
        this->cam = Graphics::CreateEntity();

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
        Vegetation::VegetationSetupSettings vegSettings{
            "tex:terrain/everest Height Map (Merged)_PNG_BC4_1.dds",
            0, 1024.0f,      // min/max height 
            Math::uint2{8192, 8192}, 3, 0.5f
        };
        Vegetation::VegetationContext::Create(vegSettings);

        Clustering::ClusterContext::Create(0.1f, 1000.0f, this->wnd);
        Lighting::LightContext::Create();
        Decals::DecalContext::Create();
        Im3d::Im3dContext::Create();
        Fog::VolumetricFogContext::Create();
        PostEffects::BloomContext::Create();
        PostEffects::SSAOContext::Create();
        PostEffects::HistogramContext::Create();
        //PostEffects::SSRContext::Create();


        this->view = gfxServer->CreateView("mainview", "frame:vkdefault.json"_uri);
        gfxServer->SetCurrentView(this->view);
        this->stage = gfxServer->CreateStage("stage1", true);

        // setup post effects
        Ptr<Frame::FrameScript> frameScript = this->view->GetFrameScript();
        // setup gbuffer bindings after frame script is loaded
        CoreGraphics::ShaderServer::Instance()->SetupBufferConstants(frameScript);
        PostEffects::BloomContext::Setup(frameScript);
        PostEffects::SSAOContext::Setup(frameScript);
        //PostEffects::SSRContext::Setup(frameScript);
        PostEffects::HistogramContext::Setup(frameScript);
        PostEffects::HistogramContext::SetWindow({ 0.0f, 0.0f }, { 1.0f, 1.0f }, 1);

        Im3d::Im3dContext::SetGridStatus(this->showGrid);
        Im3d::Im3dContext::SetGridSize(1.0f, 25);
        Im3d::Im3dContext::SetGridColor(Math::vec4(0.2f, 0.2f, 0.2f, 0.8f));

        this->globalLight = Graphics::CreateEntity();
        Lighting::LightContext::RegisterEntity(this->globalLight);
        Lighting::LightContext::SetupGlobalLight(this->globalLight, Math::vec3(1, 1, 1), 1000.0f, Math::vec3(0, 0, 0), Math::vec3(0, 0, 0), 0.0f, -Math::vector(0.1, 0.1, 0.1), true);

        this->ResetCamera();
        CameraContext::SetTransform(this->cam, this->mayaCameraUtil.GetCameraTransform());

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

        this->console = Dynui::ImguiConsole::Create();
        this->consoleHandler = Dynui::ImguiConsoleHandler::Create();
        this->console->Setup();
        this->consoleHandler->Setup();

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
#if NEBULA_ENABLE_PROFILING
        if (!this->pauseProfiling)
            this->profilingContexts = Profiling::ProfilingGetContexts();
        Profiling::ProfilingNewFrame();
#endif
        
        N_MARKER_BEGIN(Input, App);
        this->inputServer->BeginFrame();
        CoreGraphics::WindowPollEvents();
        this->inputServer->OnFrame();
        N_MARKER_END();
        CoreGraphics::WindowPollEvents();
        this->resMgr->Update(this->frameIndex);

#if NEBULA_ENABLE_PROFILING
        // copy because the information has been discarded when we render UI
        if (!this->pauseProfiling)
            this->frameProfilingMarkers = CoreGraphics::GetProfilingMarkers();
#endif NEBULA_ENABLE_PROFILING

        // Begin the next frame, waits for the previous frame with the same buffer index
        this->gfxServer->BeginFrame();
        
        this->RenderUI();

        if (this->renderDebug)
        {
            this->gfxServer->RenderDebug(0);
        }

        scenes[currentScene]->Run();

        // put game code which doesn't need visibility results or animation here
        this->gfxServer->BeforeViews();

        // put game code which need visibility data here
        this->gfxServer->RenderViews();

        // put game code which needs rendering to be done (animation etc) here
        this->gfxServer->EndViews();

        // do stuff after rendering is done
        this->gfxServer->EndFrame();

        // force wait immediately
        N_MARKER_BEGIN(Present, App);
        WindowPresent(wnd, frameIndex);
        N_MARKER_END();

        if (keyboard->KeyPressed(Input::Key::Escape)) run = false;
                
        if (keyboard->KeyPressed(Input::Key::LeftMenu) && this->cameraMode == 0
            || this->cameraMode == 1)
            this->UpdateCamera();
        
        if (keyboard->KeyPressed(Input::Key::F8))
            Terrain::TerrainContext::ClearCache();

        if (keyboard->KeyDown(Input::Key::F3))
            this->pauseProfiling = !this->pauseProfiling;

        frameIndex++;             
        this->inputServer->EndFrame();
        
    }
}

//------------------------------------------------------------------------------
/**
*/
int
RecursiveDrawScope(const Profiling::ProfilingScope& scope, ImDrawList* drawList, const ImVec2 start, const ImVec2 fullSize, ImVec2 pos, const ImVec2 canvas, const float frameTime, const int level)
{
    static const ImU32 colors[] =
    {
        IM_COL32(200, 50, 50, 255),
        IM_COL32(50, 200, 50, 255),
        IM_COL32(50, 50, 200, 255),
        IM_COL32(200, 50, 200, 255),
        IM_COL32(50, 200, 200, 255),
        IM_COL32(200, 200, 50, 255),
    };
    static const float YPad = ImGui::GetTextLineHeight();
    static const float TextPad = 5.0f;

    const uint32 numColors = sizeof(colors) / sizeof(ImU32);
    uint32 colorIndex = scope.category.HashCode() % numColors;

    // convert to milliseconds
    float startX = pos.x + scope.start / frameTime * canvas.x;
    float stopX = startX + Math::max(scope.duration / frameTime * canvas.x, 1.0);
    float startY = pos.y;
    float stopY = startY + YPad;

    ImVec2 bbMin = ImVec2(startX, startY);
    ImVec2 bbMax = ImVec2(Math::min(stopX, startX + fullSize.x), Math::min(stopY, startY + fullSize.y));

    // draw a filled rect for background, and normal rect for outline
    drawList->PushClipRect(bbMin, bbMax, true);
    drawList->AddRectFilled(bbMin, bbMax, colors[colorIndex], 0.0f);
    drawList->AddRect(bbMin, bbMax, IM_COL32(128, 128, 128, 128), 0.0f);

    // make sure text appears inside the box
    Util::String text = Util::String::Sprintf("%s (%4.4f ms)", scope.name, scope.duration * 1000);
    drawList->AddText(ImVec2(startX + TextPad, pos.y), IM_COL32_BLACK, text.AsCharPtr());
    drawList->PopClipRect();

    if (ImGui::IsMouseHoveringRect(bbMin, bbMax))
    {
        ImGui::BeginTooltip();
        Util::String text = Util::String::Sprintf("%s [%s] (%4.4f ms) in %s (%d)", scope.name, scope.category.Value(), scope.duration * 1000, scope.file, scope.line);
        ImGui::Text(text.AsCharPtr());
        ImGui::EndTooltip();
        ImVec2 l1 = bbMax;
        l1.y = start.y;
        ImVec2 l2 = bbMax;
        l2.y = fullSize.y;
        drawList->PushClipRect(start, fullSize);
        drawList->AddLine(l1, l2, IM_COL32(255, 0, 0, 255), 1.0f);
        drawList->PopClipRect();
    }

    // move next element down one level
    pos.y += YPad;
    int deepest = level + 1;
    for (IndexT i = 0; i < scope.children.Size(); i++)
    {
        int childLevel = RecursiveDrawScope(scope.children[i], drawList, start, fullSize, pos, canvas, frameTime, level + 1);
        deepest = Math::max(deepest, childLevel);
    }
    return deepest;
}

//------------------------------------------------------------------------------
/**
*/
int
RecursiveDrawGpuMarker(const CoreGraphics::FrameProfilingMarker& marker, ImDrawList* drawList, const ImVec2 start, const ImVec2 fullSize, ImVec2 pos, const ImVec2 canvas, const float frameTime, const int level)
{
    static const ImU32 colors[] =
    {
        IM_COL32(200, 50, 50, 255),
        IM_COL32(50, 200, 50, 255),
        IM_COL32(50, 50, 200, 255),
        IM_COL32(200, 50, 200, 255),
        IM_COL32(50, 200, 200, 255),
        IM_COL32(200, 200, 50, 255),
    };
    static const float YPad = ImGui::GetTextLineHeight();
    static const float TextPad = 5.0f;

    // convert to milliseconds
    float begin = marker.start / 1000000000.0f;
    float duration = marker.duration / 1000000000.0f;
    float startX = pos.x + begin / frameTime * canvas.x;
    float stopX = startX + Math::max(duration / frameTime * canvas.x, 1.0f);
    float startY = pos.y;
    float stopY = startY + YPad;

    ImVec2 bbMin = ImVec2(startX, startY);
    ImVec2 bbMax = ImVec2(Math::min(stopX, startX + fullSize.x), Math::min(stopY, startY + fullSize.y));

    // draw a filled rect for background, and normal rect for outline
    drawList->PushClipRect(bbMin, bbMax, true);
    drawList->AddRectFilled(bbMin, bbMax, colors[level % 6], 0.0f);
    drawList->AddRect(bbMin, bbMax, IM_COL32(128, 128, 128, 128), 0.0f);

    // make sure text appears inside the box
    Util::String text = Util::String::Sprintf("%s (%4.4f ms)", marker.name.AsCharPtr(), duration * 1000);
    drawList->AddText(ImVec2(startX + TextPad, startY), IM_COL32_BLACK, text.AsCharPtr());
    drawList->PopClipRect();


    if (ImGui::IsMouseHoveringRect(bbMin, bbMax))
    {
        ImGui::BeginTooltip();
        Util::String text = Util::String::Sprintf("%s (%4.4f ms)", marker.name.AsCharPtr(), duration * 1000);
        ImGui::Text(text.AsCharPtr());
        ImGui::EndTooltip();
    }

    // move next element down one level
    pos.y += YPad;
    int deepest = level + 1;
    for (IndexT i = 0; i < marker.children.Size(); i++)
    {
        int childLevel = RecursiveDrawGpuMarker(marker.children[i], drawList, start, fullSize, pos, canvas, frameTime, level + 1);
        deepest = Math::max(deepest, childLevel);
    }
    return deepest;
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

    if (!this->pauseProfiling)
    {
        this->currentFrameTime = (float)this->gfxServer->GetFrameTime();
        this->averageFrameTime += this->currentFrameTime;
        this->frametimeHistory.Append(this->currentFrameTime);
        if (this->frametimeHistory.Size() > 120)
            this->frametimeHistory.EraseFront();

        if (this->gfxServer->GetFrameIndex() % 35 == 0)
        {
            this->prevAverageFrameTime = this->averageFrameTime / 35.0f;
            this->averageFrameTime = 0.0f;
        }
    }

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

    if (this->showFrameProfiler)
    {
        //Debug::FrameScriptInspector::Run(this->view->GetFrameScript());
        if (ImGui::Begin("Performance Profiler", &this->showFrameProfiler))
        {
            ImGui::Text("ms - %.2f\nFPS - %.2f", this->prevAverageFrameTime * 1000, 1 / this->prevAverageFrameTime);
            ImGui::PlotLines("Frame Times", &this->frametimeHistory[0], this->frametimeHistory.Size(), 0, 0, FLT_MIN, FLT_MAX, { ImGui::GetContentRegionAvail().x, 90 });
            ImGui::Separator();
            ImGui::Checkbox("Fixed FPS", &this->profileFixedFps);
            if (this->profileFixedFps)
            {
                ImGui::InputInt("FPS", &this->fixedFps);
                this->currentFrameTime = 1 / float(this->fixedFps);
            }

#if NEBULA_ENABLE_PROFILING
            if (ImGui::CollapsingHeader("Profiler"))
            {

                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 start = ImGui::GetCursorScreenPos();
                ImVec2 fullSize = ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y);
                if (ImGui::CollapsingHeader("Timeline"))
                {
                    for (const Profiling::ProfilingContext& ctx : this->profilingContexts)
                    {
                        if (ImGui::CollapsingHeader(ctx.threadName.Value()))
                        {
                            ImGui::PushFont(Dynui::ImguiContext::state.smallFont);

                            ImVec2 canvasSize = ImGui::GetContentRegionAvail();
                            ImVec2 pos = ImGui::GetCursorScreenPos();
                            int levels = 0;
                            for (IndexT i = 0; i < ctx.topLevelScopes.Size(); i++)
                            {
                                const Profiling::ProfilingScope& scope = ctx.topLevelScopes[i];
                                int level = RecursiveDrawScope(scope, drawList, start, fullSize, pos, canvasSize, this->currentFrameTime, 0);
                                levels = Math::max(levels, level);
                            }

                            // set back cursor so we can draw our box
                            ImGui::SetCursorScreenPos(pos);
                            ImGui::InvisibleButton("canvas", ImVec2(canvasSize.x, Math::max(1.0f, levels * 20.0f)));
                            ImGui::PopFont();
                        }
                    }
                    if (ImGui::CollapsingHeader("GPU"))
                    {
                        ImGui::PushFont(Dynui::ImguiContext::state.smallFont);

                        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        const Util::Array<CoreGraphics::FrameProfilingMarker>& frameMarkers = this->frameProfilingMarkers;

                        // do graphics queue markers
                        drawList->AddText(ImVec2(pos.x, pos.y), IM_COL32_WHITE, "Graphics Queue");
                        pos.y += 20.0f;
                        int levels = 0;
                        for (int i = 0; i < frameMarkers.Size(); i++)
                        {
                            const CoreGraphics::FrameProfilingMarker& marker = frameMarkers[i];
                            if (marker.queue != CoreGraphics::GraphicsQueueType)
                                continue;
                            int level = RecursiveDrawGpuMarker(marker, drawList, start, fullSize, pos, canvasSize, this->currentFrameTime, 0);
                            levels = Math::max(levels, level);
                        }

                        // set back cursor so we can draw our box
                        ImGui::SetCursorScreenPos(pos);
                        ImGui::InvisibleButton("canvas", ImVec2(canvasSize.x, Math::max(1.0f, levels * 20.0f)));
                        pos.y += levels * 20.0f;

                        drawList->AddText(ImVec2(pos.x, pos.y), IM_COL32_WHITE, "Compute Queue");
                        pos.y += 20.0f;
                        levels = 0;
                        for (int i = 0; i < frameMarkers.Size(); i++)
                        {
                            const CoreGraphics::FrameProfilingMarker& marker = frameMarkers[i];
                            if (marker.queue != CoreGraphics::ComputeQueueType)
                                continue;
                            int level = RecursiveDrawGpuMarker(marker, drawList, start, fullSize, pos, canvasSize, this->currentFrameTime, 0);
                            levels = Math::max(levels, level);
                        }

                        // set back cursor so we can draw our box
                        ImGui::SetCursorScreenPos(pos);
                        ImGui::InvisibleButton("canvas", ImVec2(canvasSize.x, Math::max(1.0f, levels * 20.0f)));
                        ImGui::PopFont();
                    }
                }
                if (ImGui::CollapsingHeader("Memory"))
                {
                    ImGui::PushFont(Dynui::ImguiContext::state.smallFont);

                    Util::Dictionary<const char*, uint64> counters = Profiling::ProfilingGetCounters();
                    for (IndexT i = 0; i < counters.Size(); i++)
                    {
                        const char* name = counters.KeyAtIndex(i);
                        uint64 val = counters.ValueAtIndex(i);
                        if (val > 1_MB)
                            ImGui::LabelText(name, "%llu MB allocated", val / 1_MB);
                        else if (val > 1_KB)
                            ImGui::LabelText(name, "%llu KB allocated", val / 1_KB);
                        else
                            ImGui::LabelText(name, "%llu B allocated", val);
                    }

                    ImGui::PopFont();
                }
            }
#endif NEBULA_ENABLE_PROFILING
        }
        ImGui::End();
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
        CameraContext::SetTransform(this->cam, Math::inverse(this->mayaCameraUtil.GetCameraTransform()));
        break;
    case 1:
        CameraContext::SetTransform(this->cam, Math::inverse(this->freeCamUtil.GetTransform()));
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
