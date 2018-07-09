//------------------------------------------------------------------------------
//  memorymeshloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/memorymeshpool.h"
#include "coregraphics/mesh.h"
#include "coregraphics/legacy/nvx2streamreader.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/config.h"
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryMeshPool, 'DMMP', Resources::ResourceMemoryPool);

using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
MemoryMeshPool::MemoryMeshPool()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
MemoryMeshPool::~MemoryMeshPool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ResourcePool::LoadStatus
MemoryMeshPool::LoadFromMemory(const Resources::ResourceId id, const void* info)
{
	MeshCreateInfo* data = (MeshCreateInfo*)info;
	MeshCreateInfo& mesh = this->Get<0>(id.allocId);
	mesh = *data;

	this->states[id.poolId] = Resource::Loaded;

	return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
void
MemoryMeshPool::Unload(const Resources::ResourceId id)
{
}

//------------------------------------------------------------------------------
/**
*/
void
MemoryMeshPool::BindMesh(const MeshId id, const IndexT prim)
{
#if _DEBUG
	n_assert(id.allocType == MeshIdType);
#endif
	MeshCreateInfo& inf = this->allocator.Get<0>(id.allocId);
	CoreGraphics::SetStreamVertexBuffer(0, inf.vertexBuffer, inf.primitiveGroups[prim].GetBaseVertex());
	if (inf.indexBuffer != Ids::InvalidId64)
		CoreGraphics::SetIndexBuffer(inf.indexBuffer, inf.primitiveGroups[prim].GetBaseIndex());
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CoreGraphics::PrimitiveGroup>&
MemoryMeshPool::GetPrimitiveGroups(const MeshId id) const
{
	const MeshCreateInfo& inf = this->allocator.Get<0>(id.allocId);
	return inf.primitiveGroups;
}

//------------------------------------------------------------------------------
/**
*/
const VertexBufferId
MemoryMeshPool::GetVertexBuffer(const MeshId id) const
{
	const MeshCreateInfo& inf = this->allocator.Get<0>(id.allocId);
	return inf.vertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const VertexLayoutId
MemoryMeshPool::GetVertexLayout(const MeshId id) const
{
	const MeshCreateInfo& inf = this->allocator.Get<0>(id.allocId);
	return inf.vertexLayout;
}

//------------------------------------------------------------------------------
/**
*/
const IndexBufferId
MemoryMeshPool::GetIndexBuffer(const MeshId id) const
{
	const MeshCreateInfo& inf = this->allocator.Get<0>(id.allocId);
	return inf.indexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveTopology::Code MemoryMeshPool::GetPrimitiveTopology(const MeshId id) const
{
	const MeshCreateInfo& inf = this->allocator.Get<0>(id.allocId);
	return inf.topology;
}

} // namespace CoreGraphics
