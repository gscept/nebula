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
	Resources::ResourceId pid = meshPool->ReserveResource(info.name, info.tag);
	meshPool->LoadFromMemory(pid, &info);
	
	return pid;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DestroyMesh(const MeshId id)
{
	meshPool->DiscardResource(id.id24);
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
