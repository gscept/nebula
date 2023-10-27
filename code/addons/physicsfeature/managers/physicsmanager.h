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

class PhysicsManager
{
    __DeclareSingleton(PhysicsManager);
public:
    /// retrieve the api
    static Game::ManagerAPI Create();

    /// destroy entity manager
    static void Destroy();

private:
    /// constructor
    PhysicsManager();
    /// destructor
    ~PhysicsManager();

    void InitCreateActorProcessor();
    void InitPollTransformProcessor();

    static void OnDecay();

    static void OnCleanup(Game::World* world);
};

} // namespace PhysicsFeature
