//------------------------------------------------------------------------------
//  navigationmanager.cc
//  (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "navigationmanager.h"
#include "agentmanager.h"
#include "components/navigation.h"
#include "resources/resourceserver.h"
#include "io/path.h"
#include "profiling/profiling.h"

namespace Navigation
{
extern StreamNavMeshCache* navMeshCache;
}

namespace NavigationFeature
{
__ImplementClass(NavigationFeature::NavigationManager, 'NvMa', Game::Manager);
__ImplementSingleton(NavigationManager)

//------------------------------------------------------------------------------
/**
*/
NavigationManager::NavigationManager()
{
    __ConstructSingleton
}

//------------------------------------------------------------------------------
/**
*/
NavigationManager::~NavigationManager()
{
    __DestructSingleton
}

//------------------------------------------------------------------------------
/**
*/
static IndexT
FindActive(const Util::Array<ActiveNavMesh>& navMeshes, Game::World* world, Game::Entity entity)
{
    for (IndexT i = 0; i < navMeshes.Size(); i++)
    {
        if (navMeshes[i].world == world && navMeshes[i].entity == entity)
        {
            return i;
        }
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
static void
ReleaseNavMesh(ActiveNavMesh& navMesh)
{
    if (navMesh.resource != Resources::InvalidResourceId)
    {
        // the navmesh may go with the resource, crowds which step on it in a job are done first
        AgentManager::WaitForCrowds();
        Resources::DiscardResource(navMesh.resource);
        navMesh.resource = Resources::InvalidResourceId;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
NavigationManager::InitNavMesh(Game::World* world, Game::Entity entity, NavMesh* component)
{
    N_SCOPE(NavMeshInit, Navigation);
    if (!NavigationManager::HasInstance())
    {
        return;
    }

    Util::Array<ActiveNavMesh>& navMeshes = Singleton->navMeshes;

    // a component is initialized again without a decay when it is replaced, let go of what it had
    const IndexT existing = FindActive(navMeshes, world, entity);
    if (existing != InvalidIndex)
    {
        ReleaseNavMesh(navMeshes[existing]);
        navMeshes.EraseIndex(existing);
    }

    // an entity which was just created in the editor has no navmesh yet
    if (!component->resource.IsValid() || component->resource.Value()[0] == '\0')
    {
        return;
    }

    const IO::Path path = IO::Path::Parse(component->resource.Value());
    if (!path.IsFile() || path.GetType() != "nav")
    {
        n_warning("NavigationManager: '%s' is not a navmesh URN (folder:file:nav)\n", component->resource.Value());
        return;
    }

    ActiveNavMesh navMesh;
    navMesh.world = world;
    navMesh.entity = entity;
    navMesh.name = component->resource;
    navMesh.resource = Resources::CreateResource(path, "NavMesh", nullptr, nullptr, true);
    if (navMesh.resource == Resources::InvalidResourceId)
    {
        n_warning("NavigationManager: could not load navmesh '%s'\n", component->resource.Value());
        return;
    }
    // a failed load is kept, it is released with the component like a loaded one
    if (!IsNavMeshLoaded(navMesh))
    {
        n_warning("NavigationManager: navmesh '%s' failed to load, is it baked?\n", component->resource.Value());
    }
    navMeshes.Append(navMesh);
}

//------------------------------------------------------------------------------
/**
*/
void
NavigationManager::DecayNavMesh(Game::World* world, Game::Entity entity, NavMesh*)
{
    if (!NavigationManager::HasInstance())
    {
        return;
    }

    Util::Array<ActiveNavMesh>& navMeshes = Singleton->navMeshes;
    const IndexT index = FindActive(navMeshes, world, entity);
    if (index != InvalidIndex)
    {
        ReleaseNavMesh(navMeshes[index]);
        navMeshes.EraseIndex(index);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
NavigationManager::OnDeactivate()
{
    for (ActiveNavMesh& navMesh : this->navMeshes)
    {
        ReleaseNavMesh(navMesh);
    }
    this->navMeshes.Clear();
    Game::Manager::OnDeactivate();
}

//------------------------------------------------------------------------------
/**
    Removing the entities of a world doesn't decay their components
*/
void
NavigationManager::OnCleanup(Game::World* world)
{
    for (IndexT i = this->navMeshes.Size() - 1; i >= 0; i--)
    {
        if (this->navMeshes[i].world == world)
        {
            ReleaseNavMesh(this->navMeshes[i]);
            this->navMeshes.EraseIndex(i);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<ActiveNavMesh>&
GetActiveNavMeshes()
{
    static const Util::Array<ActiveNavMesh> none;
    if (!NavigationManager::HasInstance())
    {
        return none;
    }
    return NavigationManager::Instance()->navMeshes;
}

//------------------------------------------------------------------------------
/**
*/
bool
IsNavMeshLoaded(const ActiveNavMesh& navMesh)
{
    if (navMesh.resource == Resources::InvalidResourceId || Navigation::navMeshCache == nullptr)
    {
        return false;
    }
    if (Resources::ResourceServer::Instance()->GetState(navMesh.resource) != Resources::Resource::Loaded)
    {
        return false;
    }
    return Navigation::navMeshCache->GetDetourMesh(navMesh.resource) != nullptr;
}

//------------------------------------------------------------------------------
/**
*/
Navigation::NavMeshId
FindNavMesh(Game::World* world, const Util::StringAtom& agentKind)
{
    for (const ActiveNavMesh& navMesh : GetActiveNavMeshes())
    {
        if (navMesh.world != world || !IsNavMeshLoaded(navMesh))
        {
            continue;
        }
        const Navigation::AgentT& agent = Navigation::navMeshCache->GetAgent(navMesh.resource);
        if (agentKind == Util::StringAtom(agent.name.c_str()))
        {
            return navMesh.resource;
        }
    }
    return Navigation::InvalidNavMeshId;
}

//------------------------------------------------------------------------------
/**
*/
Navigation::NavMeshId
GetNavMesh(Game::World* world, Game::Entity entity)
{
    for (const ActiveNavMesh& navMesh : GetActiveNavMeshes())
    {
        if (navMesh.world == world && navMesh.entity == entity)
        {
            return IsNavMeshLoaded(navMesh) ? Navigation::NavMeshId(navMesh.resource) : Navigation::InvalidNavMeshId;
        }
    }
    return Navigation::InvalidNavMeshId;
}

} // namespace NavigationFeature
