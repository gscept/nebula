#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::MemoryMeshLoader
    
    Setup a Mesh object from a given vertex, index buffer and primitive group.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "resources/resourcememorypool.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/primitivegroup.h"
#include "coregraphics/mesh.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
class MemoryMeshPool : public Resources::ResourceMemoryPool
{
    __DeclareClass(MemoryMeshPool);
public:
    /// constructor
    MemoryMeshPool();
	/// destructor
	virtual ~MemoryMeshPool();

	/// update resource
	LoadStatus LoadFromMemory(const Resources::ResourceId id, const void* info);
	/// unload resource
	void Unload(const Resources::ResourceId id);

	/// bind mesh
	void BindMesh(const MeshId id, const IndexT prim);
	/// get primitive groups from mesh
	const Util::Array<CoreGraphics::PrimitiveGroup>& GetPrimitiveGroups(const MeshId id) const;
	/// get vertex buffer
	const VertexBufferId GetVertexBuffer(const MeshId id, const IndexT stream) const;	
	/// get index buffer
	const IndexBufferId GetIndexBuffer(const MeshId id) const;
	/// get topology
	const CoreGraphics::PrimitiveTopology::Code GetPrimitiveTopology(const MeshId id) const;
	/// enter thread-safe get mode
	void BeginGet();
	/// exit thread-safe get mode
	void EndGet();
private:
	friend class StreamMeshPool;


	Ids::IdAllocatorSafe<MeshCreateInfo> allocator;
	__ImplementResourceAllocatorTypedSafe(allocator, MeshIdType);
};

} // namespace CoreGraphics
//------------------------------------------------------------------------------
