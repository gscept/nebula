#pragma once
//------------------------------------------------------------------------------
/**
    @class  PhysicsFeature::PhysicsManager

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/manager.h"
#include "game/category.h"

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

    struct Pids
    {
        Game::PropertyId physicsActor;
    } pids;
private:
    /// constructor
    PhysicsManager();
    /// destructor
    ~PhysicsManager();

    void InitCreateActorProcessor();
    void InitDestroyActorProcessor();
    void InitPollTransformProcessor();

    static void OnBeginFrame();
};

} // namespace PhysicsFeature
