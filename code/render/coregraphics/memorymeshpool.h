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

private:
	friend class StreamMeshPool;

	/// update resource
	LoadStatus LoadFromMemory(const Ids::Id24 id, void* info);
	/// unload resource
	void Unload(const Ids::Id24 id);

	Ids::IdAllocatorSafe<MeshCreateInfo> allocator;
	__ImplementResourceAllocatorSafe(allocator);
};

} // namespace CoreGraphics
//------------------------------------------------------------------------------
