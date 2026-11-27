//------------------------------------------------------------------------------
//  graphicsmanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "graphicsmanager.h"
#include "graphics/graphicsentity.h"
#include "graphics/graphicsserver.h"
#include "models/modelcontext.h"
#include "visibility/visibilitycontext.h"
#include "raytracing/raytracingcontext.h"
#include "characters/charactercontext.h"
#include "game/gameserver.h"
#include "basegamefeature/components/basegamefeature.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "io/jsonreader.h"
#include "io/jsonwriter.h"
#include "io/ioserver.h"
#include "lighting/lightcontext.h"
#include "decals/decalcontext.h"
#include "gi/ddgicontext.h"
#include "terrain/terraincontext.h"
#include "vegetation/vegetationcontext.h"

#include "nflatbuffer/nebula_flat.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "flat/render/terrainresource.h"
#include "graphicsfeature/terrain/terrainfile.h"

namespace GraphicsFeature
{

__ImplementClass(GraphicsFeature::GraphicsManager, 'GrMa', Game::Manager);

//------------------------------------------------------------------------------
/**
*/
GraphicsManager::GraphicsManager()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
GraphicsManager::~GraphicsManager()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    The file a character resource is loaded from. A component holds a URN, folder:file:type, and the
    loader reads the file that is exported to. A name that is not a URN of the type is taken as the
    export path it already is
*/
static Resources::ResourceName
CharacterResourceFile(const Resources::ResourceName& name, const char* type)
{
    const IO::Path path = IO::Path::Parse(name.Value());
    if (path.IsFile() && path.GetType() == type)
        return path.ExportURI().AsString();
    return name;
}

//------------------------------------------------------------------------------
/**
*/
void
RegisterModelEntity(
    Graphics::GraphicsEntityId const gid,
    Resources::ResourceName const res,
    Resources::ResourceName const anim,
    Resources::ResourceName const skeleton,
    uint const stage,
    bool const raytracing,
    Math::mat4 const& t
)
{
    auto modelURN = IO::Path::Parse(res.Value());
    Models::ModelContext::RegisterEntity(gid);
    Models::ModelContext::Setup(
        gid,
        modelURN,
        "NONE",
        [gid, modelURN, anim, skeleton, raytracing, t]()
        {
            if (!Graphics::GraphicsServer::Instance()->IsValidGraphicsEntity(gid))
                return;
            Visibility::ObservableContext::RegisterEntity(gid);
            Models::ModelContext::SetTransform(gid, t);
            Visibility::ObservableContext::Setup(gid, Visibility::VisibilityEntityType::Model);
            if (raytracing && CoreGraphics::RayTracingSupported)
            {
                Raytracing::RaytracingContext::RegisterEntity(gid);
                Raytracing::RaytracingContext::SetupModel(gid, CoreGraphics::BlasInstanceFlags::NoFlags, 0xFF);
            }
            // The clip is played as soon as the character is set up, so the files have to be there.
            // A model that names ones that are not, or none, uses the ones packaged with it
            const Resources::ResourceName skeletonFile = anim.IsValid() && skeleton.IsValid() ? CharacterResourceFile(skeleton, "ske") : Resources::ResourceName();
            const Resources::ResourceName animFile = anim.IsValid() && skeleton.IsValid() ? CharacterResourceFile(anim, "ani") : Resources::ResourceName();
            if (skeletonFile.IsValid() && animFile.IsValid() && IO::FileExists(skeletonFile.Value()) && IO::FileExists(animFile.Value()))
            {
                Characters::CharacterContext::RegisterEntity(gid);
                Characters::CharacterContext::Setup(gid, skeletonFile, 0, animFile, 0, "NONE");
                Characters::CharacterContext::PlayClip(gid, nullptr, 0, 0, Characters::EnqueueMode::Replace);
            }
            else
            {
                if (skeletonFile.IsValid())
                    n_warning("Model '%s' names the skeleton '%s' and animation '%s', which are not there\n", modelURN.AsString().AsCharPtr(), skeleton.Value(), anim.Value());
                // a skinned model without an animation and skeleton set uses the ones packaged with it
                Characters::CharacterContext::SetupFromModel(gid, modelURN, "NONE");
            }
        },
        stage
    );
}

//------------------------------------------------------------------------------
/**
*/
void
DeregisterModelEntity(Model const* model)
{
    if ((Graphics::GraphicsEntityId)model->graphicsEntityId == Graphics::InvalidGraphicsEntityId)
        return;

    if (model->raytracing && CoreGraphics::RayTracingSupported &&
        Raytracing::RaytracingContext::IsEntityRegistered(model->graphicsEntityId))
    {
        Raytracing::RaytracingContext::DeregisterEntity(model->graphicsEntityId);
    }
    if (Characters::CharacterContext::IsEntityRegistered(model->graphicsEntityId))
    {
        Characters::CharacterContext::DeregisterEntity(model->graphicsEntityId);
    }
    if (Visibility::ObservableContext::IsEntityRegistered(model->graphicsEntityId))
    {
        Visibility::ObservableContext::DeregisterEntity(model->graphicsEntityId);
    }
    if (Models::ModelContext::IsEntityRegistered(model->graphicsEntityId))
    {
        Models::ModelContext::DeregisterEntity(model->graphicsEntityId);
    }

    Graphics::DestroyEntity(model->graphicsEntityId);
}

//------------------------------------------------------------------------------
/**
*/
void
DeregisterLight(Graphics::GraphicsEntityId gfxId)
{
    if (gfxId == Graphics::InvalidGraphicsEntityId)
        return;

    if (Lighting::LightContext::IsEntityRegistered(gfxId))
    {
        Lighting::LightContext::DeregisterEntity(gfxId);
    }
    Graphics::DestroyEntity(gfxId);
}

//------------------------------------------------------------------------------
/**
*/
void
DeregisterDecal(Graphics::GraphicsEntityId gfxId)
{
    if (gfxId == Graphics::InvalidGraphicsEntityId)
        return;

    if (Decals::DecalContext::IsEntityRegistered(gfxId))
    {
        Decals::DecalContext::DeregisterEntity(gfxId);
    }
    Graphics::DestroyEntity(gfxId);
}

//------------------------------------------------------------------------------
/**
*/
void
DeregisterTerrain(Graphics::GraphicsEntityId gfxId)
{
    if (gfxId == Graphics::InvalidGraphicsEntityId)
        return;

    if (Vegetation::VegetationContext::IsEntityRegistered(gfxId))
        Vegetation::VegetationContext::DeregisterEntity(gfxId);
    if (::Terrain::TerrainContext::IsEntityRegistered(gfxId))
        ::Terrain::TerrainContext::DeregisterEntity(gfxId);
    Graphics::DestroyEntity(gfxId);
}

//------------------------------------------------------------------------------
/**
*/
void
DeregisterDDGIVolume(Graphics::GraphicsEntityId gfxId)
{
    if (gfxId == Graphics::InvalidGraphicsEntityId)
        return;

    if (GI::DDGIContext::IsEntityRegistered(gfxId))
    {
        GI::DDGIContext::DeregisterEntity(gfxId);
    }
    Graphics::DestroyEntity(gfxId);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::OnDecay()
{
    N_MARKER_BEGIN(GraphicsManagerOnDecay, Game);
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);

    // Decay models
    Game::ComponentDecayBuffer const modelDecayBuffer = world->GetDecayBuffer(Game::GetComponentId<Model>());
    Model* data = (Model*)modelDecayBuffer.buffer;
    for (int i = 0; i < modelDecayBuffer.size; i++)
    {
        DeregisterModelEntity(data + i);
    }

    Game::ComponentDecayBuffer const pointLightDecayBuffer = world->GetDecayBuffer(Game::GetComponentId<PointLight>());
    PointLight* pointLightData = (PointLight*)pointLightDecayBuffer.buffer;
    for (int i = 0; i < pointLightDecayBuffer.size; i++)
    {
        PointLight* light = pointLightData + i;
        DeregisterLight(light->graphicsEntityId);
    }

    Game::ComponentDecayBuffer const spotLightDecayBuffer = world->GetDecayBuffer(Game::GetComponentId<SpotLight>());
    SpotLight* spotLightData = (SpotLight*)spotLightDecayBuffer.buffer;
    for (int i = 0; i < spotLightDecayBuffer.size; i++)
    {
        SpotLight* light = spotLightData + i;
        DeregisterLight(light->graphicsEntityId);
    }

    Game::ComponentDecayBuffer const areaLightDecayBuffer = world->GetDecayBuffer(Game::GetComponentId<AreaLight>());
    AreaLight* areaLightData = (AreaLight*)areaLightDecayBuffer.buffer;
    for (int i = 0; i < areaLightDecayBuffer.size; i++)
    {
        AreaLight* light = areaLightData + i;
        DeregisterLight(light->graphicsEntityId);
    }

    Game::ComponentDecayBuffer const decalDecayBuffer = world->GetDecayBuffer(Game::GetComponentId<Decal>());
    Decal* decalData = (Decal*)decalDecayBuffer.buffer;
    for (int i = 0; i < decalDecayBuffer.size; i++)
    {
        Decal* decal = decalData + i;
        DeregisterDecal(decal->graphicsEntityId);
    }

    Game::ComponentDecayBuffer const ddgiVolumeDecayBuffer = world->GetDecayBuffer(Game::GetComponentId<DDGIVolume>());
    DDGIVolume* ddgiVolumeData = (DDGIVolume*)ddgiVolumeDecayBuffer.buffer;
    for (int i = 0; i < ddgiVolumeDecayBuffer.size; i++)
    {
        DDGIVolume* volume = ddgiVolumeData + i;
        DeregisterDDGIVolume(volume->graphicsEntityId);
    }

    Game::ComponentDecayBuffer const terrainDecayBuffer = world->GetDecayBuffer(Game::GetComponentId<Terrain>());
    Terrain* terrainData = (Terrain*)terrainDecayBuffer.buffer;
    for (int i = 0; i < terrainDecayBuffer.size; i++)
        DeregisterTerrain((terrainData + i)->graphicsEntityId);
    N_MARKER_END();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::InitUpdateModelTransformProcessor()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    Game::ProcessorBuilder(world, "GraphicsManager.UpdateModelTransforms"_atm)
        .Excluding<Game::Static>()
        // .Async(true) // TODO: Should be able to be async, since two entities should not share transforms in model context
        .Func(
            [](Game::World* world,
               Game::Position const& pos,
               Game::Orientation const& orient,
               Game::Scale const& scale,
               GraphicsFeature::Model const& model)
            {
                Math::mat4 worldTransform = Math::trs(pos, orient, scale);
                Models::ModelContext::SetTransform(model.graphicsEntityId, worldTransform);
            }
        )
        .Build();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::InitUpdateLightTransformProcessor()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);

