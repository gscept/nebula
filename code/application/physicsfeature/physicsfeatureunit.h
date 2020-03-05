#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::PhysicsFeatureUnit

    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
#include "game/featureunit.h"
#include "physicsfeature/components/physicscomponents.h"

//------------------------------------------------------------------------------
namespace PhysicsFeature
{

class PhysicsFeatureUnit : public Game::FeatureUnit
{
    __DeclareClass(PhysicsFeatureUnit)
    __DeclareSingleton(PhysicsFeatureUnit)

public:

    /// constructor
    PhysicsFeatureUnit();
    /// destructor
    ~PhysicsFeatureUnit();

    /// Called upon activation of feature unit
    void OnActivate();
    /// Called upon deactivation of feature unit
    void OnDeactivate();

    /// called on begin of frame
    virtual void OnBeginFrame();

    /// called when game debug visualization is on
    virtual void OnRenderDebug();
};

} // namespace PhysicsFeature
//------------------------------------------------------------------------------
