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

_IMPL_ACQUIRE_RELEASE(MeshId, meshAllocator);

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
    };
    meshAllocator.Set<Mesh_Internals>(id, internals);
    meshAllocator.Release(id);

    MeshId ret = id;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyMesh(const MeshId id)
{
    meshAllocator.Dealloc(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CoreGraphics::PrimitiveGroup>&
MeshGetPrimitiveGroups(const MeshId id)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id).primitiveGroups;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveGroup
MeshGetPrimitiveGroup(const MeshId id, const IndexT group)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id).primitiveGroups[group];
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
MeshGetVertexBuffer(const MeshId id, const IndexT stream)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id).streams[stream].vertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const void
MeshSetVertexBuffer(const MeshId id, const BufferId buffer, const IndexT stream)
{
    meshAllocator.ConstGet<Mesh_Internals>(id.id).streams[stream].vertexBuffer = buffer;
}

//------------------------------------------------------------------------------
/**
*/
const uint64_t
MeshGetVertexOffset(const MeshId id, const IndexT stream)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id).streams[stream].offset;
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
MeshGetIndexBuffer(const MeshId id)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id).indexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const uint64_t
MeshGetIndexOffset(const MeshId id)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id).indexBufferOffset;
}

//------------------------------------------------------------------------------
/**
*/
const IndexType::Code
MeshGetIndexType(const MeshId id)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id).indexType;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveTopology::Code
MeshGetTopology(const MeshId id)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id).primitiveTopology;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::VertexLayoutId
MeshGetVertexLayout(const MeshId id)
{
    return meshAllocator.ConstGet<Mesh_Internals>(id.id).vertexLayout;
}

//------------------------------------------------------------------------------
/**
*/
const void
MeshBind(const MeshId id, const CoreGraphics::CmdBufferId cmd)
{
    const auto& internals = meshAllocator.ConstGet<Mesh_Internals>(id.id);

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
