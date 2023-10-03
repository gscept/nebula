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
#include "game/gameserver.h"
#include "graphicsfeature/components/graphics.h"
#include "basegamefeature/components/transform.h"
#include "io/jsonreader.h"
#include "io/jsonwriter.h"

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
RegisterModelEntity(Graphics::GraphicsEntityId const gid, Resources::ResourceName const res, Math::mat4 const& t)
{
    Models::ModelContext::RegisterEntity(gid);
    Visibility::ObservableContext::RegisterEntity(gid);
    Models::ModelContext::Setup(gid, res, "NONE", [gid, t]()
    {
        Models::ModelContext::SetTransform(gid, t);
        Visibility::ObservableContext::Setup(gid, Visibility::VisibilityEntityType::Model);
    });
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::InitCreateModelProcessor()
{
    Game::ProcessorBuilder("GraphicsManager.CreateModels"_atm)
        .On("OnActivate")
        .Func(
            [](Game::World* world,
               Game::Owner const& owner,
               Game::WorldTransform const& t,
               GraphicsFeature::Model& model)
            {
                auto res = model.resource;
                model.graphicsEntityId = Graphics::CreateEntity();
                RegisterModelEntity(model.graphicsEntityId, model.resource, t.value);
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
    Game::ComponentDecayBuffer const decayBuffer = Game::GetDecayBuffer(Model::ID());
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

    Game::Filter filter = Game::FilterBuilder()
        .Including({ {Game::AccessMode::READ, Model::ID()} })
        .Including<Game::WorldTransform>()
        .Excluding({ Game::GetComponentId("Static") })
        .Build();

    Game::ProcessorCreateInfo processorInfo;
    processorInfo.async = false;
    processorInfo.filter = filter;
    processorInfo.name = "GraphicsManager.UpdateModelTransforms"_atm;
    processorInfo.OnBeginFrame = [](Game::World* world, Game::Dataset data)
    {
        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::EntityTableView const& view = data.views[v];

            // TODO: check if any entitys transform is modified in the partition, and skip otherwise.

            Model const* const modelData = (Model*)view.buffers[0];
            Math::mat4 const* const transforms = (Math::mat4*)view.buffers[1];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                Model const& model = modelData[i];
                Math::mat4 const& transform = transforms[i];
                Models::ModelContext::SetTransform(model.graphicsEntityId, transform);
            }
        }
    };

    Game::ProcessorHandle pHandle = Game::CreateProcessor(processorInfo);
}

//------------------------------------------------------------------------------
/**
*/
Game::ManagerAPI
GraphicsManager::Create()
{
	n_assert(GraphicsFeature::Details::graphics_registered);
    n_assert(!GraphicsManager::HasInstance());
    GraphicsManager::Singleton = new GraphicsManager;

    Singleton->InitCreateModelProcessor();
    Singleton->InitUpdateModelTransformProcessor();

    Game::ManagerAPI api;
    api.OnCleanup    = &OnCleanup;
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
    filterInfo.inclusive[0] = GraphicsFeature::Model::ID();
    filterInfo.access[0] = Game::AccessMode::WRITE;
    filterInfo.numInclusive = 1;

    Game::Filter filter = Game::FilterBuilder::CreateFilter(filterInfo);
    Game::Dataset data = Game::Query(world, filter);

    for (int v = 0; v < data.numViews; v++)
    {
        Game::Dataset::EntityTableView const& view = data.views[v];
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

} // namespace Game

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
