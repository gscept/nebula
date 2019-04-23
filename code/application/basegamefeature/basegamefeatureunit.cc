//------------------------------------------------------------------------------
//  game/basegamefeature.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "basegamefeature/basegamefeatureunit.h"
#include "appgame/gameapplication.h"
#include "core/factory.h"
#include "game/gameserver.h"
#include "game/gameserver.h"
#include "io/ioserver.h"
#include "io/console.h"
#include "basegamefeature/components/transformcomponent.h"

namespace BaseGameFeature
{
__ImplementClass(BaseGameFeature::BaseGameFeatureUnit, 'GAGF' , Game::FeatureUnit);
__ImplementSingleton(BaseGameFeatureUnit);

using namespace App;
using namespace Game;

//------------------------------------------------------------------------------
/**
*/
BaseGameFeatureUnit::BaseGameFeatureUnit()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
BaseGameFeatureUnit::~BaseGameFeatureUnit()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::OnActivate()
{
	FeatureUnit::OnActivate();
    
	this->entityManager = EntityManager::Create();
	this->componentManager = ComponentManager::Create();
	this->AttachManager(this->entityManager.upcast<Game::Manager>());
	this->AttachManager(this->componentManager.upcast<Game::Manager>());

	Game::TransformComponent::Create();
	Game::TagComponent::Create();

	this->loaderServer = LoaderServer::Create();
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::OnDeactivate()
{
	this->componentManager->ClearAll();

    this->RemoveManager(this->entityManager.upcast<Game::Manager>());
	this->RemoveManager(this->componentManager.upcast<Game::Manager>());

    this->entityManager = nullptr;
	this->componentManager = nullptr;

	Game::TransformComponent::Discard();
	Game::TagComponent::Discard();

	this->loaderServer->Release();
	this->loaderServer = nullptr;

    FeatureUnit::OnDeactivate();
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::OnRenderDebug()
{
    // render debug for all entities and its properties
    FeatureUnit::OnRenderDebug();
}

//------------------------------------------------------------------------------
/**
    Setup a new, empty world.
*/
void
BaseGameFeatureUnit::SetupEmptyWorld()
{
	Game::GameServer::Instance()->NotifyBeforeLoad();
}

//------------------------------------------------------------------------------
/**
    Cleanup the game world. This should undo the stuff in SetupWorld().
    Override this method in a subclass if your app needs different 
    behaviour.
*/
void
BaseGameFeatureUnit::CleanupWorld()
{
	// cleanup

	n_assert2(false, "Not implemented!");
	// this->entityManager->Cleanup();
	// this->componentManager->Cleanup();

	Game::GameServer::Instance()->NotifyBeforeCleanup();            
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::OnEndFrame()
{
    FeatureUnit::OnEndFrame();
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::OnFrame()
{    
    FeatureUnit::OnFrame();
}

} // namespace Game
