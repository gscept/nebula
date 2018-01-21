//------------------------------------------------------------------------------
//  mesh.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "mesh.h"
#include "coregraphics/renderdevice.h"
#include "memorymeshpool.h"

namespace CoreGraphics
{

MemoryMeshPool* meshPool = nullptr;

using namespace Ids;
//------------------------------------------------------------------------------
/**
*/
inline const MeshId
CreateMesh(const MeshCreateInfo& info)
{
	MeshId id = meshPool->ReserveResource(info.name, info.tag);
	n_assert(id.allocType == MeshIdType);
	meshPool->LoadFromMemory(id.allocId, &info);
	return id;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DestroyMesh(const MeshId id)
{
	meshPool->DiscardResource(id.allocId);
}

//------------------------------------------------------------------------------
/**
*/
inline void
MeshBind(const MeshId id, const IndexT prim)
{
	meshPool->BindMesh(id, prim);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<CoreGraphics::PrimitiveGroup>&
MeshGetPrimitiveGroups(const MeshId id)
{
	return meshPool->GetPrimitiveGroups(id);
}

//------------------------------------------------------------------------------
/**
*/
inline const VertexBufferId
MeshGetVertexBuffer(const MeshId id)
{
	return meshPool->GetVertexBuffer(id);
}

//------------------------------------------------------------------------------
/**
*/
inline const VertexLayoutId
MeshGetVertexLayout(const MeshId id)
{
	return meshPool->GetVertexLayout(id);
}

//------------------------------------------------------------------------------
/**
*/
inline const IndexBufferId
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
