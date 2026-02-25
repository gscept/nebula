//------------------------------------------------------------------------------
//  graphicsfeature/graphicsfeatureunit.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "lighting/lightcontext.h"
#include "models/modelcontext.h"
#include "graphics/cameracontext.h"
#include "visibility/visibilitycontext.h"
#include "dynui/imguicontext.h"
#include "tbui/tbuicontext.h"
#include "characters/charactercontext.h"
#include "dynui/im3d/im3dcontext.h"
#include "appgame/gameapplication.h"
#include "graphics/environmentcontext.h"
#include "clustering/clustercontext.h"
#include "fog/volumetricfogcontext.h"
#include "posteffects/bloomcontext.h"
#include "posteffects/ssaocontext.h"
#include "decals/decalcontext.h"
#include "debug/framescriptinspector.h"
#include "terrain/terraincontext.h"
#include "vegetation/vegetationcontext.h"
#include "posteffects/histogramcontext.h"
#include "posteffects/downsamplingcontext.h"
#include "particles/particlecontext.h"
#include "raytracing/raytracingcontext.h"

#include "graphics/globalconstants.h"

#include "graphicsfeature/managers/graphicsmanager.h"
#include "graphicsfeature/managers/cameramanager.h"

#include "nflatbuffer/nebula_flat.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "flat/options/levelsettings.h"
#include "components/camera.h"
#include "components/decal.h"
#include "components/lighting.h"
#include "components/model.h"
#include "components/terrain.h"

#include "scripting/deargui.h"

#include "terrain/terraincontext.h"

#include "frame/default.h"
#include "frame/shadows.h"
#include "gi/ddgicontext.h"
#if WITH_NEBULA_EDITOR
#include "frame/editorframe.h"
#endif

using namespace Graphics;
using namespace Visibility;
using namespace Models;
using namespace Particles;