    Game::ProcessorBuilder(world, "GraphicsManager.UpdatePointLightPositions"_atm)
        .On("OnEndFrame")
        .Excluding<Game::Static>()
        .Func(
            [](Game::World* world, Game::Position const& pos, GraphicsFeature::PointLight const& light)
            {
                Lighting::LightContext::SetPosition(light.graphicsEntityId, pos);
            }
        )
        .Build();

    Game::ProcessorBuilder(world, "GraphicsManager.UpdateSpotLightTransform"_atm)
        .On("OnEndFrame")
        .Excluding<Game::Static>()
        .Func(
            [](Game::World* world,
               Game::Position const& pos,
               Game::Orientation const& rot,
               GraphicsFeature::SpotLight const& light)
            {
                Lighting::LightContext::SetPosition(light.graphicsEntityId, pos);
                Lighting::LightContext::SetRotation(light.graphicsEntityId, rot);
            }
        )
        .Build();

    Game::ProcessorBuilder(world, "GraphicsManager.UpdateAreaLightTransform"_atm)
        .On("OnEndFrame")
        .Excluding<Game::Static>()
        .Func(
            [](Game::World* world,
               Game::Position const& pos,
               Game::Orientation const& rot,
               Game::Scale const& scale,
               GraphicsFeature::AreaLight const& light)
            {
                Lighting::LightContext::SetPosition(light.graphicsEntityId, pos);
                Lighting::LightContext::SetRotation(light.graphicsEntityId, rot);
                Lighting::LightContext::SetScale(light.graphicsEntityId, scale);
            }
        )
        .Build();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::InitUpdateDecalTransformProcessor()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);

    Game::ProcessorBuilder(world, "GraphicsManager.UpdateDecalTransform"_atm)
        .On("OnEndFrame")
        .Excluding<Game::Static>()
        .Func(
            [](Game::World* world,
               Game::Position const& pos,
               Game::Orientation const& rot,
               Game::Scale const& scale,
               GraphicsFeature::Decal const& decal)
            {
                Math::mat4 transform = Math::trs(pos, rot, scale);
                Decals::DecalContext::SetTransform(decal.graphicsEntityId, transform);
            }
        )
        .Build();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::InitUpdateDDGIVolumeTransformProcessor()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);

    Game::ProcessorBuilder(world, "GraphicsManager.UpdateDDGIVolumeTransform"_atm)
        .On("OnEndFrame")
        .Excluding<Game::Static>()
        .Func(
            [](Game::World* world,
               Game::Position const& pos,
               Game::Orientation const& rot,
               Game::Scale const& scale,
               GraphicsFeature::DDGIVolume const& volume)
            {
                Math::mat4 transform = Math::trs(pos, rot, scale);
                GI::DDGIContext::SetPosition(volume.graphicsEntityId, pos);
                GI::DDGIContext::SetSize(volume.graphicsEntityId, scale);
            }
        )
        .Build();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::OnActivate()
{
    Manager::OnActivate();
    this->InitUpdateModelTransformProcessor();
    this->InitUpdateLightTransformProcessor();
    this->InitUpdateDecalTransformProcessor();
    this->InitUpdateDDGIVolumeTransformProcessor();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::OnDeactivate()
{
    Manager::OnDeactivate();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::InitPointLight(Game::World* world, Game::Entity entity, PointLight* light)
{
    light->graphicsEntityId = Graphics::CreateEntity().id;

    // TODO: This is not finished, and needs revisiting
    Game::Position pos = world->GetComponent<Game::Position>(entity);
    Lighting::LightContext::RegisterEntity(light->graphicsEntityId);
    // TODO: Cookie projection support
    Lighting::LightContext::SetupPointLight(
        light->graphicsEntityId,
        {
            .color = light->color.vec,
            .intensity = light->intensity,
            .range = light->range,
            .stageMask = light->stageMask,
            .castShadows = light->castShadows
        }
    );
    Lighting::LightContext::SetPosition(light->graphicsEntityId, pos);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::InitSpotLight(Game::World* world, Game::Entity entity, SpotLight* light)
{
    light->graphicsEntityId = Graphics::CreateEntity().id;

    // TODO: This is not finished, and needs revisiting
    Game::Position pos = world->GetComponent<Game::Position>(entity);
    Game::Orientation rot = world->GetComponent<Game::Orientation>(entity);

    Lighting::LightContext::RegisterEntity(light->graphicsEntityId);
    // TODO: Cookie projection support
    Lighting::LightContext::SetupSpotLight(
        light->graphicsEntityId,
        {
            .color = light->color.vec,
            .intensity = light->intensity,
            .innerConeAngle = Math::deg2rad(light->innerConeAngle),
            .outerConeAngle = Math::deg2rad(light->outerConeAngle),
            .range = light->range,
            .stageMask = light->stageMask,
            .castShadows = light->castShadows
        }
    );
    Lighting::LightContext::SetPosition(light->graphicsEntityId, pos);
    Lighting::LightContext::SetRotation(light->graphicsEntityId, rot);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::InitAreaLight(Game::World* world, Game::Entity entity, AreaLight* light)
{
    light->graphicsEntityId = Graphics::CreateEntity().id;

    Game::Position pos = world->GetComponent<Game::Position>(entity);
    Game::Orientation rot = world->GetComponent<Game::Orientation>(entity);
    Game::Scale scale = world->GetComponent<Game::Scale>(entity);

    Lighting::LightContext::RegisterEntity(light->graphicsEntityId);
    Lighting::LightContext::SetupAreaLight(
        light->graphicsEntityId,
        {
            .shape = (Lighting::LightContext::AreaLightShape)light->shape,
            .color = light->color.vec,
            .intensity = light->intensity,
            .range = light->range,
            .stageMask = light->stageMask,
            .twoSided = light->twoSided,
            .castShadows = light->castShadows,
            .renderMesh = light->renderMesh
        }
    );
    Lighting::LightContext::SetPosition(light->graphicsEntityId, pos);
    Lighting::LightContext::SetRotation(light->graphicsEntityId, rot);
    Lighting::LightContext::SetScale(light->graphicsEntityId, scale);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::InitDecal(Game::World* world, Game::Entity entity, Decal* decal)
{
    //decal->graphicsEntityId = Graphics::CreateEntity().id;
    //Game::Position pos = world->GetComponent<Game::Position>(entity);
    //Game::Orientation rot = world->GetComponent<Game::Orientation>(entity);
    //Game::Scale scale = world->GetComponent<Game::Scale>(entity);
    //Decals::DecalContext::RegisterEntity(decal->graphicsEntityId);
    //Decals::DecalContext::SetupDecalPBR(
    //    decal->graphicsEntityId,
    //    transform,
    //    decal->resource, // TODO: load resource
    //
    //);
    //Math::mat4 transform = Math::trs(pos, rot, scale);
    //Decals::DecalContext::SetTransform(decal->graphicsEntityId, transform);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::InitDDGIVolume(Game::World* world, Game::Entity entity, DDGIVolume* volume)
{
    if (!CoreGraphics::RayTracingSupported)
        return;
    volume->graphicsEntityId = Graphics::CreateEntity().id;
    GI::DDGIContext::RegisterEntity(volume->graphicsEntityId);
    GI::DDGIContext::VolumeSetup setup;
    setup.numProbesX = volume->numProbesX;
    setup.numProbesY = volume->numProbesY;
    setup.numProbesZ = volume->numProbesZ;
    setup.numRaysPerProbe = volume->numRaysPerProbe;
    setup.position = world->GetComponent<Game::Position>(entity);
    setup.size = world->GetComponent<Game::Scale>(entity);
    setup.irradianceScale = volume->irradianceScale;
    setup.normalBias = volume->normalBias;
    setup.viewBias = volume->viewBias;
    setup.distanceExponent = volume->distanceExponent;
    setup.hysteresis = volume->hysteresis;
    setup.blend = volume->borderBlend;
    setup.blendCutoff = volume->borderBlendCutoff;
    setup.updateBudget = volume->updateBudget;

    setup.options.flags.relocate = volume->relocate;
    setup.options.flags.scrolling = volume->scrolling;
    setup.options.flags.classify = volume->classify;

    GI::DDGIContext::SetupVolume(volume->graphicsEntityId, setup);
}

//------------------------------------------------------------------------------
/**
    A terrain without a file, or whose file isn't exported yet, is the flat default one, so a new
    Terrain component shows something to edit
*/
void
GraphicsManager::InitTerrain(Game::World* world, Game::Entity entity, Terrain* terrain)
{
    terrain->graphicsEntityId = Graphics::CreateEntity().id;
    ::Terrain::TerrainContext::RegisterEntity(terrain->graphicsEntityId);
    ::Terrain::TerrainCreateInfo createInfo;

    Render::TerrainResourceT resource;
    if (!ReadTerrainFile(terrain->resource, resource))
    {
        if (terrain->resource.IsValid())
            n_warning("Terrain: '%s' could not be read, the terrain is flat\n", terrain->resource.Value());
        DefaultTerrainResource(resource);
    }

    createInfo.minHeight = resource.min_height;
    createInfo.maxHeight = resource.max_height;
    createInfo.quadsPerTileX = resource.quads_per_tile_x;
    createInfo.quadsPerTileY = resource.quads_per_tile_z;
    createInfo.tileWidth = resource.tile_width;
    createInfo.tileHeight = resource.tile_height;
    createInfo.width = resource.world_size_x;
    createInfo.height = resource.world_size_z;
    createInfo.heightMap = TerrainTexture(resource.height_map, "system:black:tex");
    createInfo.decisionMap = TerrainTexture(resource.decision_map, "system:white:tex");
    createInfo.enableRayTracing = resource.enable_ray_tracing;
    ::Terrain::TerrainContext::SetupTerrain(terrain->graphicsEntityId, createInfo);

    for (auto const& biome : resource.biomes)
    {
        ::Terrain::BiomeSettings settings;
        settings.biomeMask = TerrainTexture(biome->mask, "system:white:tex");
        for (uint i = 0; i < 4 && i < biome->materials.size(); i++)
        {
            settings.materials[i].albedo = TerrainTexture(biome->materials[i]->albedo, "system:white:tex");
            settings.materials[i].material = TerrainTexture(biome->materials[i]->material, "system:default_material:tex");
            settings.materials[i].normal = TerrainTexture(biome->materials[i]->normals, "system:nobump:tex");
        }
        settings.biomeParameters.heightThreshold = biome->height_threshold;
        settings.biomeParameters.slopeThreshold = biome->slope_threshold;
        settings.biomeParameters.uvScaleFactor = biome->uv_scale_factor;
        ::Terrain::TerrainContext::CreateBiome(terrain->graphicsEntityId, settings);
    }

    Vegetation::VegetationContext::RegisterEntity(terrain->graphicsEntityId);
    Vegetation::VegetationContext::Setup(terrain->graphicsEntityId);
    for (auto const& vegetation : resource.vegetation)
    {
        Vegetation::VegetationMeshSetup mesh;
        mesh.mask = TerrainTexture(vegetation->mask, "system:black:tex");
        mesh.model = TerrainModel(vegetation->model, "system:box:mdl");
        mesh.heightThreshold = vegetation->height_threshold;
        mesh.slopeThreshold = vegetation->slope_threshold;
        mesh.density = vegetation->density;
        Vegetation::VegetationContext::SetupMesh(terrain->graphicsEntityId, mesh);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::InitModel(Game::World* world, Game::Entity entity, Model* model)
{
    auto res = model->resource;
    model->graphicsEntityId = Graphics::CreateEntity().id;
    Game::Position pos = world->GetComponent<Game::Position>(entity);
    Game::Orientation orient = world->GetComponent<Game::Orientation>(entity);
    Game::Scale scale = world->GetComponent<Game::Scale>(entity);
    Math::mat4 worldTransform = Math::trs(pos, orient, scale);
    RegisterModelEntity(
        model->graphicsEntityId, model->resource, model->anim, model->skeleton, model->stages.GetBits(0), model->raytracing, worldTransform
    );
}

//------------------------------------------------------------------------------
/**
    Cleanup all graphics entities
*/
void
GraphicsManager::OnCleanup(Game::World* world)
{
    { // Model cleanup
        Game::Filter filter = Game::FilterBuilder().Including<Model>().Build();
        Game::Dataset data = world->Query(filter);

        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::View const& view = data.views[v];
            Model const* const componentData = (Model*)view.buffers[0];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                DeregisterModelEntity(componentData + i);
            }
        }

        Game::DestroyFilter(filter);
    }
    { // Pointlight cleanup
        Game::Filter filter = Game::FilterBuilder().Including<PointLight>().Build();
        Game::Dataset data = world->Query(filter);

        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::View const& view = data.views[v];
            PointLight const* const componentData = (PointLight*)view.buffers[0];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                DeregisterLight((componentData + i)->graphicsEntityId);
            }
        }

        Game::DestroyFilter(filter);
    }
    { // Spotlight cleanup
        Game::Filter filter = Game::FilterBuilder().Including<SpotLight>().Build();
        Game::Dataset data = world->Query(filter);

        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::View const& view = data.views[v];
            SpotLight const* const componentData = (SpotLight*)view.buffers[0];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                DeregisterLight((componentData + i)->graphicsEntityId);
            }
        }

        Game::DestroyFilter(filter);
    }
    { // AreaLight cleanup
        Game::Filter filter = Game::FilterBuilder().Including<AreaLight>().Build();
        Game::Dataset data = world->Query(filter);

        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::View const& view = data.views[v];
            AreaLight const* const componentData = (AreaLight*)view.buffers[0];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                DeregisterLight((componentData + i)->graphicsEntityId);
            }
        }

        Game::DestroyFilter(filter);
    }
    { // Decal cleanup
        Game::Filter filter = Game::FilterBuilder().Including<Decal>().Build();
        Game::Dataset data = world->Query(filter);

        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::View const& view = data.views[v];
            Decal const* const componentData = (Decal*)view.buffers[0];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                DeregisterDecal((componentData + i)->graphicsEntityId);
            }
        }

        Game::DestroyFilter(filter);
    }
    { // DDGIVolume cleanup
        Game::Filter filter = Game::FilterBuilder().Including<DDGIVolume>().Build();
        Game::Dataset data = world->Query(filter);

        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::View const& view = data.views[v];
            DDGIVolume const* const componentData = (DDGIVolume*)view.buffers[0];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                DeregisterDDGIVolume((componentData + i)->graphicsEntityId);
            }
        }

        Game::DestroyFilter(filter);
    }
    { // Terrain cleanup
        Game::Filter filter = Game::FilterBuilder().Including<Terrain>().Build();
        Game::Dataset data = world->Query(filter);

        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::View const& view = data.views[v];
            Terrain const* const componentData = (Terrain*)view.buffers[0];

            for (IndexT i = 0; i < view.numInstances; ++i)
                DeregisterTerrain((componentData + i)->graphicsEntityId);
        }

        Game::DestroyFilter(filter);
    }
}

} // namespace GraphicsFeature

template <>
void
IO::JsonReader::Get<Graphics::GraphicsEntityId>(Graphics::GraphicsEntityId& ret, char const* attr)
{
    // read nothing
    ret = Graphics::GraphicsEntityId();
}

template <>
void
IO::JsonWriter::Add<Graphics::GraphicsEntityId>(Graphics::GraphicsEntityId const& id, Util::String const& val)
{
    // Write nothing
    return;
}

//------------------------------------------------------------------------------
/**
*/
template <>
void
Game::ComponentDrawFuncT<GraphicsFeature::AreaLightShape>(Game::Entity, Game::ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    static const char* items[] {"Disk", "Rectangle", "Tube"};
    static_assert(sizeof(items) / sizeof(const char*) == (int)GraphicsFeature::AreaLightShape::NumAreaLightShape);
    GraphicsFeature::AreaLightShape* selectedItem = (GraphicsFeature::AreaLightShape*)data;
    if (ImGui::Combo("##AreaShape", (int*)selectedItem, items, IM_ARRAYSIZE(items)))
    {
        *commit = true;
    }
    ImGui::PopID();
}
