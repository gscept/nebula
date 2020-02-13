//------------------------------------------------------------------------------
//  game/basegamefeature.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "basegamefeature/basegamefeatureunit.h"
#include "appgame/gameapplication.h"
#include "core/factory.h"
#include "game/gameserver.h"
#include "io/ioserver.h"
#include "io/console.h"
#include "managers/entitymanager.h"
#include "managers/categorymanager.h"
#include "managers/factorymanager.h"

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
	this->categoryManager = CategoryManager::Create();
	this->factoryManager = FactoryManager::Create();
	this->AttachManager(this->entityManager.upcast<Game::Manager>());
	this->AttachManager(this->categoryManager.upcast<Game::Manager>());
	this->AttachManager(this->factoryManager.upcast<Game::Manager>());
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::OnDeactivate()
{
	this->RemoveManager(this->entityManager.upcast<Game::Manager>());
	this->RemoveManager(this->categoryManager.upcast<Game::Manager>());
	this->RemoveManager(this->factoryManager.upcast<Game::Manager>());
	this->entityManager = nullptr;
	this->categoryManager = nullptr;
	this->factoryManager = nullptr;
	
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
	n_error("Not implemented!");
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
