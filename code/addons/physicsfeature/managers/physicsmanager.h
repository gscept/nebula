#pragma once
//------------------------------------------------------------------------------
/**
    @class  PhysicsFeature::PhysicsManager

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/manager.h"
#include "game/category.h"
#include "game/entity.h"
#include "game/world.h"
#include "basegamefeature/components/basegamefeature.h"
#include "physicsinterface.h"
#include "physics/actorcontext.h"
#include "resources/resourceserver.h"
#include "components/physicsfeature.h"
#include "game/api.h"

namespace PhysicsFeature
{

class PhysicsManager : public Game::Manager
{
    __DeclareClass(PhysicsManager)
    __DeclareSingleton(PhysicsManager)
public:
    PhysicsManager();
    virtual ~PhysicsManager();

    void OnActivate() override;
    void OnDeactivate() override;
    void OnDecay() override;
    void OnCleanup(Game::World* world) override;

    static void InitPhysicsActor(Game::World*, Game::Entity, PhysicsFeature::PhysicsActor*);

private:
    void InitPollTransformProcessor();
};

} // namespace PhysicsFeature
