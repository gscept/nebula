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
#include "graphics/cameracontext.h"
#include "game/gameserver.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "graphicsfeature/components/graphics.h"
#include "basegamefeature/components/transform.h"

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
    Game::Filter filter = Game::FilterBuilder()
        .Including<
            const Game::Owner,
            const Game::WorldTransform,
            const ModelResource>()
        .Excluding({ this->pids.modelEntityData })
        .Build();

    Game::ProcessorCreateInfo processorInfo;
    processorInfo.async = false;
    processorInfo.filter = filter;
    processorInfo.name = "GraphicsManager.CreateModels"_atm;
    processorInfo.OnEndFrame = [](Game::World* world, Game::Dataset data)
    {
        Game::OpBuffer opBuffer = Game::CreateOpBuffer(world);

        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::EntityTableView const& view = data.views[v];
            Game::Entity const* const owners = (Game::Entity*)view.buffers[0];
            Math::mat4 const* const transforms = (Math::mat4*)view.buffers[1];
            Resources::ResourceName const* const resources = (Resources::ResourceName*)view.buffers[2];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                Game::Entity const& entity = owners[i];
                Math::mat4 const& t = transforms[i];
                Resources::ResourceName const& res = resources[i];
                Graphics::GraphicsEntityId gid = Graphics::CreateEntity();
                RegisterModelEntity(gid, res, t);
                ModelEntityData mdlData;
                mdlData.gid = gid;

                Game::Op::RegisterComponent regOp;
                regOp.entity = entity;
                regOp.component = GraphicsManager::Singleton->pids.modelEntityData;
                regOp.value = &mdlData;
                Game::AddOp(opBuffer, regOp);
            }
        }
        Game::Dispatch(opBuffer);
        Game::DestroyOpBuffer(opBuffer);
    };
    
    Game::ProcessorHandle pHandle = Game::CreateProcessor(processorInfo);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::OnDecay()
{
    Game::ComponentDecayBuffer const decayBuffer = Game::GetDecayBuffer(Singleton->pids.modelEntityData);
    ModelEntityData* data = (ModelEntityData*)decayBuffer.buffer;
    for (int i = 0; i < decayBuffer.size; i++)
    {
        Visibility::ObservableContext::DeregisterEntity(data[i].gid);
        Models::ModelContext::DeregisterEntity(data[i].gid);
        Graphics::DestroyEntity(data[i].gid);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::InitUpdateModelTransformProcessor()
{

    Game::Filter filter = Game::FilterBuilder()
        .Including({ {Game::AccessMode::READ, this->pids.modelEntityData} })
        .Including<Game::WorldTransform>()
        .Excluding({ Game::GetComponentId("Static") })
        .Build();

    Game::ProcessorCreateInfo processorInfo;
    processorInfo.async = false;
    processorInfo.filter = filter;
    processorInfo.name = "GraphicsManager.UpdateModelTransforms"_atm;
    processorInfo.OnBeginFrame = [](Game::World*, Game::Dataset data)
    {
        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::EntityTableView const& view = data.views[v];
            ModelEntityData const* const modelEntityDatas = (ModelEntityData*)view.buffers[0];
            Math::mat4 const* const transforms = (Math::mat4*)view.buffers[1];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                ModelEntityData const& modelEntityData = modelEntityDatas[i];
                Math::mat4 const& transform = transforms[i];

                Models::ModelContext::SetTransform(modelEntityData.gid, transform);
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
    GraphicsManager::Singleton = n_new(GraphicsManager);

    
    Game::ComponentCreateInfo info;
    info.name = "ModelEntityData";
    ModelEntityData defaultValue;
    info.defaultValue = &defaultValue;
    info.flags = Game::ComponentFlags::COMPONENTFLAG_MANAGED;
    info.byteSize = sizeof(ModelEntityData);
    Singleton->pids.modelEntityData = Game::CreateComponent(info);

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
    n_delete(GraphicsManager::Singleton);
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
    filterInfo.inclusive[0] = Singleton->pids.modelEntityData;
    filterInfo.access[0] = Game::AccessMode::WRITE;
    filterInfo.numInclusive = 1;

    Game::Filter filter = Game::FilterBuilder::CreateFilter(filterInfo);
    Game::Dataset data = Game::Query(world, filter);

    for (int v = 0; v < data.numViews; v++)
    {
        Game::Dataset::EntityTableView const& view = data.views[v];
        ModelEntityData const* const modelEntityDatas = (ModelEntityData*)view.buffers[0];
        
        for (IndexT i = 0; i < view.numInstances; ++i)
        {
            ModelEntityData const& modelEntityData = modelEntityDatas[i];
            
            if (Models::ModelContext::IsEntityRegistered(modelEntityData.gid))
            {
                if (Visibility::ObservableContext::IsEntityRegistered(modelEntityData.gid))
                    Visibility::ObservableContext::DeregisterEntity(modelEntityData.gid);

                Models::ModelContext::DeregisterEntity(modelEntityData.gid);
            }
            
            Graphics::DestroyEntity(modelEntityData.gid);
        }
    }

    Game::DestroyFilter(filter);
}

} // namespace Game
