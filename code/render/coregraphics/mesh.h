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

RESOURCE_ID_TYPE(MeshId);

struct MeshCreateInfo
{
    struct Stream
    {
        BufferId vertexBuffer;
        IndexT index;
    };

    Resources::ResourceName name;
    Util::StringAtom tag;
    Util::ArrayStack<Stream, 16> streams;
    BufferId indexBuffer;
    VertexLayoutId vertexLayout;
    CoreGraphics::PrimitiveTopology::Code topology;
    Util::Array<CoreGraphics::PrimitiveGroup> primitiveGroups;
};

/// create new mesh
const MeshId CreateMesh(const MeshCreateInfo& info);
/// destroy mesh
void DestroyMesh(const MeshId id);

/// bind mesh and primitive group
void MeshBind(const MeshId id, const IndexT prim);
/// get number of primitive groups
const Util::Array<CoreGraphics::PrimitiveGroup>& MeshGetPrimitiveGroups(const MeshId id);
/// get vertex buffer
const BufferId MeshGetVertexBuffer(const MeshId id, const IndexT stream);
/// get index buffer
const BufferId MeshGetIndexBuffer(const MeshId id);
/// get topology
const CoreGraphics::PrimitiveTopology::Code MeshGetTopology(const MeshId id);

class MemoryMeshCache;
extern MemoryMeshCache* meshPool;
} // CoreGraphics
