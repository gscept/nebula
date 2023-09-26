//------------------------------------------------------------------------------
//  mesh.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

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
    Ids::Id32 id = meshAllocator.Alloc();

    // Thing is, we just allocated this index so we own it right now
    meshAllocator.Set<Mesh_Name>(id, info.name);
    __Mesh internals{
        .streams = info.streams,
        .indexBufferOffset = info.indexBufferOffset,
        .indexBuffer = info.indexBuffer,
        .indexType = info.indexType,
        .vertexLayout = info.vertexLayout,
        .primitiveTopology = info.topology,
        .primitiveGroups = info.primitiveGroups,
        .vertexAllocation = info.vertexBufferAllocation,
        .indexAllocation = info.indexBufferAllocation
    };
    meshAllocator.Set<Mesh_Internals>(id, internals);
    meshAllocator.Release(id);

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
    const CoreGraphics::VertexAlloc& vertices = meshAllocator.ConstGet<Mesh_Internals>(id.id24).vertexAllocation;
    const CoreGraphics::VertexAlloc& indices = meshAllocator.ConstGet<Mesh_Internals>(id.id24).indexAllocation;

    CoreGraphics::DeallocateVertices(vertices);
    CoreGraphics::DeallocateIndices(indices);
    meshAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CoreGraphics::PrimitiveGroup>&
MeshGetPrimitiveGroups(const MeshId id)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id24).primitiveGroups;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveGroup
MeshGetPrimitiveGroup(const MeshId id, const IndexT group)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id24).primitiveGroups[group];
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
MeshGetVertexBuffer(const MeshId id, const IndexT stream)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id24).streams[stream].vertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const void
MeshSetVertexBuffer(const MeshId id, const BufferId buffer, const IndexT stream)
{
    meshAllocator.ConstGet<Mesh_Internals>(id.id24).streams[stream].vertexBuffer = buffer;
}

//------------------------------------------------------------------------------
/**
*/
const uint
MeshGetVertexOffset(const MeshId id, const IndexT stream)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id24).streams[stream].offset;
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
MeshGetIndexBuffer(const MeshId id)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id24).indexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const uint
MeshGetIndexOffset(const MeshId id)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id24).indexBufferOffset;
}

//------------------------------------------------------------------------------
/**
*/
const IndexType::Code
MeshGetIndexType(const MeshId id)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id24).indexType;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveTopology::Code
MeshGetTopology(const MeshId id)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id24).primitiveTopology;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::VertexLayoutId
MeshGetVertexLayout(const MeshId id)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id24).vertexLayout;
}

//------------------------------------------------------------------------------
/**
*/
const void
MeshBind(const MeshId id, const CoreGraphics::CmdBufferId cmd)
{
    const auto& internals = meshAllocator.ConstGet<Mesh_Internals>(id.id24);

    CoreGraphics::CmdSetPrimitiveTopology(cmd, internals.primitiveTopology);
    CoreGraphics::CmdSetVertexLayout(cmd, internals.vertexLayout);

    // bind vertex buffers
    for (IndexT i = 0; i < internals.streams.Size(); i++)
    {
        BufferIdAcquire(internals.streams[i].vertexBuffer);
        CoreGraphics::CmdSetVertexBuffer(cmd, i, internals.streams[i].vertexBuffer, internals.streams[i].offset);
        BufferIdRelease(internals.streams[i].vertexBuffer);
    }

    if (internals.indexBuffer != CoreGraphics::InvalidBufferId)
    {
        BufferIdAcquire(internals.indexBuffer);
        CoreGraphics::CmdSetIndexBuffer(cmd, internals.indexType, internals.indexBuffer, internals.indexBufferOffset);
        BufferIdRelease(internals.indexBuffer);
    }
}

} // CoreGraphics
