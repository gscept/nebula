//------------------------------------------------------------------------------
//  graphicsmanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "graphicsmanager.h"
#include "basegamefeature/managers/entitymanager.h"
#include "graphics/graphicsentity.h"
#include "graphics/graphicsserver.h"
#include "models/modelcontext.h"
#include "visibility/visibilitycontext.h"
#include "game/gameserver.h"

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
void GraphicsManager::InitCreateModelProcessor()
{
    Game::FilterCreateInfo filterInfo;
    filterInfo.inclusive[0] = Game::GetPropertyId("Owner");
    filterInfo.access[0] = Game::AccessMode::READ;
    filterInfo.inclusive[1] = Game::GetPropertyId("WorldTransform");
    filterInfo.access[1] = Game::AccessMode::READ;
    filterInfo.inclusive[2] = Game::GetPropertyId("ModelResource");
    filterInfo.access[2] = Game::AccessMode::READ;
    filterInfo.numInclusive = 3;

    filterInfo.exclusive[0] = this->pids.modelEntityData;
    filterInfo.numExclusive = 1;

    Game::Filter filter = Game::CreateFilter(filterInfo);

    Game::ProcessorCreateInfo processorInfo;
    processorInfo.async = false;
    processorInfo.filter = filter;
    processorInfo.name = "GraphicsManager - CreateModels"_atm;
    processorInfo.OnBeginFrame = [](Game::Dataset data)
    {
        Game::OpBuffer opBuffer = Game::CreateOpBuffer();

        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::CategoryTableView const& view = data.views[v];
            Game::Entity const* const owners = (Game::Entity*)view.buffers[0];
            Math::mat4 const* const transforms = (Math::mat4*)view.buffers[1];
            Resources::ResourceName const* const resources = (Resources::ResourceName*)view.buffers[2];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                Game::Owner const& entity = owners[i];
                Math::mat4 const& t = transforms[i];
                Resources::ResourceName const& res = resources[i];
                Graphics::GraphicsEntityId gid = Graphics::CreateEntity();

                RegisterModelEntity(gid, res, t);

                ModelEntityData mdlData;
                mdlData.gid = gid;

                Game::Op::RegisterProperty regOp;
                regOp.entity = entity;
                regOp.pid = GraphicsManager::Singleton->pids.modelEntityData;
                regOp.value = &mdlData;
                Game::AddOp(opBuffer, regOp);
            }
        }

        // execute ops
        Game::Dispatch(opBuffer);
    };
    
    Game::ProcessorHandle pHandle = Game::CreateProcessor(processorInfo);
}

//------------------------------------------------------------------------------
/**
*/
void GraphicsManager::InitDestroyModelProcessor()
{
    Game::FilterCreateInfo filterInfo;
    filterInfo.inclusive[0] = this->pids.modelEntityData;
    filterInfo.access[0] = Game::AccessMode::READ;
    filterInfo.numInclusive = 1;

    filterInfo.exclusive[0] = Game::GetPropertyId("ModelResource");
    filterInfo.numExclusive = 1;

    Game::Filter filter = Game::CreateFilter(filterInfo);

    Game::ProcessorCreateInfo processorInfo;
    processorInfo.async = false;
    processorInfo.filter = filter;
    processorInfo.name = "GraphicsManager - DestroyModels"_atm;
    processorInfo.OnBeginFrame = [](Game::Dataset data)
    {
        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::CategoryTableView const& view = data.views[v];
            ModelEntityData const* const modelEntityDatas = (ModelEntityData*)view.buffers[0];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                ModelEntityData const& modelEntityData = modelEntityDatas[i];

                // FIXME: Removing graphics entities is broken!
                Models::ModelContext::SetTransform(modelEntityData.gid, Math::translation(0, -10000000.0f, 0));

                //Visibility::ObservableContext::DeregisterEntity(modelEntityData.gid);
                //Models::ModelContext::DeregisterEntity(modelEntityData.gid);
                //Graphics::DestroyEntity(modelEntityData.gid);
            }
        }
    };

    Game::ProcessorHandle pHandle = Game::CreateProcessor(processorInfo);
}

//------------------------------------------------------------------------------
/**
*/
void GraphicsManager::InitUpdateTransformProcessor()
{
	Game::FilterCreateInfo filterInfo;
	filterInfo.inclusive[0] = this->pids.modelEntityData;
	filterInfo.access[0] = Game::AccessMode::READ;
	filterInfo.inclusive[1] = Game::GetPropertyId("WorldTransform"_atm);
	filterInfo.access[1] = Game::AccessMode::READ;
	filterInfo.numInclusive = 2;

	filterInfo.exclusive[0] = Game::GetPropertyId("Static");
	filterInfo.numExclusive = 1;

	Game::Filter filter = Game::CreateFilter(filterInfo);

	Game::ProcessorCreateInfo processorInfo;
	processorInfo.async = false;
	processorInfo.filter = filter;
	processorInfo.name = "GraphicsManager - UpdateTransforms"_atm;
	processorInfo.OnBeginFrame = [](Game::Dataset data)
	{
		for (int v = 0; v < data.numViews; v++)
		{
			Game::Dataset::CategoryTableView const& view = data.views[v];
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
    n_assert(!GraphicsManager::HasInstance());
    GraphicsManager::Singleton = n_new(GraphicsManager);
    
    Singleton->pids.modelEntityData = MemDb::TypeRegistry::Register("ModelEntityData"_atm, ModelEntityData(), Game::PropertyFlags::PROPERTYFLAG_MANAGED);

    Singleton->InitCreateModelProcessor();
    Singleton->InitDestroyModelProcessor();
	Singleton->InitUpdateTransformProcessor();

    Game::ManagerAPI api;
    api.OnBeginFrame = &OnBeginFrame;
    api.OnDeactivate = &Destroy;
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
*/
void
GraphicsManager::OnBeginFrame()
{
    n_assert(GraphicsManager::HasInstance());
}

} // namespace Game
