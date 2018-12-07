//------------------------------------------------------------------------------
//  mesh.cc
//  (C)2017-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "mesh.h"
#include "memorymeshpool.h"

namespace CoreGraphics
{

MemoryMeshPool* meshPool = nullptr;

using namespace Ids;
//------------------------------------------------------------------------------
/**
*/
const MeshId
CreateMesh(const MeshCreateInfo& info)
{
	MeshId id = meshPool->ReserveResource(info.name, info.tag);
	n_assert(id.allocType == MeshIdType);
	meshPool->LoadFromMemory(id, &info);
	return id;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyMesh(const MeshId id)
{
	meshPool->DiscardResource(id);
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBind(const MeshId id, const IndexT prim)
{
	meshPool->BindMesh(id, prim);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CoreGraphics::PrimitiveGroup>&
MeshGetPrimitiveGroups(const MeshId id)
{
	return meshPool->GetPrimitiveGroups(id);
}

//------------------------------------------------------------------------------
/**
*/
const VertexBufferId
MeshGetVertexBuffer(const MeshId id, const IndexT stream)
{
	return meshPool->GetVertexBuffer(id, stream);
}

//------------------------------------------------------------------------------
/**
*/
const VertexLayoutId
MeshGetVertexLayout(const MeshId id)
{
	return meshPool->GetVertexLayout(id);
}

//------------------------------------------------------------------------------
/**
*/
const IndexBufferId
MeshGetIndexBuffer(const MeshId id)
{
	return meshPool->GetIndexBuffer(id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveTopology::Code
MeshGetTopology(const MeshId id)
{
	return meshPool->GetPrimitiveTopology(id);
}

} // Base
