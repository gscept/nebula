//------------------------------------------------------------------------------
//  graphicsfeature/graphicsfeatureunit.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "graphicsfeature/graphicsfeatureunit.h"

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

	GraphicsComponent::Create();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeatureUnit::OnDeactivate()
{	
    FeatureUnit::OnDeactivate();

	GraphicsComponent::Discard();
}

} // namespace Game
