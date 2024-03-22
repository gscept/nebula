//------------------------------------------------------------------------------
//  navigationfeature/navigationfeatureunit.cc
//  (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "navigationfeatureunit.h"
#include "streamnavmeshcache.h"
#include "debug/detourdebug.h"
#include "game/api.h"
#include "resources/resourceserver.h"
#include "io/assignregistry.h"
#include "DetourDebugDraw.h"

namespace Navigation
{
StreamNavMeshCache* navMeshCache;
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
NavigationFeatureUnit::OnActivate()
{
    FeatureUnit::OnActivate();

    Resources::ResourceServer::Instance()->RegisterStreamLoader("navmesh", Navigation::StreamNavMeshCache::RTTI);
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("nav", "export:navigation"));

    Navigation::navMeshCache = Resources::GetStreamPool<Navigation::StreamNavMeshCache>();
}

//------------------------------------------------------------------------------
/**
*/
void
NavigationFeatureUnit::OnDeactivate()
{   
    FeatureUnit::OnDeactivate();    
}

//------------------------------------------------------------------------------
/**
*/
void 
NavigationFeatureUnit::OnBeginFrame()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
NavigationFeatureUnit::OnRenderDebug()
{
    Util::Array<Navigation::NavMeshId> meshes = Navigation::navMeshCache->GetLoadedMeshes();
    for (Navigation::NavMeshId id : meshes)
    {
        dtNavMesh* mesh = Navigation::navMeshCache->GetDetourMesh(id);
        Navigation::DebugDraw dd;
        duDebugDrawNavMesh(&dd, *mesh, 0);
    }
}

} // namespace NavigationFeature
