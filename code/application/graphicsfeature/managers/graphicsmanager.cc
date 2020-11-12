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
	
	Singleton->pids.owner			= Game::GetPropertyId("Owner"_atm);
	Singleton->pids.worldTransform	= Game::GetPropertyId("WorldTransform"_atm);
	Singleton->pids.modelResource	= Game::GetPropertyId("ModelResource"_atm);
	Singleton->pids.graphicsId		= Game::GetPropertyId("GraphicsId"_atm);
	Singleton->pids.modelEntityData = MemDb::TypeRegistry::Register("ModelEntityData"_atm, ModelEntityData());

	Singleton->addPropertyMsgQueue = Msg::AddProperty::AllocateMessageQueue();

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
	Msg::AddProperty::DeAllocateMessageQueue(Singleton->addPropertyMsgQueue);

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

	InitModelEntities();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsManager::InitModelEntities()
{
	Game::FilterSet filter;
	filter.inclusive = {
		Singleton->pids.owner,
		Singleton->pids.worldTransform,
		Singleton->pids.modelResource,
		Singleton->pids.graphicsId
	};
	filter.exclusive = {
		Singleton->pids.modelEntityData
	};

	Game::Dataset const data = Game::Query(filter);

	for (auto const& tbl : data.tables)
	{
		Game::Entity const* const owners = (Game::Entity*)tbl.buffers[0];
		Math::mat4 const* const transforms = (Math::mat4*)tbl.buffers[1];
		Resources::ResourceName const* const resources = (Resources::ResourceName*)tbl.buffers[2];
		Graphics::GraphicsEntityId* const gids = (Graphics::GraphicsEntityId*)tbl.buffers[3];

		for (IndexT i = 0; i < tbl.numInstances; ++i)
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
			
			Msg::AddProperty::Defer(Singleton->addPropertyMsgQueue, entity, Singleton->pids.modelEntityData);
		}
	}

	// Dispatch queue internally
	auto& queue = Msg::AddProperty::GetMessageQueue(Singleton->addPropertyMsgQueue);
	SizeT queueSize = queue.Size();
	for (IndexT i = 0; i < queueSize; i++)
	{
		auto& tuple = queue.Get<0>(i);
		Game::AddProperty(Util::Get<0>(tuple), Util::Get<1>(tuple));
	}
	// clear queue
	queue.Clear();
}

} // namespace Game
