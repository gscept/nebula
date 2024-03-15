#pragma once
//------------------------------------------------------------------------------
/**
    @struct CoreGraphics::MeshCreateInfo
    
    Mesh collects vertex and index buffers with primitive groups which can be used to render with

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/config.h"
#include "resources/resourceid.h"
#include "ids/id.h"
#include "ids/idallocator.h"
#include "coregraphics/buffer.h"
#include "coregraphics/primitivetopology.h"
#include "coregraphics/primitivegroup.h"
#include "coregraphics/vertexlayout.h"
namespace CoreGraphics
{
struct VertexAlloc
{
    uint size, offset, node;
};

struct CmdBufferId;

ID_24_8_TYPE(MeshId);

struct VertexStream
{
    BufferId vertexBuffer;
    SizeT offset;
    IndexT index;
};

struct MeshCreateInfo
{
    Resources::ResourceName name;
    Util::StackArray<VertexStream, 4> streams;
    SizeT indexBufferOffset;
    BufferId indexBuffer;
    IndexType::Code indexType;
    VertexLayoutId vertexLayout;
    CoreGraphics::PrimitiveTopology::Code topology;
    Util::Array<CoreGraphics::PrimitiveGroup> primitiveGroups;

    CoreGraphics::VertexAlloc vertexBufferAllocation, indexBufferAllocation;
};

/// create new mesh
const MeshId CreateMesh(const MeshCreateInfo& info);
/// destroy mesh
void DestroyMesh(const MeshId id);

/// get number of primitive groups
const Util::Array<CoreGraphics::PrimitiveGroup>& MeshGetPrimitiveGroups(const MeshId id);
/// get primitive group
const CoreGraphics::PrimitiveGroup MeshGetPrimitiveGroup(const MeshId id, const IndexT group);
/// get vertex buffer
const BufferId MeshGetVertexBuffer(const MeshId id, const IndexT stream);
/// Set vertex buffer
const void MeshSetVertexBuffer(const MeshId id, const BufferId buffer, const IndexT stream);
/// Get mesh vertex offset
const uint MeshGetVertexOffset(const MeshId id, const IndexT stream);
/// get index buffer
const BufferId MeshGetIndexBuffer(const MeshId id);
/// Get index buffer base offset
const uint MeshGetIndexOffset(const MeshId id);
/// Get index type
const IndexType::Code MeshGetIndexType(const MeshId id);
/// Get topology
const CoreGraphics::PrimitiveTopology::Code MeshGetTopology(const MeshId id);
/// Get vertex layout
const CoreGraphics::VertexLayoutId MeshGetVertexLayout(const MeshId id);

/// Bind mesh on command buffer
const void MeshBind(const MeshId id, const CoreGraphics::CmdBufferId cmd);

enum
{
    Mesh_Name,
    Mesh_Internals,
};

struct __Mesh
{
    Util::StackArray<VertexStream, 4> streams;
    SizeT indexBufferOffset;
    BufferId indexBuffer;
    IndexType::Code indexType;
    VertexLayoutId vertexLayout;
    CoreGraphics::PrimitiveTopology::Code primitiveTopology;
    Util::Array<CoreGraphics::PrimitiveGroup> primitiveGroups;
    CoreGraphics::VertexAlloc vertexAllocation, indexAllocation;
};

typedef Ids::IdAllocatorSafe<
    0xFFFF,
    Resources::ResourceName,
    __Mesh
> MeshAllocator;
extern MeshAllocator meshAllocator;

_DECL_ACQUIRE_RELEASE(MeshId);

extern MeshId RectangleMesh;
extern MeshId DiskMesh;

} // CoreGraphics
