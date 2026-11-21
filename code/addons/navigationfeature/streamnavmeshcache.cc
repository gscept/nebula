//------------------------------------------------------------------------------
//  streamnavmeshcache.cc
//  (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "streamnavmeshcache.h"
#include "managers/agentmanager.h"
#include "resources/resourceserver.h"
#include "navbuild/navmeshasset.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "profiling/profiling.h"

namespace Navigation
{

__ImplementClass(Navigation::StreamNavMeshCache, 'NVMC', Resources::ResourceLoader);


//------------------------------------------------------------------------------
/**
*/
StreamNavMeshCache::StreamNavMeshCache()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
StreamNavMeshCache::~StreamNavMeshCache()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
dtNavMesh*
StreamNavMeshCache::GetDetourMesh(NavMeshId id)
{
    return this->allocator.Get<Nav_Mesh>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
dtNavMeshQuery*
StreamNavMeshCache::GetQuery(NavMeshId id)
{
    return this->allocator.Get<Nav_Query>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const AgentT&
StreamNavMeshCache::GetAgent(NavMeshId id)
{
    return this->allocator.Get<Nav_Agent>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
void
StreamNavMeshCache::AddNavMeshReloadListener(const std::function<void(Resources::ResourceId)>& callback)
{
    this->AddReloadListener(callback);
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom&
StreamNavMeshCache::GetName(NavMeshId id)
{
    return this->allocator.Get<Nav_Name>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<NavMeshId>
StreamNavMeshCache::GetLoadedMeshes()
{
    Util::Array<NavMeshId> meshes;
    meshes.Reserve(this->ids.Size());
    for (auto const& mesh : this->ids)
    {
        meshes.Append(this->GetId(mesh.Key()));
    }
    return meshes;
}

//------------------------------------------------------------------------------
/**
*/
bool
StreamNavMeshCache::LoadNavMesh(const Ptr<IO::Stream>& stream, const char* name, dtNavMesh*& navMesh, dtNavMeshQuery*& query, AgentT& agent)
{
    N_SCOPE(NavMeshLoadTiles, Navigation);
    n_assert(stream.isvalid());
    n_assert(stream->CanBeMapped());
    navMesh = nullptr;
    query = nullptr;

    const void* buf = stream->MemoryMap();
    const IO::Stream::Size size = stream->GetSize();
    if (!Build::IsBakedNavMesh(buf, size))
    {
        n_warning("StreamNavMeshCache: '%s' is not a baked navmesh\n", name);
        stream->MemoryUnmap();
        return false;
    }
    const BakedNavMesh* baked = GetBakedNavMesh(buf);
    // the tile data is copied, Detour owns it
    navMesh = Build::CreateDetourNavMesh(baked);
    agent = AgentT();
    if (baked->agent_kind() != nullptr)
    {
        baked->agent_kind()->UnPackTo(&agent);
    }
    stream->MemoryUnmap();

    if (navMesh == nullptr)
    {
        n_warning("StreamNavMeshCache: '%s' holds invalid Detour data\n", name);
        return false;
    }
    query = dtAllocNavMeshQuery();
    if (query == nullptr || dtStatusFailed(query->init(navMesh, MAX_NAV_NODES)))
    {
        n_warning("StreamNavMeshCache: could not create a query for '%s'\n", name);
        dtFreeNavMeshQuery(query);
        dtFreeNavMesh(navMesh);
        query = nullptr;
        navMesh = nullptr;
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceLoader::ResourceInitOutput
StreamNavMeshCache::InitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream)
{
    Resources::ResourceLoader::ResourceInitOutput retVal;
    retVal.id = InvalidNavMeshId;

    dtNavMesh* navMesh;
    dtNavMeshQuery* query;
    AgentT agent;
    if (!this->LoadNavMesh(stream, job.name.AsCharPtr(), navMesh, query, agent))
    {
        return retVal;
    }

    NavMeshId ret = { this->allocator.Alloc(), NavMeshIdType };
    this->allocator.Set<Nav_Name>(ret.resourceId, job.name);
    this->allocator.Set<Nav_Mesh>(ret.resourceId, navMesh);
    this->allocator.Set<Nav_Query>(ret.resourceId, query);
    this->allocator.Get<Nav_Agent>(ret.resourceId) = std::move(agent);
    retVal.id = ret;
    return retVal;
}

//------------------------------------------------------------------------------
/**
    A file that fails to load keeps the navmesh that is loaded
*/
Resources::ResourceLoader::ResourceInitOutput
StreamNavMeshCache::ReinitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream)
{
    // the old navmesh is freed, crowds which step on it in a job are done first
    NavigationFeature::AgentManager::WaitForCrowds();
    Resources::ResourceLoader::ResourceInitOutput retVal;
    retVal.id = job.id.resource;

    dtNavMesh* navMesh;
    dtNavMeshQuery* query;
    AgentT agent;
    if (!this->LoadNavMesh(stream, job.name.AsCharPtr(), navMesh, query, agent))
    {
        return retVal;
    }

    const Ids::Id32 id = job.id.resourceId;
    dtNavMesh*& oldMesh = this->allocator.Get<Nav_Mesh>(id);
    dtNavMeshQuery*& oldQuery = this->allocator.Get<Nav_Query>(id);
    dtFreeNavMeshQuery(oldQuery);
    dtFreeNavMesh(oldMesh);
    oldMesh = navMesh;
    oldQuery = query;
    this->allocator.Get<Nav_Agent>(id) = std::move(agent);
    return retVal;
}

//------------------------------------------------------------------------------
/**
*/
void
StreamNavMeshCache::Unload(const Resources::ResourceId res)
{
    NavigationFeature::AgentManager::WaitForCrowds();
    dtNavMesh*& navMesh = this->allocator.Get<Nav_Mesh>(res.resourceId);
    dtNavMeshQuery*& navMeshQuery = this->allocator.Get<Nav_Query>(res.resourceId);
    dtFreeNavMeshQuery(navMeshQuery);
    dtFreeNavMesh(navMesh);
    navMeshQuery = nullptr;
    navMesh = nullptr;
    this->allocator.Get<Nav_Agent>(res.resourceId) = AgentT();
    this->allocator.Dealloc(res.resourceId);
}

} // namespace Navigation
