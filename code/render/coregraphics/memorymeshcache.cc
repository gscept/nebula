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
    __LockName(this->Allocator(), lock, Util::ArrayAllocatorAccess::Write);
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
MemoryMeshCache::BindMesh(const MeshId id, IndexT prim, const CoreGraphics::CmdBufferId cmdBuf)
{
#if _DEBUG
    n_assert(id.resourceType == MeshIdType);
#endif
    __LockName(this->Allocator(), lock, Util::ArrayAllocatorAccess::Read);
    MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);

    // setup pipeline (a bit ugly)
    CoreGraphics::CmdSetPrimitiveTopology(cmdBuf, inf.topology);
    CoreGraphics::CmdSetVertexLayout(cmdBuf, inf.primitiveGroups[prim].GetVertexLayout());

    // bind vertex buffers
    IndexT i;
    for (i = 0; i < inf.streams.Size(); i++)
        CoreGraphics::CmdSetVertexBuffer(cmdBuf, inf.streams[i].index, inf.streams[i].vertexBuffer, 0);

    if (inf.indexBuffer != CoreGraphics::InvalidBufferId)
        CoreGraphics::CmdSetIndexBuffer(cmdBuf, inf.indexBuffer, 0);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CoreGraphics::PrimitiveGroup>&
MemoryMeshCache::GetPrimitiveGroups(const MeshId id)
{
    __LockName(this->Allocator(), lock, Util::ArrayAllocatorAccess::Read);
    const MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);
    return inf.primitiveGroups;
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
MemoryMeshCache::GetVertexBuffer(const MeshId id, const IndexT stream)
{
    __LockName(this->Allocator(), lock, Util::ArrayAllocatorAccess::Read);
    const MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);
    return inf.streams[stream].vertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
MemoryMeshCache::GetIndexBuffer(const MeshId id)
{
    __LockName(this->Allocator(), lock, Util::ArrayAllocatorAccess::Read);
    const MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);
    return inf.indexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveTopology::Code
MemoryMeshCache::GetPrimitiveTopology(const MeshId id)
{
    __LockName(this->Allocator(), lock, Util::ArrayAllocatorAccess::Read);
    const MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);
    return inf.topology;
}

} // namespace CoreGraphics
