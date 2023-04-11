//------------------------------------------------------------------------------
//  mesh.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "mesh.h"
#include "coregraphics/commandbuffer.h"

namespace CoreGraphics
{

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
    meshAllocator.SetUnsafe<Mesh_Name>(id, info.name);
    meshAllocator.SetUnsafe<Mesh_Streams>(id, info.streams);
    meshAllocator.SetUnsafe<Mesh_IndexBufferOffset>(id, info.indexBufferOffset);
    meshAllocator.SetUnsafe<Mesh_IndexBuffer>(id, info.indexBuffer);
    meshAllocator.SetUnsafe<Mesh_IndexType>(id, info.indexType);
    meshAllocator.SetUnsafe<Mesh_VertexLayout>(id, info.vertexLayout);
    meshAllocator.SetUnsafe<Mesh_Topology>(id, info.topology);
    meshAllocator.SetUnsafe<Mesh_PrimitiveGroups>(id, info.primitiveGroups);

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
    meshAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CoreGraphics::PrimitiveGroup>&
MeshGetPrimitiveGroups(const MeshId id)
{
    return meshAllocator.GetUnsafe<Mesh_PrimitiveGroups>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
MeshGetVertexBuffer(const MeshId id, const IndexT stream)
{
    return meshAllocator.GetUnsafe<Mesh_Streams>(id.id24)[stream].vertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const uint
MeshGetVertexOffset(const MeshId id, const IndexT stream)
{
    return meshAllocator.GetUnsafe<Mesh_Streams>(id.id24)[stream].offset;
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
MeshGetIndexBuffer(const MeshId id)
{
    return meshAllocator.GetUnsafe<Mesh_IndexBuffer>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const uint
MeshGetIndexOffset(const MeshId id)
{
    return meshAllocator.GetUnsafe<Mesh_IndexBufferOffset>(id.id24);
}

const IndexType::Code
MeshGetIndexType(const MeshId id)
{
    return meshAllocator.GetUnsafe<Mesh_IndexType>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveTopology::Code
MeshGetTopology(const MeshId id)
{
    return meshAllocator.GetUnsafe<Mesh_Topology>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::VertexLayoutId
MeshGetVertexLayout(const MeshId id)
{
    return meshAllocator.GetUnsafe<Mesh_VertexLayout>(id.id24);
}

} // Base
