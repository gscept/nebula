#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::MemoryMeshLoader
    
    Setup a Mesh object from a given vertex, index buffer and primitive group.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "resources/resourcememorypool.h"
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
    const Util::Array<CoreGraphics::PrimitiveGroup>& GetPrimitiveGroups(const MeshId id);
    /// get vertex buffer
    const BufferId GetVertexBuffer(const MeshId id, const IndexT stream);
    /// get index buffer
    const BufferId GetIndexBuffer(const MeshId id);
    /// get topology
    const CoreGraphics::PrimitiveTopology::Code GetPrimitiveTopology(const MeshId id);
private:
    friend class StreamMeshPool;


    Ids::IdAllocatorSafe<MeshCreateInfo> allocator;
    __ImplementResourceAllocatorTypedSafe(allocator, MeshIdType);
};

} // namespace CoreGraphics
//------------------------------------------------------------------------------
