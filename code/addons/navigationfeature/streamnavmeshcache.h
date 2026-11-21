#pragma once
//------------------------------------------------------------------------------
/**
    Implements a resource loader for baked nav meshes

    Loads Navigation::BakedNavMesh files, the export of a navmesh work file. Nav meshes are
    addressed with URNs of the nav type, folder:file:nav.

    A reload replaces the Detour navmesh and its query in place, the id stays the same.
    Anything that keeps pointers into the old navmesh, polygon refs, queries or crowds,
    has to be rebuilt in a reload listener, see ResourceLoader::AddReloadListener.

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "core/refcounted.h"
#include "resources/resourceloader.h"
#include "flat/navigation/agentkinds.h"
#include "ids/idallocator.h"

class dtNavMesh;
class dtNavMeshQuery;

#define MAX_NAV_NODES 8192

namespace Navigation
{

enum NavigationIdType
{
    NavMeshIdType
};

RESOURCE_ID_TYPE(NavMeshId);

class StreamNavMeshCache : public Resources::ResourceLoader
{
    __DeclareClass(StreamNavMeshCache);

public:

    /// constructor
    StreamNavMeshCache();
    /// destructor
    virtual ~StreamNavMeshCache();

    /// get the Detour navmesh, null if the resource failed to load
    dtNavMesh* GetDetourMesh(NavMeshId id);
    /// get the query on the navmesh, null if the resource failed to load
    dtNavMeshQuery* GetQuery(NavMeshId id);
    /// get the agent kind the navmesh was baked for
    const AgentT& GetAgent(NavMeshId id);
    /// the name the navmesh was loaded by
    const Util::StringAtom& GetName(NavMeshId id);

    ///
    Util::Array<NavMeshId> GetLoadedMeshes();
    /// call a function after a navmesh was reloaded, for whatever points into the old one
    void AddNavMeshReloadListener(const std::function<void(Resources::ResourceId)>& callback);

private:

    /// perform actual load, override in subclass
    ResourceLoader::ResourceInitOutput InitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream) override;
    /// replace the navmesh of a loaded resource with the reloaded file
    ResourceLoader::ResourceInitOutput ReinitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream) override;
    /// unload resource
    void Unload(const Resources::ResourceId id) override;

    /// create the Detour navmesh and query from a baked navmesh file
    bool LoadNavMesh(const Ptr<IO::Stream>& stream, const char* name, dtNavMesh*& navMesh, dtNavMeshQuery*& query, AgentT& agent);

    enum
    {
        Nav_Name,
        Nav_Mesh,
        Nav_Query,
        Nav_Agent
    };
    Ids::IdAllocatorSafe<0xff,
        Util::StringAtom,
        dtNavMesh*,
        dtNavMeshQuery*,
        AgentT> allocator;
};
}

