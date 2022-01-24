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
#include "characters/charactercontext.h"
#include "dynui/im3d/im3dcontext.h"
#include "appgame/gameapplication.h"
#include "graphics/environmentcontext.h"
#include "clustering/clustercontext.h"
#include "fog/volumetricfogcontext.h"
#include "posteffects/bloomcontext.h"
#include "posteffects/ssaocontext.h"
#include "posteffects/ssrcontext.h"
#include "decals/decalcontext.h"
#include "debug/framescriptinspector.h"
#include "terrain/terraincontext.h"
#include "posteffects/histogramcontext.h"
#include "particles/particlecontext.h"

#include "graphicsfeature/managers/graphicsmanager.h"
#include "graphicsfeature/managers/cameramanager.h"

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
GraphicsFeatureUnit::OnActivate()
{
    FeatureUnit::OnActivate();

    this->r_debug = Core::CVarCreate(Core::CVar_Int, "r_debug", "0", "Enable debugging rendering [0,2]");
    this->r_show_frame_inspector = Core::CVarCreate(Core::CVar_Int, "r_show_frame_inspector", "0", "Show the frame script inspector [0,1]");

    this->gfxServer = Graphics::GraphicsServer::Create();
    this->inputServer = Input::InputServer::Create();
    this->gfxServer->Open();
    this->inputServer->Open();

    SizeT width = this->GetCmdLineArgs().GetInt("-w", 1280);
    SizeT height = this->GetCmdLineArgs().GetInt("-h", 960);

    //FIXME
    CoreGraphics::WindowCreateInfo wndInfo =
        {
            CoreGraphics::DisplayMode {100, 100, width, height},
            "GraphicsFeature",
            "",
            CoreGraphics::AntiAliasQuality::None,
            true,
            true,
            false};
    this->wnd = CreateWindow(wndInfo);

    CameraContext::Create();
    ModelContext::Create();
    ObserverContext::Create();
    ObservableContext::Create();
    ParticleContext::Create();
    Clustering::ClusterContext::Create(0.01f, 1000.0f, this->wnd);
    Lighting::LightContext::Create();
    Decals::DecalContext::Create();
    Characters::CharacterContext::Create();
    Im3d::Im3dContext::Create();
    Dynui::ImguiContext::Create();
    Fog::VolumetricFogContext::Create();
    PostEffects::BloomContext::Create();
    PostEffects::SSAOContext::Create();
    PostEffects::HistogramContext::Create();
    Terrain::TerrainSetupSettings settings{
        0, 1024.0f,      // min/max height 
        //0, 0,
        8192, 8192,   // world size in meters
        256, 256,     // tile size in meters
        16, 16        // 1 vertex every X meters
    };
    Terrain::TerrainContext::Create(settings);

    this->defaultView = gfxServer->CreateView("mainview", this->defaultFrameScript);
    this->defaultStage = gfxServer->CreateStage("defaultStage", true);
    this->defaultView->SetStage(this->defaultStage);

    Ptr<Frame::FrameScript> frameScript = this->defaultView->GetFrameScript();
    PostEffects::BloomContext::Setup(frameScript);
    PostEffects::SSAOContext::Setup(frameScript);
    PostEffects::HistogramContext::Setup(frameScript);
    PostEffects::HistogramContext::SetWindow({ 0.0f, 0.0f }, { 1.0f, 1.0f }, 1);

    CoreGraphics::ShaderServer::Instance()->SetupBufferConstants(frameScript);

    this->globalLight = Graphics::CreateEntity();
    Lighting::LightContext::RegisterEntity(this->globalLight);
    Lighting::LightContext::SetupGlobalLight(this->globalLight, Math::vec3(0.734, 0.583, 0.377), 50.000f, Math::vec3(0, 0, 0), Math::vec3(0, 0, 0), 0, Math::vector(1, -1, 1), true);

    ObserverContext::CreateBruteforceSystem({});

    // create environment context for the atmosphere effects
    EnvironmentContext::Create(this->globalLight);

    // Attach managers
    this->graphicsManagerHandle = this->AttachManager(GraphicsManager::Create());
    this->cameraManagerHandle = this->AttachManager(CameraManager::Create());

    this->defaultViewHandle = CameraManager::RegisterView(this->defaultView);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnDeactivate()
{
    FeatureUnit::OnDeactivate();
    DestroyWindow(this->wnd);
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

    CoreGraphics::WindowPollEvents();

    this->inputServer->BeginFrame();
    this->inputServer->OnFrame();

    this->gfxServer->BeginFrame();

    for (auto const& uiFunc : this->uiCallbacks)
    {
        uiFunc();
    }

    switch (Core::CVarReadInt(this->r_debug))
    {
    case 2:
        this->gfxServer->RenderDebug(0);
    case 1:
        Game::GameServer::Instance()->RenderDebug();
    default:
        break;
    }

    if (Core::CVarReadInt(this->r_show_frame_inspector) > 0)
        Debug::FrameScriptInspector::Run(this->defaultView->GetFrameScript());


    this->gfxServer->BeforeViews();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnFrame()
{
    FeatureUnit::OnFrame();
    this->gfxServer->RenderViews();
    this->gfxServer->EndViews();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnEndFrame()
{
    FeatureUnit::OnEndFrame();
    this->gfxServer->EndFrame();
    CoreGraphics::WindowPresent(this->wnd, App::GameApplication::FrameIndex);
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

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::AddRenderUICallback(UIRenderFunc func)
{
    this->uiCallbacks.Append(func);
}

} // namespace GraphicsFeature
