#pragma once
//------------------------------------------------------------------------------
/**
    @class  NavigationFeature::NavigationManager

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/manager.h"
#include "game/category.h"

namespace NavigationFeature
{

class NavigationManager
{
    __DeclareSingleton(NavigationManager);
public:
    /// retrieve the api
    static Game::ManagerAPI Create();

    /// destroy entity manager
    static void Destroy();

    struct Pids
    {
        Game::PropertyId navigationActor;
    } pids;
private:
    /// constructor
    NavigationManager();
    /// destructor
    ~NavigationManager();

    void InitCreateAgentProcessor();
    void InitUpdateAgentTransformProcessor();

    static void OnDecay();

    static void OnCleanup(Game::World* world);
};

} // namespace NavigationFeature
