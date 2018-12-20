#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::GraphicsFeatureUnit

    (C) 2018 Individual contributors, see AUTHORS file
*/
#include "game/featureunit.h"
#include "graphicsfeature/components/graphicscomponent.h"

//------------------------------------------------------------------------------
namespace GraphicsFeature
{

class GraphicsFeatureUnit : public Game::FeatureUnit    
{
	__DeclareClass(GraphicsFeatureUnit)
	__DeclareSingleton(GraphicsFeatureUnit)

public:

    /// constructor
    GraphicsFeatureUnit();
    /// destructor
    ~GraphicsFeatureUnit();

	/// Called upon activation of feature unit
	void OnActivate();
	/// Called upon deactivation of feature unit
	void OnDeactivate();
};

} // namespace GraphicsFeature
//------------------------------------------------------------------------------
