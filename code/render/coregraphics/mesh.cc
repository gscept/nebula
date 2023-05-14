//------------------------------------------------------------------------------
//  mesh.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "mesh.h"
#include "coregraphics/commandbuffer.h"

namespace CoreGraphics
{

MeshId RectangleMesh;
MeshId DiskMesh;

MeshAllocator meshAllocator;
using namespace Ids;
//------------------------------------------------------------------------------
/**
*/
const MeshId
CreateMesh(const MeshCreateInfo& info)
{
    Util::AllocatorLock lock(&meshAllocator, Util::ArrayAllocatorAccess::Write);
    Ids::Id32 id = meshAllocator.Alloc();

    // Thing is, we just allocated this index so we own it right now
    meshAllocator.Set<Mesh_Name>(id, info.name);
    __Mesh internals{
        info.streams,
        info.indexBufferOffset,
        info.indexBuffer,
        info.indexType,
        info.vertexLayout,
        info.topology,
        info.primitiveGroups
    };
    meshAllocator.Set<Mesh_Internals>(id, internals);

    MeshId ret;
    ret.id24 = id;
    ret.id8 = MeshIdType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyMesh(const MeshId id)
{
    Util::AllocatorLock lock(&meshAllocator, Util::ArrayAllocatorAccess::Write);
    meshAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CoreGraphics::PrimitiveGroup>&
MeshGetPrimitiveGroups(const MeshId id)
{
    Util::AllocatorLock lock(&meshAllocator, Util::ArrayAllocatorAccess::Read);
    return meshAllocator.Get<Mesh_Internals>(id.id24).primitiveGroups;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveGroup
MeshGetPrimitiveGroup(const MeshId id, const IndexT group)
{
    Util::AllocatorLock lock(&meshAllocator, Util::ArrayAllocatorAccess::Read);
    return meshAllocator.Get<Mesh_Internals>(id.id24).primitiveGroups[group];
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
MeshGetVertexBuffer(const MeshId id, const IndexT stream)
{
    Util::AllocatorLock lock(&meshAllocator, Util::ArrayAllocatorAccess::Read);
    return meshAllocator.Get<Mesh_Internals>(id.id24).streams[stream].vertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const void
MeshSetVertexBuffer(const MeshId id, const BufferId buffer, const IndexT stream)
{
    Util::AllocatorLock lock(&meshAllocator, Util::ArrayAllocatorAccess::Read);
    meshAllocator.Get<Mesh_Internals>(id.id24).streams[stream].vertexBuffer = buffer;
}

//------------------------------------------------------------------------------
/**
*/
const uint
MeshGetVertexOffset(const MeshId id, const IndexT stream)
{
    Util::AllocatorLock lock(&meshAllocator, Util::ArrayAllocatorAccess::Read);
    return meshAllocator.Get<Mesh_Internals>(id.id24).streams[stream].offset;
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
MeshGetIndexBuffer(const MeshId id)
{
    Util::AllocatorLock lock(&meshAllocator, Util::ArrayAllocatorAccess::Read);
    return meshAllocator.Get<Mesh_Internals>(id.id24).indexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const uint
MeshGetIndexOffset(const MeshId id)
{
    Util::AllocatorLock lock(&meshAllocator, Util::ArrayAllocatorAccess::Read);
    return meshAllocator.Get<Mesh_Internals>(id.id24).indexBufferOffset;
}

//------------------------------------------------------------------------------
/**
*/
const IndexType::Code
MeshGetIndexType(const MeshId id)
{
    Util::AllocatorLock lock(&meshAllocator, Util::ArrayAllocatorAccess::Read);
    return meshAllocator.Get<Mesh_Internals>(id.id24).indexType;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveTopology::Code
MeshGetTopology(const MeshId id)
{
    Util::AllocatorLock lock(&meshAllocator, Util::ArrayAllocatorAccess::Read);
    return meshAllocator.Get<Mesh_Internals>(id.id24).primitiveTopology;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::VertexLayoutId
MeshGetVertexLayout(const MeshId id)
{
    Util::AllocatorLock lock(&meshAllocator, Util::ArrayAllocatorAccess::Read);
    return meshAllocator.Get<Mesh_Internals>(id.id24).vertexLayout;
}

//------------------------------------------------------------------------------
/**
*/
const void
MeshBind(const MeshId id, const CoreGraphics::CmdBufferId cmd)
{
    Util::AllocatorLock lock(&meshAllocator, Util::ArrayAllocatorAccess::Read);
    const auto& internals = meshAllocator.Get<Mesh_Internals>(id.id24);

    CoreGraphics::CmdSetPrimitiveTopology(cmd, internals.primitiveTopology);
    CoreGraphics::CmdSetVertexLayout(cmd, internals.vertexLayout);

    // bind vertex buffers
    for (IndexT i = 0; i < internals.streams.Size(); i++)
        CoreGraphics::CmdSetVertexBuffer(cmd, i, internals.streams[i].vertexBuffer, internals.streams[i].offset);

    if (internals.indexBuffer != CoreGraphics::InvalidBufferId)
        CoreGraphics::CmdSetIndexBuffer(cmd, internals.indexType, internals.indexBuffer, internals.indexBufferOffset);
}

} // Base
