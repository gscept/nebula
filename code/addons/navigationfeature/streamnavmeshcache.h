#pragma once
//------------------------------------------------------------------------------
/**
    Implements a resource loader for nav meshes

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "core/refcounted.h"
#include "resources/resourcestreamcache.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "flat/navigation/navmesh.h"

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

class StreamNavMeshCache : public Resources::ResourceStreamCache
{
    __DeclareClass(StreamNavMeshCache);

public:
    
    /// constructor
    StreamNavMeshCache();
    /// destructor
    virtual ~StreamNavMeshCache();

    ///
    dtNavMesh* GetDetourMesh(NavMeshId id);

    ///
    Util::Array<NavMeshId> GetLoadedMeshes();

private:
    /// perform actual load, override in subclass
    LoadStatus LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate) override;
    /// unload resource
    void Unload(const Resources::ResourceId id);

    Ids::IdAllocatorSafe<
        Util::String,
        dtNavMesh*,
        dtNavMeshQuery*,
        NavMeshT> allocator;
    __ImplementResourceAllocatorTypedSafe(allocator, NavMeshIdType);

};
}

