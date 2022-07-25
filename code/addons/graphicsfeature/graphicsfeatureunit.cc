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
#include "staticui/staticuicontext.h"
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
#include "vegetation/vegetationcontext.h"
#include "posteffects/histogramcontext.h"
#include "particles/particlecontext.h"

#include "graphics/globalconstants.h"

#include "graphicsfeature/managers/graphicsmanager.h"
#include "graphicsfeature/managers/cameramanager.h"

#include "nflatbuffer/nebula_flat.h"
#include "flat/graphicsfeature/graphicsfeatureschema.h"
#include "nflatbuffer/flatbufferinterface.h"


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
GraphicsFeatureUnit::OnActivate()
{
    FeatureUnit::OnActivate();

    this->r_debug = Core::CVarCreate(Core::CVar_Int, "r_debug", "0", "Enable debugging rendering [0,2]");
    this->r_show_frame_inspector = Core::CVarCreate(Core::CVar_Int, "r_show_frame_inspector", "0", "Show the frame script inspector [0,1]");

    this->gfxServer = Graphics::GraphicsServer::Create();
    this->inputServer = Input::InputServer::Create();
    this->gfxServer->Open();
    this->inputServer->Open();

    const IO::URI graphicsFeaturePath("data:tables/graphicsfeature/graphicsfeature.json");
    Util::String contents;

    GraphicsFeature::TerrainSettingsT terrainSettings;
    if (IO::IoServer::Instance()->ReadFile(graphicsFeaturePath, contents))
    {
        Flat::FlatbufferInterface::DeserializeFlatbuffer<GraphicsFeature::TerrainSettings>(terrainSettings, (byte*)contents.AsCharPtr());
    }

    SizeT width = this->GetCmdLineArgs().GetInt("-w", 1280);
    SizeT height = this->GetCmdLineArgs().GetInt("-h", 1024);

    //FIXME
    CoreGraphics::WindowCreateInfo wndInfo =
        {
            CoreGraphics::DisplayMode {100, 100, width, height},
            this->title,
            "",
            CoreGraphics::AntiAliasQuality::None,
            true,
            true,
            false};
    this->wnd = CreateWindow(wndInfo);

    this->defaultView = gfxServer->CreateView("mainview", this->defaultFrameScript);
    this->defaultStage = gfxServer->CreateStage("defaultStage", true);
    this->defaultView->SetStage(this->defaultStage);
    this->globalLight = Graphics::CreateEntity();

    Ptr<Frame::FrameScript> frameScript = this->defaultView->GetFrameScript();

    Im3d::Im3dContext::Create();
    Dynui::ImguiContext::Create();
    StaticUI::StaticUIContext::Create();

    CameraContext::Create();
    ModelContext::Create();
    ObserverContext::Create();
    ObservableContext::Create();
    ParticleContext::Create();
    Clustering::ClusterContext::Create(0.01f, 1000.0f, this->wnd);

    Terrain::TerrainSetupSettings settings{
        terrainSettings.min_height, terrainSettings.max_height,                         // Min/max height 
        terrainSettings.world_size_width, terrainSettings.world_size_height,            // World size in meters
        terrainSettings.tile_size_width, terrainSettings.tile_size_height,              // Tile size in meters
        terrainSettings.quads_per_tile_width, terrainSettings.quads_per_tile_height,    // Amount of quads per tile
        this->globalLight
    };
    Terrain::TerrainContext::Create(settings);

    Vegetation::VegetationSetupSettings vegSettings{
        terrainSettings.min_height, terrainSettings.max_height,      // min/max height 
        Math::vec2{ terrainSettings.world_size_width, terrainSettings.world_size_height }
    };
    Vegetation::VegetationContext::Create(vegSettings);

    Lighting::LightContext::Create(frameScript);
    Decals::DecalContext::Create();
    Characters::CharacterContext::Create();
    Fog::VolumetricFogContext::Create(frameScript);
    PostEffects::BloomContext::Create();
    PostEffects::SSAOContext::Create();
    PostEffects::HistogramContext::Create();

    PostEffects::BloomContext::Setup(frameScript);
    PostEffects::SSAOContext::Setup(frameScript);
    PostEffects::HistogramContext::Setup(frameScript);
    PostEffects::HistogramContext::SetWindow({ 0.0f, 0.0f }, { 1.0f, 1.0f }, 1);

    Graphics::SetupBufferConstants(frameScript);

    Lighting::LightContext::RegisterEntity(this->globalLight);
    Lighting::LightContext::SetupGlobalLight(this->globalLight, Math::vec3(0.734, 0.583, 0.377), 50.000f, Math::vec3(0, 0, 0), Math::vec3(0, 0, 0), 0, 60_rad, 0_rad, true);

    ObserverContext::CreateBruteforceSystem({});

    // create environment context for the atmosphere effects
    EnvironmentContext::Create(this->globalLight);

    // Attach managers
    this->graphicsManagerHandle = this->AttachManager(GraphicsManager::Create());
    this->cameraManagerHandle = this->AttachManager(CameraManager::Create());

    this->defaultViewHandle = CameraManager::RegisterView(this->defaultView);

    this->defaultView->BuildFrameScript();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnDeactivate()
{
    Im3d::Im3dContext::Discard();
    Dynui::ImguiContext::Discard();
    StaticUI::StaticUIContext::Discard();
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

    this->inputServer->BeginFrame();

    CoreGraphics::WindowPollEvents();
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

}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnBeforeViews()
{
    FeatureUnit::OnBeforeViews();
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
    N_MARKER_BEGIN(Present, App);
    CoreGraphics::WindowPresent(this->wnd, App::GameApplication::FrameIndex);
    N_MARKER_END();
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

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::AddRenderUICallback(UIRenderFunc func)
{
    this->uiCallbacks.Append(func);
}

} // namespace GraphicsFeature
