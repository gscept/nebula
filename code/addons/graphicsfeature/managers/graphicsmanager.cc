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
#include "graphicsfeature/components/graphicsfeature.h"
#include "basegamefeature/components/basegamefeature.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "io/jsonreader.h"
#include "io/jsonwriter.h"
#include "lighting/lightcontext.h"

namespace GraphicsFeature
{

__ImplementSingleton(GraphicsManager)

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
    Graphics::GraphicsEntityId const gid
    , Resources::ResourceName const res
    , Resources::ResourceName const anim
    , Resources::ResourceName const skeleton
    , bool const raytracing
    , Math::mat4 const& t
)
{
    Models::ModelContext::RegisterEntity(gid);
    if (raytracing && CoreGraphics::RayTracingSupported)
    {
        Raytracing::RaytracingContext::RegisterEntity(gid);
    }
    if (anim.IsValid() && skeleton.IsValid())
    {
        Characters::CharacterContext::RegisterEntity(gid);
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
            if (raytracing && CoreGraphics::RayTracingSupported)
            {
                Raytracing::RaytracingContext::SetupModel(gid, CoreGraphics::BlasInstanceFlags::NoFlags, 0xFF);
            }
            if (anim.IsValid() && skeleton.IsValid())
            {
                Characters::CharacterContext::Setup(gid, skeleton, 0, anim, 0, "NONE");
                Characters::CharacterContext::PlayClip(gid, nullptr, 0, 0, Characters::EnqueueMode::Replace);
            }
            Visibility::ObservableContext::Setup(gid, Visibility::VisibilityEntityType::Model);
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
GraphicsManager::OnDecay()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);

    Game::ComponentDecayBuffer const decayBuffer = world->GetDecayBuffer(Game::GetComponentId<Model>());
    Model* data = (Model*)decayBuffer.buffer;
    for (int i = 0; i < decayBuffer.size; i++)
    {
        DeregisterModelEntity(data + i);
    }
    Game::ComponentDecayBuffer const lightDecayBuffer = world->GetDecayBuffer(Game::GetComponentId<PointLight>());
    PointLight* lightData = (PointLight*)lightDecayBuffer.buffer;
    for (int i = 0; i < lightDecayBuffer.size; i++)
    {
        PointLight* light = lightData + i;
        if ((Graphics::GraphicsEntityId)light->graphicsEntityId == Graphics::InvalidGraphicsEntityId)
        {
            continue;
        }
        if (Lighting::LightContext::IsEntityRegistered(light->graphicsEntityId))
        {
            Lighting::LightContext::DeregisterEntity(light->graphicsEntityId);
        }
        Graphics::DestroyEntity(light->graphicsEntityId);
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
            [](Game::World* world, Game::Position const& pos, Game::Orientation const& orient, Game::Scale const& scale, GraphicsFeature::Model const& model)
            {
                if (model.graphicsEntityId != -1)
                {
                    Math::mat4 worldTransform = Math::trs(pos, orient, scale);
                    Models::ModelContext::SetTransform(model.graphicsEntityId, worldTransform);
                }
            }
        )
        .Build();
}

//------------------------------------------------------------------------------
/**
*/
Game::ManagerAPI
GraphicsManager::Create()
{
    n_assert(!GraphicsManager::HasInstance());
    GraphicsManager::Singleton = new GraphicsManager;

    Singleton->InitUpdateModelTransformProcessor();

    Game::ManagerAPI api;
    api.OnCleanup = &OnCleanup;
    api.OnDeactivate = &Destroy;
    api.OnDecay = &OnDecay;
    return api;
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::Destroy()
{
    delete GraphicsManager::Singleton;
    GraphicsManager::Singleton = nullptr;
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
    Lighting::LightContext::SetupPointLight(light->graphicsEntityId, light->color, light->intensity, light->range, light->castShadows);
    Lighting::LightContext::SetPosition(light->graphicsEntityId, pos);
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
    RegisterModelEntity(model->graphicsEntityId, model->resource, model->anim, model->skeleton, model->raytracing, worldTransform);
}

//------------------------------------------------------------------------------
/**
    Cleanup all graphics entities
*/
void
GraphicsManager::OnCleanup(Game::World* world)
{
    n_assert(GraphicsManager::HasInstance());

    Game::Filter filter = Game::FilterBuilder().Including<Model>().Build();
    Game::Dataset data = world->Query(filter);

    for (int v = 0; v < data.numViews; v++)
    {
        Game::Dataset::View const& view = data.views[v];
        Model const* const modelData = (Model*)view.buffers[0];

        for (IndexT i = 0; i < view.numInstances; ++i)
        {
            DeregisterModelEntity(modelData + i);
        }
    }

    Game::DestroyFilter(filter);
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
