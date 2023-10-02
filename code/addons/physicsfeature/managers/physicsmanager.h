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
#include "game/component.h"
#include "physicsinterface.h"
#include "memdb/typeregistry.h"

namespace PhysicsFeature
{

struct PhysicsActor
{
    Physics::ActorId value;
    DECLARE_COMPONENT;
};

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
