//------------------------------------------------------------------------------
//  mesh.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "mesh.h"
#include "memorymeshcache.h"
#include "coregraphics/commandbuffer.h"

namespace CoreGraphics
{

MemoryMeshCache* meshCache = nullptr;

using namespace Ids;
//------------------------------------------------------------------------------
/**
*/
const MeshId
CreateMesh(const MeshCreateInfo& info)
{
    MeshId id = meshCache->ReserveResource(info.name, info.tag);
    n_assert(id.resourceType == MeshIdType);
    meshCache->LoadFromMemory(id, &info);
    return id;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyMesh(const MeshId id)
{
    meshCache->DiscardResource(id);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CoreGraphics::PrimitiveGroup>&
MeshGetPrimitiveGroups(const MeshId id)
{
    return meshCache->GetPrimitiveGroups(id);
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
MeshGetVertexBuffer(const MeshId id, const IndexT stream)
{
    return meshCache->GetVertexBuffer(id, stream);
}

//------------------------------------------------------------------------------
/**
*/
const uint
MeshGetVertexOffset(const MeshId id, const IndexT stream)
{
    return meshCache->GetVertexOffset(id, stream);
}

//------------------------------------------------------------------------------
/**
*/
const BufferId
MeshGetIndexBuffer(const MeshId id)
{
    return meshCache->GetIndexBuffer(id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveTopology::Code
MeshGetTopology(const MeshId id)
{
    return meshCache->GetPrimitiveTopology(id);
}

} // Base
