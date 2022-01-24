//------------------------------------------------------------------------------
//  streamnavmeshpool.cc
//  (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "streamnavmeshpool.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"

namespace Navigation
{

__ImplementClass(Navigation::StreamNavMeshPool, 'NVMP', Resources::ResourceStreamPool);


//------------------------------------------------------------------------------
/**
*/
StreamNavMeshPool::StreamNavMeshPool()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
StreamNavMeshPool::~StreamNavMeshPool()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
dtNavMesh*
StreamNavMeshPool::GetDetourMesh(NavMeshId id)
{
    return this->allocator.Get<1>(id.resourceId);
}



//------------------------------------------------------------------------------
/**
*/
Util::Array<NavMeshId>
StreamNavMeshPool::GetLoadedMeshes()
{
    Util::Array<NavMeshId> meshes;
    meshes.Reserve(this->ids.Size());
    for (auto const& mesh : this->ids)
    {
        meshes.Append(mesh.Value());
    }
    return meshes;
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
StreamNavMeshPool::LoadFromStream(const Resources::ResourceId res, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    n_assert(stream.isvalid());
    n_assert(this->GetState(res) == Resources::Resource::Pending);

    /// during the load-phase, we can safetly get the structs
    this->EnterGet();
    NavMeshT& meshInfo = this->allocator.Get<3>(res.resourceId);
    Util::String& name = this->allocator.Get<0>(res.resourceId);
    dtNavMesh*& navMesh = this->allocator.Get<1>(res.resourceId);
    dtNavMeshQuery*& navMeshQuery = this->allocator.Get<2>(res.resourceId);
    this->LeaveGet();

    if (stream->Open())
    {
        void* buf = stream->Map();
        if (buf == nullptr)
        {
            return Failed;
        }

        Flat::FlatbufferInterface::DeserializeFlatbuffer<Navigation::NavMesh>(meshInfo, (uint8_t*)buf);
        stream->Unmap();

        name = meshInfo.name;

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
                return Success;
            }
            storedNavMesh->Unmap();
        }
    }
    return Failed;
}

//------------------------------------------------------------------------------
/**
*/
void
StreamNavMeshPool::Unload(const Resources::ResourceId res)
{
    this->EnterGet();
    NavMeshT& meshInfo = this->allocator.Get<3>(res.resourceId);
    Util::String& name = this->allocator.Get<0>(res.resourceId);
    dtNavMesh*& navMesh = this->allocator.Get<1>(res.resourceId);
    dtNavMeshQuery*& navMeshQuery = this->allocator.Get<2>(res.resourceId);
    dtFreeNavMesh(navMesh);
    dtFreeNavMeshQuery(navMeshQuery);
    this->LeaveGet();
    this->states[res.poolId] = Resources::Resource::State::Unloaded;
}

}
