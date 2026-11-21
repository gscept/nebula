#pragma once
//------------------------------------------------------------------------------
/**
    @class  NavigationFeature::NavigationManager

    Keeps the navmeshes of the level loaded.

    A NavMesh component loads its baked navmesh when it is initialized, and releases it when
    it decays, which also covers a change of its resource in the editor. The loaded navmeshes
    are registered with the entity they belong to, queries and agents find them by the agent
    kind they were baked for.

    A rebake reloads a navmesh in place, the ids stay valid.

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/manager.h"
#include "game/world.h"
#include "navigationfeature/streamnavmeshcache.h"

namespace NavigationFeature
{

struct NavMesh;

/// a navmesh a NavMesh component has loaded
struct ActiveNavMesh
{
    Game::World* world = nullptr;
    Game::Entity entity;
    /// the resource the component holds, released when it decays
    Resources::ResourceId resource = Resources::InvalidResourceId;
    /// the URN it was loaded from
    Resources::ResourceName name;
};

class NavigationManager : public Game::Manager
{
    __DeclareClass(NavigationManager)
    __DeclareSingleton(NavigationManager);
public:
    /// constructor
    NavigationManager();
    /// destructor
    ~NavigationManager();

    /// release the navmeshes of every world
    void OnDeactivate() override;
    /// release the navmeshes of a world which is cleaned up
    void OnCleanup(Game::World* world) override;

    /// load the navmesh of a NavMesh component
    static void InitNavMesh(Game::World* world, Game::Entity entity, NavMesh* component);
    /// release the navmesh of a NavMesh component
    static void DecayNavMesh(Game::World* world, Game::Entity entity, NavMesh* component);

private:
    friend const Util::Array<ActiveNavMesh>& GetActiveNavMeshes();
    Util::Array<ActiveNavMesh> navMeshes;
};

/// the navmeshes NavMesh components have loaded, including ones which failed to load
const Util::Array<ActiveNavMesh>& GetActiveNavMeshes();
/// true if the navmesh is loaded, a failed load has no Detour navmesh
bool IsNavMeshLoaded(const ActiveNavMesh& navMesh);
/// the first loaded navmesh in the world baked for an agent kind, invalid if there is none
Navigation::NavMeshId FindNavMesh(Game::World* world, const Util::StringAtom& agentKind);
/// the navmesh an entity with a NavMesh component has loaded, invalid if it has none or it failed to load
Navigation::NavMeshId GetNavMesh(Game::World* world, Game::Entity entity);

} // namespace NavigationFeature
