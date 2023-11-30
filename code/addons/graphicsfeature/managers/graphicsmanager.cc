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
RegisterModelEntity(Graphics::GraphicsEntityId const gid, Resources::ResourceName const res, bool const raytracing, Math::mat4 const& t)
{
    Models::ModelContext::RegisterEntity(gid);
    Visibility::ObservableContext::RegisterEntity(gid);
    if (raytracing)
    {
        Raytracing::RaytracingContext::RegisterEntity(gid);
    }
    Models::ModelContext::Setup(
        gid,
        res,
        "NONE",
        [gid, raytracing, t]()
        {
            Models::ModelContext::SetTransform(gid, t);
            if (raytracing)
            {
                Raytracing::RaytracingContext::Setup(gid);
            }
            Visibility::ObservableContext::Setup(gid, Visibility::VisibilityEntityType::Model);
        }
    );
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::InitCreatePointLightProcessor()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    Game::ProcessorBuilder(world, "GraphicsManager.CreatePointLights"_atm)
        .On("OnActivate")
        .Func(
            [](Game::World* world, Game::Entity const& owner, Game::Position const& position, GraphicsFeature::PointLight& light)
            {
                light.graphicsEntityId = Graphics::CreateEntity().id;

                // TODO: This is not finished, and needs revisiting
                Lighting::LightContext::RegisterEntity(light.graphicsEntityId);
                Lighting::LightContext::SetPosition(light.graphicsEntityId, position);
            }
        )
        .Build();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::InitCreateModelProcessor()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    Game::ProcessorBuilder(world, "GraphicsManager.CreateModels"_atm)
        .On("OnActivate")
        .Func(
            [](Game::World* world, Game::Position const& pos, Game::Orientation const& orient, Game::Scale const& scale, GraphicsFeature::Model& model)
            {
                auto res = model.resource;
                model.graphicsEntityId = Graphics::CreateEntity().id;
                Math::mat4 worldTransform = Math::trs(pos, orient, scale);
                RegisterModelEntity(model.graphicsEntityId, model.resource, model.raytracing, worldTransform);
            }
        )
        .Build();
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
        Visibility::ObservableContext::DeregisterEntity(data[i].graphicsEntityId);
        Models::ModelContext::DeregisterEntity(data[i].graphicsEntityId);
        Graphics::DestroyEntity(data[i].graphicsEntityId);
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
                Math::mat4 worldTransform = Math::trs(pos, orient, scale);
                Models::ModelContext::SetTransform(model.graphicsEntityId, worldTransform);
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

    Singleton->InitCreateModelProcessor();
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
    Cleanup all graphics entities
*/
void
GraphicsManager::OnCleanup(Game::World* world)
{
    n_assert(GraphicsManager::HasInstance());

    Game::FilterBuilder::FilterCreateInfo filterInfo;
    filterInfo.inclusive[0] = Game::GetComponentId<Model>();
    filterInfo.access[0] = Game::AccessMode::WRITE;
    filterInfo.numInclusive = 1;

    Game::Filter filter = Game::FilterBuilder::CreateFilter(filterInfo);
    Game::Dataset data = world->Query(filter);

    for (int v = 0; v < data.numViews; v++)
    {
        Game::Dataset::View const& view = data.views[v];
        Model const* const modelData = (Model*)view.buffers[0];

        for (IndexT i = 0; i < view.numInstances; ++i)
        {
            Model const& model = modelData[i];

            if (Models::ModelContext::IsEntityRegistered(model.graphicsEntityId))
            {
                if (Visibility::ObservableContext::IsEntityRegistered(model.graphicsEntityId))
                    Visibility::ObservableContext::DeregisterEntity(model.graphicsEntityId);

                Models::ModelContext::DeregisterEntity(model.graphicsEntityId);
            }

            Graphics::DestroyEntity(model.graphicsEntityId);
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
