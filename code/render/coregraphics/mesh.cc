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
    meshAllocator.SetUnsafe<Mesh_VertexLayout>(id, info.vertexLayout);
    meshAllocator.SetUnsafe<Mesh_Topology>(id, info.topology);
    meshAllocator.SetUnsafe<Mesh_PrimitiveGroups>(id, info.primitiveGroups);

    MeshId ret;
    ret.resourceId = id;
    ret.resourceType = MeshIdType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyMesh(const MeshId id)
{
    meshAllocator.Dealloc(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CoreGraphics::PrimitiveGroup>&
MeshGetPrimitiveGroups(const MeshId id)
{
    return meshAllocator.GetUnsafe<Mesh_PrimitiveGroups>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
MeshGetVertexBuffer(const MeshId id, const IndexT stream)
{
    return meshAllocator.GetUnsafe<Mesh_Streams>(id.resourceId)[stream].vertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const uint
MeshGetVertexOffset(const MeshId id, const IndexT stream)
{
    return meshAllocator.GetUnsafe<Mesh_Streams>(id.resourceId)[stream].offset;
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
MeshGetIndexBuffer(const MeshId id)
{
    return meshAllocator.GetUnsafe<Mesh_IndexBuffer>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const uint
MeshGetIndexOffset(const MeshId id)
{
    return meshAllocator.GetUnsafe<Mesh_IndexBufferOffset>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveTopology::Code
MeshGetTopology(const MeshId id)
{
    return meshAllocator.GetUnsafe<Mesh_Topology>(id.resourceId);
}

} // Base