namespace GraphicsFeature
{
__ImplementClass(GraphicsFeature::GraphicsFeatureUnit, 'FXFU', Game::FeatureUnit);
__ImplementSingleton(GraphicsFeatureUnit);

//------------------------------------------------------------------------------
/**
*/
GraphicsFeatureUnit::GraphicsFeatureUnit() :
    defaultFrameScript("frame:vkdefault.json"_uri)
    , title("GraphicsFeatureUnit")
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
GraphicsFeatureUnit::~GraphicsFeatureUnit()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnAttach()
{
    this->RegisterComponentType<PointLight>({.decay = true, .OnInit = &GraphicsManager::InitPointLight });
    this->RegisterComponentType<SpotLight>({.decay = true, .OnInit = &GraphicsManager::InitSpotLight });
    this->RegisterComponentType<AreaLight>({.decay = true, .OnInit = &GraphicsManager::InitAreaLight });
    this->RegisterComponentType<DDGIVolume>({.decay = true, .OnInit = &GraphicsManager::InitDDGIVolume});
    this->RegisterComponentType<Model>({.decay = true, .OnInit = &GraphicsManager::InitModel });
    this->RegisterComponentType<Decal>({.decay = true, .OnInit = &GraphicsManager::InitDecal});
    this->RegisterComponentType<Terrain>({ .decay = true, .OnInit = &GraphicsManager::InitTerrain });
    this->RegisterComponentType<Camera>();
    Scripting::RegisterDearguiModule();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnActivate()
{
    FeatureUnit::OnActivate();

    this->r_debug = Core::CVarCreate(Core::CVar_Int, "r_debug", "0", "Enable debugging rendering [0,2]");
    this->r_show_frame_inspector = Core::CVarCreate(Core::CVar_Int, "r_show_frame_inspector", "0", "Show the frame script inspector [0,1]");

    this->gfxServer = Graphics::GraphicsServer::Create();
    this->inputServer = Input::InputServer::Create();
    this->gfxServer->Open();
    this->inputServer->Open();
    Util::String contents;

    const IO::URI levelSetupPath("tbl:app/base_level.nlst");
    App::LevelSettingsT levelSettings;
    if (IO::IoServer::Instance()->ReadFile(levelSetupPath, contents))
    {
        Flat::FlatbufferInterface::DeserializeFlatbuffer<App::LevelSettings>(levelSettings, (byte*)contents.AsCharPtr());
    }
    auto& terrainSettings = levelSettings.terrain_setup;
    auto& vegetationSettings = levelSettings.vegetation_setup;

    SizeT width = this->GetCmdLineArgs().GetInt("-w", 1280);
    SizeT height = this->GetCmdLineArgs().GetInt("-h", 1024);

    //FIXME
    CoreGraphics::WindowCreateInfo wndInfo =
    {
        CoreGraphics::DisplayMode {100, 100, width, height},
        this->title,
        "icon.png",
        CoreGraphics::AntiAliasQuality::None,
        nullptr,
        true,
        true,
        false,
        true
    };
    this->mainWindow = CreateMainWindow(wndInfo);
    Graphics::GraphicsServer::Instance()->AddWindow(this->mainWindow);

    CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(this->mainWindow);

    FrameScript_shadows::Initialize(1024, 1024);
    FrameScript_default::Initialize(mode.GetWidth(), mode.GetHeight());
#if WITH_NEBULA_EDITOR
    if (App::GameApplication::IsEditorEnabled())
    {
        FrameScript_editorframe::Initialize(mode.GetWidth(), mode.GetHeight());
    }
#endif
    this->defaultView = gfxServer->CreateView("mainview", FrameScript_default::Run, Math::rectangle<int>(0, 0, mode.GetWidth(), mode.GetHeight()));
    this->defaultStage = gfxServer->CreateStage("defaultStage", true);
    this->defaultView->SetStage(this->defaultStage);
    this->globalLight = Graphics::CreateEntity();

    Im3d::Im3dContext::Create();
    Dynui::ImguiContext::Create();
    //StaticUI::StaticUIContext::Create();
    TBUI::TBUIContext::Create();

    CameraContext::Create();
    ModelContext::Create();
    ObserverContext::Create();
    ObservableContext::Create();
    ParticleContext::Create();

    Raytracing::RaytracingSetupSettings raytracingSettings =
    {
        .maxNumAllowedInstances = 0xFFFF,
    };
    Raytracing::RaytracingContext::Create(raytracingSettings);
    Clustering::ClusterContext::Create(0.01f, 1000.0f, this->mainWindow);

    Lighting::LightContext::Create();
    Decals::DecalContext::Create();
    Characters::CharacterContext::Create();
    Fog::VolumetricFogContext::Create();
    GI::DDGIContext::Create();
    ::Terrain::TerrainContext::Create();
    PostEffects::BloomContext::Create();
    PostEffects::SSAOContext::Create();
    PostEffects::HistogramContext::Create();
    PostEffects::DownsamplingContext::Create();

    PostEffects::BloomContext::Setup();
    PostEffects::SSAOContext::Setup();
    PostEffects::HistogramContext::Setup();
    PostEffects::HistogramContext::SetWindow({ 0.0f, 0.0f }, { 1.0f, 1.0f }, 1);
    PostEffects::DownsamplingContext::Setup();

    Graphics::SetupBufferConstants();
    this->gfxServer->AddPreViewCall([](IndexT frameIndex, IndexT bufferIndex) {
        static auto lastFrameSubmission = FrameScript_default::Submission_Scene;
        FrameScript_shadows::Run(Math::rectangle<int>(0, 0, 1024, 1024), frameIndex, bufferIndex);
        FrameScript_default::Bind_Shadows(FrameScript_shadows::Submission_Shadows);
        FrameScript_default::Bind_SunShadowDepth(Frame::TextureImport::FromExport(FrameScript_shadows::Export_SunShadowDepth));
    });
    this->gfxServer->SetResizeCall([](const SizeT windowWidth, const SizeT windowHeight) {
        FrameScript_default::Initialize(windowWidth, windowHeight);
        Graphics::SetupBufferConstants();
        FrameScript_default::SetupPipelines();
        FrameScript_shadows::SetupPipelines();
#if WITH_NEBULA_EDITOR
        if (App::GameApplication::IsEditorEnabled())
        {
            FrameScript_editorframe::Initialize(windowWidth, windowHeight);
            FrameScript_editorframe::SetupPipelines();
        }
#endif
    });

    Lighting::LightContext::RegisterEntity(this->globalLight);
    Lighting::LightContext::SetupGlobalLight(this->globalLight, Math::vec3(1), 50.000f, Math::vec3(0, 0, 0), 70_rad, 0_rad, true);

    ObserverContext::CreateBruteforceSystem({});

    // create environment context for the atmosphere effects
    EnvironmentContext::Create(this->globalLight);

    Util::Array<Graphics::ViewIndependentCall> preLogicCalls =
    {
        Dynui::ImguiContext::NewFrame,
        TBUI::TBUIContext::FrameUpdate,
        CameraContext::UpdateCameras,
        ModelContext::UpdateTransforms,
        Characters::CharacterContext::UpdateAnimations,
        Fog::VolumetricFogContext::RenderUI,
        EnvironmentContext::OnBeforeFrame,
        EnvironmentContext::RenderUI,
        Raytracing::RaytracingContext::ReconstructTopLevelAcceleration,
        Particles::ParticleContext::UpdateParticles,
        ::Terrain::TerrainContext::RenderUI
    };

    Util::Array<Graphics::ViewDependentCall> preLogicViewCalls =
    {
        Lighting::LightContext::OnPrepareView,
        Particles::ParticleContext::OnPrepareView,
        Im3d::Im3dContext::OnPrepareView,
        PostEffects::SSAOContext::UpdateViewDependentResources,
        PostEffects::HistogramContext::UpdateViewResources,
        Decals::DecalContext::UpdateViewDependentResources,
        Fog::VolumetricFogContext::UpdateViewDependentResources,
        Lighting::LightContext::UpdateViewDependentResources,
        Raytracing::RaytracingContext::UpdateViewResources,
        GI::DDGIContext::UpdateActiveVolumes,
        ::Terrain::TerrainContext::CullPatches
    };

    Util::Array<Graphics::ViewIndependentCall> postLogicCalls =
    {
        Dynui::ImguiContext::EndFrame,
        Clustering::ClusterContext::UpdateResources,
        ObserverContext::RunVisibilityTests,
        ObserverContext::GenerateDrawLists,
        Raytracing::RaytracingContext::UpdateTransforms,

        // At the very latest point, wait for work to finish
        ModelContext::WaitForWork,
        Raytracing::RaytracingContext::WaitForJobs,
        Characters::CharacterContext::WaitForCharacterJobs,
        Particles::ParticleContext::WaitForParticleUpdates,
        ObserverContext::WaitForVisibility
    };

    Util::Array<Graphics::ViewDependentCall> postLogicViewCalls =
    {
        ::Terrain::TerrainContext::UpdateLOD
    };

    this->gfxServer->SetupPreLogicCalls(preLogicCalls);
    this->gfxServer->SetupPreLogicViewCalls(preLogicViewCalls);
    this->gfxServer->SetupPostLogicCalls(postLogicCalls);
    this->gfxServer->SetupPostLogicViewCalls(postLogicViewCalls);

    // Attach managers
    this->graphicsManager = GraphicsManager::Create();
    this->cameraManager = CameraManager::Create();

    this->AttachManager(this->graphicsManager);
    this->AttachManager(this->cameraManager);

    FrameScript_default::SetupPipelines();
    FrameScript_shadows::SetupPipelines();
#if WITH_NEBULA_EDITOR
    if (App::GameApplication::IsEditorEnabled())
    {
        FrameScript_editorframe::SetupPipelines();
    }
#endif
    this->defaultViewHandle = CameraManager::RegisterView(this->defaultView);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnDeactivate()
{
    this->RemoveManager(this->cameraManager);
    this->RemoveManager(this->graphicsManager);
    this->cameraManager = nullptr;
    this->graphicsManager = nullptr;
    Raytracing::RaytracingContext::Discard();
    Im3d::Im3dContext::Discard();
    Dynui::ImguiContext::Discard();
    TBUI::TBUIContext::Discard();
    FeatureUnit::OnDeactivate();
    CoreGraphics::DestroyWindow(this->mainWindow);
    this->gfxServer->DiscardStage(this->defaultStage);
    this->gfxServer->DiscardView(this->defaultView);
    ObserverContext::Discard();
    Lighting::LightContext::Discard();
    Decals::DecalContext::Discard();
    Fog::VolumetricFogContext::Discard();

    this->gfxServer->Close();
    this->inputServer->Close();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnBeginFrame()
{
    FeatureUnit::OnBeginFrame();

    // Do potential new-frame stuff for window, such as resize
    N_MARKER_BEGIN(ResizeWindows, App)
    const auto& windows = Graphics::GraphicsServer::Instance()->GetWindows();
    for (const auto& window : windows)
        CoreGraphics::WindowNewFrame(window);
    N_MARKER_END()

    this->inputServer->BeginFrame();

    N_MARKER_BEGIN(PollWindows, CoreGraphics)
    CoreGraphics::WindowPollEvents();
    N_MARKER_END()

    N_MARKER_BEGIN(Input, Input)
    this->inputServer->OnFrame();
    N_MARKER_END()

    this->gfxServer->RunPreLogic();

    switch (Core::CVarReadInt(this->r_debug))
    {
    case 2:
        this->gfxServer->RenderDebug(0);
    case 1:
        Game::GameServer::Instance()->RenderDebug();
    default:
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnBeforeViews()
{
    FeatureUnit::OnBeforeViews();
    this->gfxServer->RunPostLogic();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnFrame()
{
    FeatureUnit::OnFrame();
    this->gfxServer->Render();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnEndFrame()
{
    FeatureUnit::OnEndFrame();

    // Finish up the frame and present the current framebuffer
    this->gfxServer->EndFrame();
    N_MARKER_BEGIN(Present, App);
    const auto& windows = Graphics::GraphicsServer::Instance()->GetWindows();
    for (const auto& window : windows)
        CoreGraphics::WindowPresent(window, App::GameApplication::FrameIndex);
    
    N_MARKER_END();

    // Trigger a new frame
    this->gfxServer->NewFrame();
    this->inputServer->EndFrame();

}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnRenderDebug()
{
    FeatureUnit::OnRenderDebug();
}

} // namespace GraphicsFeature
