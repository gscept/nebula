#pragma once
//------------------------------------------------------------------------------
/**
    @class PhysicsFeature::PhysicsFeatureUnit

    Sets up and interfaces with the physics subsystem.
    
    @copyright
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
#include "game/featureunit.h"
#include "game/frameevent.h"
#include "game/timesource.h"
#include "core/cvar.h"

#define TIMESOURCE_PHYSICS uint32_t('PHTS')

// we aim for 60hz physics
#define PHYSICS_RATE 0.016

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

    void OnAttach() override;

    /// Called upon activation of feature unit
    void OnActivate();
    /// Called upon deactivation of feature unit
    void OnDeactivate();
    /// called from within GameServer::NotifyBeforeCleanup()
    void OnBeforeCleanup(Game::World* world);

    /// called when game debug visualization is on
    virtual void OnRenderDebug();

    /// returns the physics scene belonging to a game world
    IndexT SceneForWorld(Game::World* world);

private:
    /// the fixed frame event the physics step runs on
    Game::FrameEvent* physicsEvent;

    Util::Dictionary<Game::World*, IndexT> physicsWorlds;
    int debugDrawPhysicsLastValue = 0;

    Core::CVar* cl_debug_draw_physics;
};

} // namespace PhysicsFeature
//------------------------------------------------------------------------------
