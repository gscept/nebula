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
#include "flat/graphicsfeature/graphicsfeatureschema.h"
#include "flat/graphicsfeature/terrainschema.h"
#include "flat/graphicsfeature/vegetationschema.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "components/graphicsfeature.h"

#include "scripting/deargui.h"

#include "terrain/terraincontext.h"

#include "frame/default.h"
#include "frame/shadows.h"

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
    Game::RegisterType<PointLight>({.decay = true, .OnInit = &GraphicsManager::InitPointLight });
    Game::RegisterType<Model>({.decay = true, .OnInit = &GraphicsManager::InitModel });
    Game::RegisterType<Camera>();
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

    const IO::URI terrainSetupPath("data:tables/graphicsfeature/terrain.pter");
    GraphicsFeature::TerrainSetupT terrainSettings;
    if (IO::IoServer::Instance()->ReadFile(terrainSetupPath, contents))
    {
        Flat::FlatbufferInterface::DeserializeFlatbuffer<GraphicsFeature::TerrainSetup>(terrainSettings, (byte*)contents.AsCharPtr());
    }

    const IO::URI vegetationSetupPath("data:tables/graphicsfeature/vegetation.pveg");
    GraphicsFeature::VegetationSetupT vegetationSettings;
    if (IO::IoServer::Instance()->ReadFile(vegetationSetupPath, contents))
    {
        Flat::FlatbufferInterface::DeserializeFlatbuffer<GraphicsFeature::VegetationSetup>(vegetationSettings, (byte*)contents.AsCharPtr());
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
        false
    };
    this->wnd = CreateWindow(wndInfo);

    FrameScript_shadows::Initialize(1024, 1024);
    FrameScript_default::Initialize(width, height);
    this->defaultView = gfxServer->CreateView("mainview", FrameScript_default::Run, this->wnd);
    this->defaultStage = gfxServer->CreateStage("defaultStage", true);
    this->defaultView->SetStage(this->defaultStage);
    this->globalLight = Graphics::CreateEntity();

    Im3d::Im3dContext::Create();
    Dynui::ImguiContext::Create();
    //StaticUI::StaticUIContext::Create();

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
    Clustering::ClusterContext::Create(0.01f, 1000.0f, this->wnd);

    if (terrainSettings.config->use)
    {
        Terrain::TerrainSetupSettings settings {
            terrainSettings.config->min_height,
            terrainSettings.config->max_height, // Min/max height
            terrainSettings.config->world_size_width,
            terrainSettings.config->world_size_height, // World size in meters
            terrainSettings.config->tile_size_width,
            terrainSettings.config->tile_size_height, // Tile size in meters
            terrainSettings.config->quads_per_tile_width,
            terrainSettings.config->quads_per_tile_height, // Amount of quads per tile
        };
        Terrain::TerrainContext::Create(settings);
        Terrain::TerrainContext::SetSun(this->globalLight);

        /*
        this->terrain.entity = Graphics::CreateEntity();
        Graphics::RegisterEntity<Terrain::TerrainContext>(this->terrain.entity);
        Terrain::TerrainContext::SetupTerrain(this->terrain.entity, terrainSettings.instance->height, terrainSettings.instance->decision, terrainSettings.config->raytracing);

        for (IndexT i = 0; i < terrainSettings.biomes.size(); i++)
        {
            const std::unique_ptr<TerrainBiomeSettingsT>& settings = terrainSettings.biomes[i];

            Terrain::BiomeParameters biomeParams =
            {
                .slopeThreshold = settings->parameters->slope_threshold,
                .heightThreshold = settings->parameters->height_threshold,
                .uvScaleFactor = settings->parameters->uv_scale_factor
            };
            Terrain::BiomeSettings biomeSettings = Terrain::BiomeSettingsBuilder()
                .Parameters(biomeParams)
                .FlatMaterial(Terrain::BiomeMaterialBuilder()
                    .Albedo(settings->flat_material->albedo)
                    .Normal(settings->flat_material->normal)
                    .Material(settings->flat_material->material)
                    .Finish()
                )
                .SlopeMaterial(Terrain::BiomeMaterialBuilder()
                    .Albedo(settings->slope_material->albedo)
                    .Normal(settings->slope_material->normal)
                    .Material(settings->slope_material->material)
                    .Finish()
                )
                .HeightMaterial(Terrain::BiomeMaterialBuilder()
                    .Albedo(settings->height_material->albedo)
                    .Normal(settings->height_material->normal)
                    .Material(settings->height_material->material)
                    .Finish()
                )
                .HeightSlopeMaterial(Terrain::BiomeMaterialBuilder()
                    .Albedo(settings->height_slope_material->albedo)
                    .Normal(settings->height_slope_material->normal)
                    .Material(settings->height_slope_material->material)
                    .Finish()
                )
                .Mask(settings->mask)
                .Finish();

            Terrain::TerrainBiomeId biome = Terrain::TerrainContext::CreateBiome(biomeSettings);
            this->terrain.biomes.Append(biome);
        }

        if (vegetationSettings.use)
        {
            Vegetation::VegetationSetupSettings vegSettings {
                terrainSettings.config->min_height,
                terrainSettings.config->max_height, // min/max height
                Math::vec2 {terrainSettings.config->world_size_width, terrainSettings.config->world_size_height}};
            Vegetation::VegetationContext::Create(vegSettings);
        }
        */
    }
  
    Lighting::LightContext::Create();
    Decals::DecalContext::Create();
    Characters::CharacterContext::Create();
    Fog::VolumetricFogContext::Create();
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

    Lighting::LightContext::RegisterEntity(this->globalLight);
    Lighting::LightContext::SetupGlobalLight(this->globalLight, Math::vec3(1), 50.000f, Math::vec3(0, 0, 0), Math::vec3(0, 0, 0), 0, 70_rad, 0_rad, true);

    ObserverContext::CreateBruteforceSystem({});

    // create environment context for the atmosphere effects
    EnvironmentContext::Create(this->globalLight);

    Util::Array<Graphics::ViewIndependentCall> preLogicCalls =
    {
        Im3d::Im3dContext::NewFrame,
        Dynui::ImguiContext::NewFrame,
        CameraContext::UpdateCameras,
        ModelContext::UpdateTransforms,
        Characters::CharacterContext::UpdateAnimations,
        Fog::VolumetricFogContext::RenderUI,
        EnvironmentContext::OnBeforeFrame,
        EnvironmentContext::RenderUI,
        Raytracing::RaytracingContext::ReconstructTopLevelAcceleration,
        Particles::ParticleContext::UpdateParticles,
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
    };

    Util::Array<Graphics::ViewIndependentCall> postLogicCalls =
    {
        Clustering::ClusterContext::UpdateResources,
        ObserverContext::RunVisibilityTests,
        ObserverContext::GenerateDrawLists,
        Raytracing::RaytracingContext::UpdateTransforms,

        // At the very latest point, wait for work to finish
        Dynui::ImguiContext::Render,
        ModelContext::WaitForWork,
        Raytracing::RaytracingContext::WaitForJobs,
        Characters::CharacterContext::WaitForCharacterJobs,
        Particles::ParticleContext::WaitForParticleUpdates,
        ObserverContext::WaitForVisibility
    };

    Util::Array<Graphics::ViewDependentCall> postLogicViewCalls =
    {
    };
    
    if (terrainSettings.config->use)
    {
        preLogicCalls.Append(Terrain::TerrainContext::RenderUI);
        preLogicViewCalls.Append(Terrain::TerrainContext::CullPatches);
        postLogicViewCalls.Append(Terrain::TerrainContext::UpdateLOD);

        if (vegetationSettings.use)
        {
            postLogicViewCalls.Append(Vegetation::VegetationContext::UpdateViewResources);
        }
    }

    this->gfxServer->SetupPreLogicCalls(preLogicCalls);
    this->gfxServer->SetupPreLogicViewCalls(preLogicViewCalls);
    this->gfxServer->SetupPostLogicCalls(postLogicCalls);
    this->gfxServer->SetupPostLogicViewCalls(postLogicViewCalls);

    // Attach managers
    this->graphicsManagerHandle = this->AttachManager(GraphicsManager::Create());
    this->cameraManagerHandle = this->AttachManager(CameraManager::Create());

    FrameScript_default::SetupPipelines();
    FrameScript_shadows::SetupPipelines();
    this->defaultViewHandle = CameraManager::RegisterView(this->defaultView);
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
    CoreGraphics::WindowMakeCurrent(this->wnd);
    this->inputServer->OnFrame();

    this->gfxServer->RunPreLogic();

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
    CoreGraphics::WindowPresent(this->wnd, App::GameApplication::FrameIndex);
    N_MARKER_END();

    // Trigger a new frame
    this->gfxServer->NewFrame();
    this->inputServer->EndFrame();

    // Do potential new-frame stuff for window, such as resize
    CoreGraphics::WindowNewFrame(this->wnd);
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
