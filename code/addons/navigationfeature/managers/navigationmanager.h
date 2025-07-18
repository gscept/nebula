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

class NavigationManager : public Game::Manager
{
    __DeclareClass(NavigationManager)
    __DeclareSingleton(NavigationManager);
public:
    
    struct Pids
    {
        Game::ComponentId navigationActor;
    } pids;
private:
    /// constructor
    NavigationManager();
    /// destructor
    ~NavigationManager();

    void InitCreateAgentProcessor();
    void InitUpdateAgentTransformProcessor();

    void OnDecay() override;

    void OnCleanup(Game::World* world) override;
};

} // namespace NavigationFeature
