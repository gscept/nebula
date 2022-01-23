//------------------------------------------------------------------------------
//  memorymeshloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/memorymeshcache.h"
#include "coregraphics/mesh.h"
#include "coregraphics/legacy/nvx2streamreader.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/config.h"
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryMeshCache, 'DMMP', Resources::ResourceMemoryCache);

using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
MemoryMeshCache::MemoryMeshCache()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
MemoryMeshCache::~MemoryMeshCache()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ResourceCache::LoadStatus
MemoryMeshCache::LoadFromMemory(const Resources::ResourceId id, const void* info)
{
    __LockName(this->Allocator(), lock);
    MeshCreateInfo* data = (MeshCreateInfo*)info;
    MeshCreateInfo& mesh = this->Get<0>(id.resourceId);
    mesh = *data;

    this->states[id.poolId] = Resource::Loaded;

    return ResourceCache::Success;
}

//------------------------------------------------------------------------------
/**
*/
void
MemoryMeshCache::Unload(const Resources::ResourceId id)
{
    this->states[id.poolId] = Resource::State::Unloaded;
}

//------------------------------------------------------------------------------
/**
*/
void
MemoryMeshCache::BindMesh(const MeshId id, const IndexT prim)
{
#if _DEBUG
    n_assert(id.resourceType == MeshIdType);
#endif
    __LockName(this->Allocator(), lock);
    MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);

    // setup pipeline (a bit ugly)
    CoreGraphics::SetVertexLayout(inf.primitiveGroups[prim].GetVertexLayout());
    CoreGraphics::SetPrimitiveTopology(inf.topology);

    // set input
    CoreGraphics::SetPrimitiveGroup(inf.primitiveGroups[prim]);

    // bind vertex buffers
    IndexT i;
    for (i = 0; i < inf.streams.Size(); i++)
        CoreGraphics::SetStreamVertexBuffer(inf.streams[i].index, inf.streams[i].vertexBuffer, 0);

    if (inf.indexBuffer != CoreGraphics::InvalidBufferId)
        CoreGraphics::SetIndexBuffer(inf.indexBuffer, 0);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CoreGraphics::PrimitiveGroup>&
MemoryMeshCache::GetPrimitiveGroups(const MeshId id)
{
    __LockName(this->Allocator(), lock);
    const MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);
    return inf.primitiveGroups;
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
MemoryMeshCache::GetVertexBuffer(const MeshId id, const IndexT stream)
{
    __LockName(this->Allocator(), lock);
    const MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);
    return inf.streams[stream].vertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
MemoryMeshCache::GetIndexBuffer(const MeshId id)
{
    __LockName(this->Allocator(), lock);
    const MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);
    return inf.indexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveTopology::Code
MemoryMeshCache::GetPrimitiveTopology(const MeshId id)
{
    __LockName(this->Allocator(), lock);
    const MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);
    return inf.topology;
}

} // namespace CoreGraphics
