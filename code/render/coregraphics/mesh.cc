//------------------------------------------------------------------------------
//  mesh.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "config.h"
#include "mesh.h"
#include "coregraphics/renderdevice.h"

namespace CoreGraphics
{

Ids::IdAllocator<MeshCreateInfo> meshAllocator(0x00FFFFFF);
MemoryMeshPool* meshPool = nullptr;

using namespace Ids;
//------------------------------------------------------------------------------
/**
*/
const MeshId
CreateMesh(const MeshCreateInfo& info)
{
	Id32 pid = meshAllocator.AllocObject();

	// simply copy the create info...
	MeshCreateInfo& inf = meshAllocator.Get<0>(pid);
	inf = info;

	MeshId id;
	id.id24 = pid;
	id.id8 = MeshIdType;
	return id;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyMesh(const MeshId id)
{
	meshAllocator.DeallocObject(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void
BindMesh(const MeshId id, const IndexT prim)
{
	RenderDevice* renderDevice = RenderDevice::Instance();
#if _DEBUG
	n_assert(id.id8 == MeshIdType);
#endif
	MeshCreateInfo& inf = meshAllocator.Get<0>(id.id24);
	BindVertexBuffer(inf.vertexBuffer, 0, inf.primitiveGroups[prim].GetBaseVertex());
	if (inf.indexBuffer != Ids::InvalidId64)
		BindIndexBuffer(inf.indexBuffer, inf.primitiveGroups[prim].GetBaseIndex());
}

} // Base
