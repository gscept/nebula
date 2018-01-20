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
	id.allocType = MeshIdType;
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
inline const SizeT
MeshGetPrimitiveGroups(const MeshId id)
{
	return meshPool->GetPrimitiveGroups(id);
}

} // Base
