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
#include "lighting/lightcontext.h"
#include "game/componentinspection.h"
#include "decals/decalcontext.h"
#include "gi/ddgicontext.h"

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
*/
void
RegisterModelEntity(
    Graphics::GraphicsEntityId const gid,
    Resources::ResourceName const res,
    Resources::ResourceName const anim,
    Resources::ResourceName const skeleton,
    bool const raytracing,
    Math::mat4 const& t
)
{
    Models::ModelContext::RegisterEntity(gid);
    if (raytracing && CoreGraphics::RayTracingSupported)
    {
        Raytracing::RaytracingContext::RegisterEntity(gid);
    }
    Models::ModelContext::Setup(
        gid,
        res,
        "NONE",
        [gid, anim, skeleton, raytracing, t]()
        {
            if (!Graphics::GraphicsServer::Instance()->IsValidGraphicsEntity(gid))
                return;
            Visibility::ObservableContext::RegisterEntity(gid);
            Models::ModelContext::SetTransform(gid, t);
            Visibility::ObservableContext::Setup(gid, Visibility::VisibilityEntityType::Model);
            if (raytracing && CoreGraphics::RayTracingSupported)
            {
                Raytracing::RaytracingContext::SetupModel(gid, CoreGraphics::BlasInstanceFlags::NoFlags, 0xFF);
            }
            if (anim.IsValid() && skeleton.IsValid())
            {
                Characters::CharacterContext::RegisterEntity(gid);
                Characters::CharacterContext::Setup(gid, skeleton, 0, anim, 0, "NONE");
                Characters::CharacterContext::PlayClip(gid, nullptr, 0, 0, Characters::EnqueueMode::Replace);
            }
        }
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
        light->graphicsEntityId, light->color.vec, light->intensity, light->range, light->castShadows
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
        light->color.vec,
        light->intensity,
        Math::deg2rad(light->innerConeAngle),
        Math::deg2rad(light->outerConeAngle),
        light->range,
        light->castShadows
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
        (Lighting::LightContext::AreaLightShape)light->shape,
        light->color.vec,
        light->intensity,
        light->range,
        light->twoSided,
        light->castShadows,
        light->renderMesh
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
    GI::DDGIContext::SetupVolume(volume->graphicsEntityId, setup);
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
        model->graphicsEntityId, model->resource, model->anim, model->skeleton, model->raytracing, worldTransform
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
