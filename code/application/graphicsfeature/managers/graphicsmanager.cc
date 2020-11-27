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
#include "game/op.h"
#include "game/gameserver.h"

namespace GraphicsFeature
{

__ImplementSingleton(GraphicsManager)

struct ModelEntityData
{
	// empty for now, only used for filtering
	// TODO: we could replace this with some flagsystem
};

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
Game::ManagerAPI
GraphicsManager::Create()
{
	n_assert(!GraphicsManager::HasInstance());
	GraphicsManager::Singleton = n_new(GraphicsManager);
	
	Singleton->pids.modelEntityData = MemDb::TypeRegistry::Register("ModelEntityData"_atm, ModelEntityData());

	Game::FilterCreateInfo filterInfo;
	filterInfo.inclusive[0] = Game::GetPropertyId("Owner");
	filterInfo.access   [0] = Game::AccessMode::READ;
	filterInfo.inclusive[1] = Game::GetPropertyId("WorldTransform");
	filterInfo.access   [1] = Game::AccessMode::READ;
	filterInfo.inclusive[2] = Game::GetPropertyId("ModelResource");
	filterInfo.access   [2] = Game::AccessMode::READ;
	filterInfo.inclusive[3] = Game::GetPropertyId("GraphicsId");
	filterInfo.access   [3] = Game::AccessMode::READ;
	filterInfo.numInclusive = 4;

	filterInfo.exclusive[0] = Singleton->pids.modelEntityData;
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
			Graphics::GraphicsEntityId* const gids = (Graphics::GraphicsEntityId*)view.buffers[3];

			for (IndexT i = 0; i < view.numInstances; ++i)
			{
				Game::Owner const& entity = owners[i];
				Math::mat4 const& t = transforms[i];
				Resources::ResourceName const& res = resources[i];
				Graphics::GraphicsEntityId& gid = gids[i];

				if (gid == Graphics::GraphicsEntityId::Invalid())
				{
					gid = Graphics::CreateEntity();
				}

				Models::ModelContext::RegisterEntity(gid);
				Visibility::ObservableContext::RegisterEntity(gid);
				Models::ModelContext::Setup(gid, res, "NONE", [gid, t]()
				{
					Models::ModelContext::SetTransform(gid, t);
					Visibility::ObservableContext::Setup(gid, Visibility::VisibilityEntityType::Model);
				});

				Game::Op::RegisterProperty regOp;
				regOp.entity = entity;
				regOp.pid = Singleton->pids.modelEntityData;
				regOp.value = NULL;
				Game::AddOp(opBuffer, regOp);
			}
		}

		// execute ops
		Game::Dispatch(opBuffer);
	};
	processorInfo.OnDeactivate = &Destroy;

	Game::ProcessorHandle pHandle = Game::CreateProcessor(processorInfo);

	Game::ManagerAPI api;
	//api.OnBeginFrame = &OnBeginFrame;
	//api.OnDeactivate = &Destroy;
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
