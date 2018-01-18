#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::MemoryMeshLoader
    
    Setup a Mesh object from a given vertex, index buffer and primitive group.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
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
	LoadStatus LoadFromMemory(const Ids::Id24 id, const void* info);
	/// unload resource
	void Unload(const Ids::Id24 id);

	/// bind mesh
	void BindMesh(const MeshId id, const IndexT prim);
	/// get primitive groups from mesh
	const SizeT	GetPrimitiveGroups(const MeshId id);
private:
	friend class StreamMeshPool;


	Ids::IdAllocatorSafe<MeshCreateInfo> allocator;
	__ImplementResourceAllocatorSafe(allocator);
};

} // namespace CoreGraphics
//------------------------------------------------------------------------------
