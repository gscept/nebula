//------------------------------------------------------------------------------
//  navigationfeature/navigationfeatureunit.cc
//  (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "navigationfeatureunit.h"
#include "debug/detourdebug.h"
#include "game/api.h"
#include "resources/resourceserver.h"
#include "io/assignregistry.h"
#include "io/path.h"
#include "DetourDebugDraw.h"
#include "navbuild/navmeshasset.h"
#include "navbuild/geometrysources.h"
#include "components/navigation.h"
#include "managers/navigationmanager.h"
#include "managers/agentmanager.h"
#include "profiling/profiling.h"

namespace Navigation
{
StreamNavMeshCache* navMeshCache;
}

namespace
{
// owned by the feature, a static would be destroyed by exit(), after the heap its table data lives in is gone
Navigation::Build::AreaTable* areaTable = nullptr;
}


namespace NavigationFeature
{
__ImplementClass(NavigationFeature::NavigationFeatureUnit, 'NAFU' , Game::FeatureUnit);
__ImplementSingleton(NavigationFeatureUnit);

//------------------------------------------------------------------------------
/**
*/
NavigationFeatureUnit::NavigationFeatureUnit()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
NavigationFeatureUnit::~NavigationFeatureUnit()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
NavigationFeatureUnit::OnAttach()
{
    this->RegisterComponentType<NavArea>();
    this->RegisterComponentType<NavMesh>({ .OnInit = &NavigationManager::InitNavMesh, .OnDecay = &NavigationManager::DecayNavMesh });
    this->RegisterComponentType<NavAgent>({ .OnInit = &AgentManager::InitAgent, .OnDecay = &AgentManager::DecayAgent });
}

//------------------------------------------------------------------------------
/**
*/
void
NavigationFeatureUnit::OnActivate()
{
    FeatureUnit::OnActivate();

    this->navMeshCache = Navigation::StreamNavMeshCache::Create(); this->navMeshCache->Setup();
    // nav meshes are addressed with URNs in the nav namespace, folder:file:nav, which resolves to nav:folder/file.navmesh.
    // Like physics the loader is also registered by its file extension, which resource reloads look it up by
    Resources::ResourceServer::Instance()->RegisterStreamLoader("nav", this->navMeshCache);
    Resources::ResourceServer::Instance()->RegisterStreamLoader("navmesh", this->navMeshCache);
    Navigation::Build::RegisterPathMappings();
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("nav", "export:navigation"));

    Navigation::navMeshCache = Resources::GetStreamLoader<Navigation::StreamNavMeshCache>();
    ReloadAreaTable();

    this->AttachManager(NavigationManager::Create());
    this->AttachManager(AgentManager::Create());
}

//------------------------------------------------------------------------------
/**
*/
void
NavigationFeatureUnit::OnDeactivate()
{   
    delete areaTable;
    areaTable = nullptr;
    FeatureUnit::OnDeactivate();    
}

//------------------------------------------------------------------------------
/**
*/
void 
NavigationFeatureUnit::OnBeginFrame()
{
    FeatureUnit::OnBeginFrame();
}

//------------------------------------------------------------------------------
/**
*/
static void
DrawLoadedNavMeshes()
{
    N_SCOPE(NavDrawNavMeshes, Navigation);
    if (Navigation::navMeshCache == nullptr)
    {
        return;
    }
    Util::Array<Navigation::NavMeshId> meshes = Navigation::navMeshCache->GetLoadedMeshes();
    for (Navigation::NavMeshId id : meshes)
    {
        // failed loads have no navmesh
        if (id == Navigation::InvalidNavMeshId)
        {
            continue;
        }
        dtNavMesh* mesh = Navigation::navMeshCache->GetDetourMesh(id);
        if (mesh != nullptr)
        {
            Navigation::DebugDraw dd;
            duDebugDrawNavMesh(&dd, *mesh, 0);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
NavigationFeatureUnit::OnRenderDebug()
{
    DrawLoadedNavMeshes();
    DrawAgents();
    FeatureUnit::OnRenderDebug();
}


//------------------------------------------------------------------------------
/**
*/
Navigation::StreamNavMeshCache*
GetNavMeshCache()
{
    return Navigation::navMeshCache;
}

//------------------------------------------------------------------------------
/**
*/
const Navigation::Build::AreaTable&
GetAreaTable()
{
    // the editor may ask before the feature is activated or after it is gone, the built in areas are there anyway
    if (areaTable == nullptr)
    {
        areaTable = new Navigation::Build::AreaTable();
    }
    return *areaTable;
}

//------------------------------------------------------------------------------
/**
*/
void
ReloadAreaTable()
{
    if (areaTable == nullptr)
    {
        areaTable = new Navigation::Build::AreaTable();
    }
    areaTable->Load(IO::URI(Navigation::Build::AreaTableFile));
}

//------------------------------------------------------------------------------
/**
*/
bool
RaycastNavMeshes(const Math::point& start, const Math::vector& dir, float& outT)
{
    N_SCOPE_ACCUM(NavRaycastNavMeshes, Navigation);
    if (Navigation::navMeshCache == nullptr)
    {
        return false;
    }
    bool hit = false;
    outT = FLT_MAX;
    for (Navigation::NavMeshId id : Navigation::navMeshCache->GetLoadedMeshes())
    {
        if (id == Navigation::InvalidNavMeshId)
        {
            continue;
        }
        const dtNavMesh* mesh = Navigation::navMeshCache->GetDetourMesh(id);
        float t;
        if (mesh != nullptr && Navigation::Build::RaycastNavMesh(*mesh, start, dir, t) && t < outT)
        {
            outT = t;
            hit = true;
        }
    }
    return hit;
}

//------------------------------------------------------------------------------
/**
*/
void RenderUI(Graphics::GraphicsEntityId camera)
{
    ImGui::Separator();
    static bool showNavmesh = false;
    ImGui::Checkbox("Render navmeshes and agents", &showNavmesh);
    // the window runs every frame, debug shapes only last for the frame they are drawn in
    if (showNavmesh)
    {
        DrawLoadedNavMeshes();
        DrawAgents();
    }
}

} // namespace NavigationFeature
