//------------------------------------------------------------------------------
//  graphicsfeature/graphicsfeatureunit.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "basegamefeature/managers/componentmanager.h"

namespace GraphicsFeature
{
__ImplementClass(GraphicsFeatureUnit, 'FXFU' , Game::FeatureUnit);
__ImplementSingleton(GraphicsFeatureUnit);

//------------------------------------------------------------------------------
/**
*/
GraphicsFeatureUnit::GraphicsFeatureUnit()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
GraphicsFeatureUnit::~GraphicsFeatureUnit()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnActivate()
{
	FeatureUnit::OnActivate();

	Ptr<Game::ComponentManager> componentManager = Game::ComponentManager::Instance();
	this->graphicsComponent = GraphicsComponent::Create();
	componentManager->RegisterComponent(this->graphicsComponent.upcast<Game::BaseComponent>());	
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnDeactivate()
{
	Ptr<Game::ComponentManager> componentManager = Game::ComponentManager::Instance();
	componentManager->DeregisterComponent(this->graphicsComponent);
	this->graphicsComponent = nullptr;
	
    FeatureUnit::OnDeactivate();
}

} // namespace Game
