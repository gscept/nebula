//------------------------------------------------------------------------------
//  memorymeshloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
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
	this->EnterGet();
	MeshCreateInfo* data = (MeshCreateInfo*)info;
	MeshCreateInfo& mesh = this->Get<0>(id.resourceId);
	mesh = *data;

	this->states[id.poolId] = Resource::Loaded;
	this->LeaveGet();

	return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
void
MemoryMeshPool::Unload(const Resources::ResourceId id)
{
	this->states[id.poolId] = Resource::State::Unloaded;
}

//------------------------------------------------------------------------------
/**
*/
void
MemoryMeshPool::BindMesh(const MeshId id, const IndexT prim)
{
#if _DEBUG
	n_assert(id.resourceType == MeshIdType);
#endif
	this->allocator.EnterGet();
	MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);

	// setup pipeline (a bit ugly)
	CoreGraphics::SetVertexLayout(inf.primitiveGroups[prim].GetVertexLayout());
	CoreGraphics::SetPrimitiveTopology(inf.topology);

	// set input
	CoreGraphics::SetPrimitiveGroup(inf.primitiveGroups[prim]);

	// bind vertex buffers
	IndexT i;
	for (i = 0; i < inf.streams.Size(); i++)
		CoreGraphics::SetStreamVertexBuffer(inf.streams[i].index, inf.streams[i].vertexBuffer, 0);

	if (inf.indexBuffer != CoreGraphics::IndexBufferId::Invalid())
		CoreGraphics::SetIndexBuffer(inf.indexBuffer, 0);
	this->allocator.LeaveGet();
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CoreGraphics::PrimitiveGroup>&
MemoryMeshPool::GetPrimitiveGroups(const MeshId id)
{
	this->EnterGet();
	const MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);
	this->LeaveGet();
	return inf.primitiveGroups;
}

//------------------------------------------------------------------------------
/**
*/
const VertexBufferId
MemoryMeshPool::GetVertexBuffer(const MeshId id, const IndexT stream)
{
	this->EnterGet();
	const MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);
	this->LeaveGet();
	return inf.streams[stream].vertexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const IndexBufferId
MemoryMeshPool::GetIndexBuffer(const MeshId id)
{
	this->EnterGet();
	const MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);
	this->LeaveGet();
	return inf.indexBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PrimitiveTopology::Code
MemoryMeshPool::GetPrimitiveTopology(const MeshId id)
{
	this->EnterGet();
	const MeshCreateInfo& inf = this->allocator.Get<0>(id.resourceId);
	this->LeaveGet();
	return inf.topology;
}

} // namespace CoreGraphics
