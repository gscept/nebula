//------------------------------------------------------------------------------
//  @file meshresource.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "meshresource.h"
namespace CoreGraphics
{

MeshResourceAllocator meshResourceAllocator;

//------------------------------------------------------------------------------
/**
*/
const MeshId
MeshResourceGetMesh(const MeshResourceId id, const IndexT index)
{
    return meshResourceAllocator.Get<0>(id.id)[index];
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
MeshResourceGetNumMeshes(const MeshResourceId id)
{
    return meshResourceAllocator.Get<0>(id.id).Size();
}

//------------------------------------------------------------------------------
/**
*/
void DestroyMeshResource(const MeshResourceId id)
{
    auto vertexData = meshResourceAllocator.Get<MeshResource_VertexData>(id.id);
    auto indexData = meshResourceAllocator.Get<MeshResource_IndexData>(id.id);
    auto meshes = meshResourceAllocator.Get<MeshResource_Meshes>(id.id);

    CoreGraphics::DeallocateVertices(vertexData);
    CoreGraphics::DeallocateIndices(indexData);
    for (IndexT i = 0; i < meshes.Size(); i++)
    {
        DestroyMesh(meshes[i]);
    }
    meshResourceAllocator.Dealloc(id.id);
}

} // namespace CoreGraphics
