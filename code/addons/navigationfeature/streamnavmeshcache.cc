//------------------------------------------------------------------------------
//  streamnavmeshcache.cc
//  (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "streamnavmeshcache.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"

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
    return this->allocator.Get<1>(id.resourceId);
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
Resources::ResourceUnknownId StreamNavMeshCache::InitializeResource(const Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    n_assert(stream.isvalid());
    n_assert(stream->CanBeMapped());
    if (!stream->Open())
    {
        return InvalidNavMeshId;
    }
   
    void* buf = stream->Map();
    if (buf == nullptr)
    {
        return InvalidNavMeshId;
    }

    NavMeshId ret = { this->allocator.Alloc(), NavMeshIdType };

    NavMeshT& meshInfo = this->allocator.Get<Nav_MeshInfo>(ret.resourceId);
    dtNavMesh*& navMesh = this->allocator.Get<Nav_Mesh>(ret.resourceId);
    dtNavMeshQuery*& navMeshQuery = this->allocator.Get<Nav_Query>(ret.resourceId);


    Flat::FlatbufferInterface::DeserializeFlatbuffer<Navigation::NavMesh>(meshInfo, (uint8_t*)buf);
    stream->Unmap();

    this->allocator.Set<Nav_Name>(ret.resourceId, meshInfo.name);

    Ptr<IO::Stream> storedNavMesh = IO::IoServer::Instance()->CreateStream(meshInfo.file);
    if (storedNavMesh->Open())
    {
        unsigned char* navData = (unsigned char*)storedNavMesh->Map();
        IO::Stream::Size navDataSize = storedNavMesh->GetSize();
        navMesh = dtAllocNavMesh();
        navMeshQuery = dtAllocNavMeshQuery();
        if (DT_SUCCESS == navMesh->init(navData, navDataSize, 0))
        {
            navMeshQuery->init(navMesh, MAX_NAV_NODES);
            return ret;
        }
        storedNavMesh->Unmap();
    }
    return InvalidNavMeshId;
}

//------------------------------------------------------------------------------
/**
*/
void
StreamNavMeshCache::Unload(const Resources::ResourceId res)
{
    dtNavMesh*& navMesh = this->allocator.Get<Nav_Mesh>(res.resourceId);
    dtNavMeshQuery*& navMeshQuery = this->allocator.Get<Nav_Query>(res.resourceId);
    dtFreeNavMesh(navMesh);
    dtFreeNavMeshQuery(navMeshQuery);
    this->allocator.Dealloc(res.resourceId);
}

}
