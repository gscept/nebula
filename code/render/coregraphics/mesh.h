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
namespace CoreGraphics
{

struct CmdBufferId;

RESOURCE_ID_TYPE(MeshId);

struct VertexStream
{
    BufferId vertexBuffer;
    SizeT offset;
    IndexT index;
};

struct MeshCreateInfo
{
    Resources::ResourceName name;
    Util::ArrayStack<VertexStream, 16> streams;
    SizeT indexBufferOffset;
    BufferId indexBuffer;
    VertexLayoutId vertexLayout;
    CoreGraphics::PrimitiveTopology::Code topology;
    Util::Array<CoreGraphics::PrimitiveGroup> primitiveGroups;
};

/// create new mesh
const MeshId CreateMesh(const MeshCreateInfo& info);
/// destroy mesh
void DestroyMesh(const MeshId id);

/// get number of primitive groups
const Util::Array<CoreGraphics::PrimitiveGroup>& MeshGetPrimitiveGroups(const MeshId id);
/// get vertex buffer
const BufferId MeshGetVertexBuffer(const MeshId id, const IndexT stream);
/// Get mesh vertex offset
const uint MeshGetVertexOffset(const MeshId id, const IndexT stream);
/// get index buffer
const BufferId MeshGetIndexBuffer(const MeshId id);
/// Get index buffer base offset
const uint MeshGetIndexOffset(const MeshId id);
/// get topology
const CoreGraphics::PrimitiveTopology::Code MeshGetTopology(const MeshId id);

enum
{
    Mesh_Name,
    Mesh_Streams,
    Mesh_IndexBufferOffset,
    Mesh_IndexBuffer,
    Mesh_VertexLayout,
    Mesh_Topology,
    Mesh_PrimitiveGroups
};
typedef Ids::IdAllocatorSafe<
    Resources::ResourceName,
    Util::ArrayStack<VertexStream, 16>,
    SizeT,
    BufferId,
    VertexLayoutId,
    CoreGraphics::PrimitiveTopology::Code,
    Util::Array<CoreGraphics::PrimitiveGroup>
> MeshAllocator;
extern MeshAllocator meshAllocator;

} // CoreGraphics
