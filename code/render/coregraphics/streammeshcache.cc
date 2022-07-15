//------------------------------------------------------------------------------
// vkstreammeshloader.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "streammeshcache.h"
#include "coregraphics/mesh.h"
#include "coregraphics/legacy/nvx2streamreader.h"
#include "coregraphics/memorymeshcache.h"
#include "coregraphics/graphicsdevice.h"
#include "streammeshcache.h"

namespace CoreGraphics
{
using namespace IO;
using namespace CoreGraphics;
using namespace Util;
__ImplementClass(CoreGraphics::StreamMeshCache, 'VKML', Resources::ResourceStreamCache);
//------------------------------------------------------------------------------
/**
*/
StreamMeshCache::StreamMeshCache() :
    activeMesh(Ids::InvalidId24)
{
    this->placeholderResourceName = "msh:system/placeholder.nvx2";
    this->failResourceName = "msh:system/error.nvx2";
    this->async = true;

    this->streamerThreadName = "Mesh Pool Streamer Thread";
}

//------------------------------------------------------------------------------
/**
*/
StreamMeshCache::~StreamMeshCache()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceCache::LoadStatus
StreamMeshCache::LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    n_assert(stream.isvalid());
    n_assert(id != Ids::InvalidId24);
    String resIdExt = this->GetName(id.poolId).AsString().GetFileExtension();

#if NEBULA_LEGACY_SUPPORT
    if (resIdExt == "nvx2")
    {
        return this->SetupMeshFromNvx2(stream, id);
    }
    else
#endif
        if (resIdExt == "nvx3")
        {
            return this->SetupMeshFromNvx3(stream, id);
        }
        else if (resIdExt == "n3d3")
        {
            return this->SetupMeshFromN3d3(stream, id);
        }
        else
        {
            n_error("StreamMeshCache::SetupMeshFromStream(): unrecognized file extension in '%s'\n", resIdExt.AsCharPtr());
            return Resources::ResourceCache::Failed;
        }
}

//------------------------------------------------------------------------------
/**
*/
void
StreamMeshCache::Unload(const Resources::ResourceId id)
{
    n_assert(id.resourceId != Ids::InvalidId24);
    __LockName(meshCache->Allocator(), lock, Util::ArrayAllocatorAccess::Write);
    MeshCreateInfo& msh = meshCache->Get<0>(id.resourceId);

    if (msh.indexBuffer != InvalidBufferId)
        DestroyBuffer(msh.indexBuffer);

    IndexT i;
    for (i = 0; i < msh.streams.Size(); i++)
        DestroyBuffer(msh.streams[i].vertexBuffer);
    msh.streams.Clear();
    msh.primitiveGroups.Clear();

    this->states[id.poolId] = Resources::Resource::State::Unloaded;
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceUnknownId
StreamMeshCache::AllocObject()
{
    return meshCache->AllocObject();
}

//------------------------------------------------------------------------------
/**
*/
void
StreamMeshCache::DeallocObject(const Resources::ResourceUnknownId id)
{
    meshCache->DeallocObject(id);
}

//------------------------------------------------------------------------------
/**
    Setup the mesh resource from legacy nvx2 file (Nebula2 binary mesh format).
*/
#if NEBULA_LEGACY_SUPPORT
Resources::ResourceCache::LoadStatus
StreamMeshCache::SetupMeshFromNvx2(const Ptr<Stream>& stream, const Resources::ResourceId res)
{
    n_assert(stream.isvalid());

    Ptr<Legacy::Nvx2StreamReader> nvx2Reader = Legacy::Nvx2StreamReader::Create();
    nvx2Reader->SetStream(stream);
    nvx2Reader->SetUsage(this->usage);
    nvx2Reader->SetAccess(this->access);
    Resources::ResourceName name = this->GetName(res);

    // get potential metadata
    const _LoadMetaData& metaData = this->metaData[res.poolId];
    if (metaData.data != nullptr)
    {
        StreamMeshLoadMetaData* typedMetadata = static_cast<StreamMeshLoadMetaData*>(metaData.data);
        nvx2Reader->SetBuffersCopySource(typedMetadata->copySource);
    }

    // opening the reader also loads the file
    if (nvx2Reader->Open(name))
    {
        __LockName(meshCache->Allocator(), lock, Util::ArrayAllocatorAccess::Write);
        MeshCreateInfo& msh = meshCache->Get<0>(res);
        n_assert(this->GetState(res) == Resources::Resource::Pending);
        auto vertexLayout = CreateVertexLayout({ nvx2Reader->GetVertexComponents() });
        msh.streams.Append({ nvx2Reader->GetVertexBuffer(), nvx2Reader->GetBaseVertexOffset(), 0 });
        msh.indexBufferOffset = nvx2Reader->GetBaseIndexOffset();
        msh.indexBuffer = nvx2Reader->GetIndexBuffer();
        msh.topology = PrimitiveTopology::TriangleList;
        msh.primitiveGroups = nvx2Reader->GetPrimitiveGroups();
        msh.vertexLayout = vertexLayout;
        // nvx2 does not have per primitive layouts, we apply them to all
        for (auto & i : msh.primitiveGroups)
        {
            i.SetVertexLayout(vertexLayout);
        }

        nvx2Reader->Close();
        return ResourceCache::Success;
    }
    return ResourceCache::Failed;
}
#endif

//------------------------------------------------------------------------------
/**
    Setup the mesh resource from a nvx3 file (Nebula's
    native binary mesh file format).
*/
Resources::ResourceCache::LoadStatus
StreamMeshCache::SetupMeshFromNvx3(const Ptr<Stream>& stream, const Resources::ResourceId res)
{
    // FIXME!
    n_error("StreamMeshCache::SetupMeshFromNvx3() not yet implemented");
    return Resources::ResourceCache::Failed;
}

//------------------------------------------------------------------------------
/**
    Setup the mesh resource from a n3d3 file (Nebula's
    native ascii mesh file format).
*/
Resources::ResourceCache::LoadStatus
StreamMeshCache::SetupMeshFromN3d3(const Ptr<Stream>& stream, const Resources::ResourceId res)
{
    // FIXME!
    n_error("StreamMeshCache::SetupMeshFromN3d3() not yet implemented");
    return Resources::ResourceCache::Failed;
}

} // namespace Vulkan
