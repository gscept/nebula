#pragma once
//------------------------------------------------------------------------------
/**
    @class NavigationFeature::NavigationFeatureUnit

        
    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
#include "game/featureunit.h"

//------------------------------------------------------------------------------
namespace NavigationFeature
{

class NavigationFeatureUnit : public Game::FeatureUnit
{
    __DeclareClass(NavigationFeatureUnit)
    __DeclareSingleton(NavigationFeatureUnit)

public:

    /// constructor
    NavigationFeatureUnit();
    /// destructor
    ~NavigationFeatureUnit();

    /// Called upon activation of feature unit
    void OnActivate();
    /// Called upon deactivation of feature unit
    void OnDeactivate();

    /// called on begin of frame
    virtual void OnBeginFrame();

    /// called when game debug visualization is on
    virtual void OnRenderDebug();

private:

};

} // namespace NavigationFeature
//------------------------------------------------------------------------------
