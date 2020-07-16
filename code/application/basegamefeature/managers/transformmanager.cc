//------------------------------------------------------------------------------
//  transformmanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "transformmanager.h"
#include "entitymanager.h"

namespace Game
{

void /*TransformManager::*/Create()
{
	Db::StateDescriptor localTransformDescriptor = Game::EntityManager::Instance()->state.worldDatabase->GetStateDescriptor("LocalTransform"_atm);
	Db::StateDescriptor worldTransformDescriptor = Game::EntityManager::Instance()->state.worldDatabase->GetStateDescriptor("WorldTransform"_atm);

	/*
	Game::ProcessorCreateInfo info;

	// can instance specific functions be bundled per category perhaps? (similar to how properties were)
	// this would save a lot of lookups...
	info.functions.OnActivateInstance = &OnActivateInstance;

	info.filter.inclusive =	{ localTransformDescriptor, worldTransformDescriptor }
	info.filter.access =	{ AccessMode::Read,			AccessMode::ReadWrite	 }
	info.filter.exclusive =	{};

	(global) ProcessorId pid = Game::EntityManager::CreateProcessor(info);
	*/
}

void OnActivateInstance(Game::Entity entity, Game::CategoryId category, Game::InstanceId instance, Game::Dataset const* dataset)
{

	((Math::mat4*)(dataset->categories[category.id].buffers[0]))[instance.id] = Math::mat4();
}

} // namespace Game
