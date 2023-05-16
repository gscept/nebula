//------------------------------------------------------------------------------
//  @file geometryhelpers.cc
//  @copyright (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "geometryhelpers.h"
namespace RenderUtil
{

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::MeshId
GeometryHelpers::CreateRectangle()
{
    struct Vertex {
        Math::float3 pos;
        Math::float2 uv;
    };
    static Vertex vertices[] = {
        Vertex{ Math::float3{ -0.5, -0.5, 0 }, Math::float2{ 0, 0 } }
        , Vertex{ Math::float3{ -0.5, 0.5, 0 }, Math::float2 { 0, 1 } }
        , Vertex{ Math::float3{ 0.5, 0.5, 0 }, Math::float2{ 1, 1 } }
        , Vertex{ Math::float3{ 0.5, -0.5, 0 }, Math::float2{ 1, 0 } }
    };
    static ushort indices[] = {
        0, 1, 2, 0, 2, 3
    };

    Geometry geo;
    CoreGraphics::BufferCreateInfo vboInfo;
    vboInfo.elementSize = sizeof(Vertex);
    vboInfo.size = 4;
    vboInfo.usageFlags = CoreGraphics::BufferUsageFlag::VertexBuffer;
    vboInfo.data = vertices;
    vboInfo.dataSize = sizeof(vertices);

    CoreGraphics::BufferCreateInfo iboInfo;
    iboInfo.elementSize = sizeof(ushort);
    iboInfo.size = 6;
    iboInfo.usageFlags = CoreGraphics::BufferUsageFlag::IndexBuffer;
    iboInfo.data = indices;
    iboInfo.dataSize = sizeof(indices);

    CoreGraphics::MeshCreateInfo meshInfo;
    meshInfo.indexBuffer = CoreGraphics::CreateBuffer(iboInfo);
    meshInfo.indexBufferOffset = 0;
    meshInfo.indexType = CoreGraphics::IndexType::Index16;
    CoreGraphics::VertexStream stream;
    stream.vertexBuffer = CoreGraphics::CreateBuffer(vboInfo);
    stream.offset = 0;
    stream.index = 0;
    meshInfo.streams.Append(stream);

    CoreGraphics::PrimitiveGroup primGroup;
    primGroup.SetNumIndices(6);
   
    meshInfo.primitiveGroups.Append(primGroup);
    meshInfo.topology = CoreGraphics::PrimitiveTopology::TriangleList;

    CoreGraphics::VertexLayoutCreateInfo vloInfo;
    vloInfo.comps = {
        CoreGraphics::VertexComponent{ 0, CoreGraphics::VertexComponent::Float3 }
        , CoreGraphics::VertexComponent{ 0, CoreGraphics::VertexComponent::Float2 }
    };
    meshInfo.vertexLayout = CoreGraphics::CreateVertexLayout(vloInfo);

    return CoreGraphics::CreateMesh(meshInfo);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::MeshId
GeometryHelpers::CreateDisk(SizeT numPoints)
{
    struct Vertex {
        Math::float3 pos;
        Math::float2 uv;
    };

    Util::Array<Vertex> vertices = {
        Vertex{ Math::float3{ 0, 0, 0 }, Math::float2{ 0, 0 } }
    };

    Util::Array<ushort> indices = {
        0
    };

    for (int i = 0; i < numPoints + 1; i++)
    {
        Vertex v;
        v.pos = Math::float3{ sin(i * 2 * N_PI / float(numPoints)) * 0.5f, cos(i * 2 * N_PI / float(numPoints)) * 0.5f, 0 };
        v.uv = Math::float2{ 0, 0 };
        vertices.Append(v);
        indices.Append(i + 1);
    }

    Geometry geo;
    CoreGraphics::BufferCreateInfo vboInfo;
    vboInfo.elementSize = sizeof(Vertex);
    vboInfo.size = vertices.Size();
    vboInfo.usageFlags = CoreGraphics::BufferUsageFlag::VertexBuffer;
    vboInfo.data = vertices.Begin();
    vboInfo.dataSize = vertices.ByteSize();

    CoreGraphics::BufferCreateInfo iboInfo;
    iboInfo.elementSize = sizeof(ushort);
    iboInfo.size = indices.Size();
    iboInfo.usageFlags = CoreGraphics::BufferUsageFlag::IndexBuffer;
    iboInfo.data = indices.Begin();
    iboInfo.dataSize = indices.ByteSize();

    CoreGraphics::MeshCreateInfo meshInfo;
    meshInfo.indexBuffer = CoreGraphics::CreateBuffer(iboInfo);
    meshInfo.indexBufferOffset = 0;
    meshInfo.indexType = CoreGraphics::IndexType::Index16;
    CoreGraphics::VertexStream stream;
    stream.vertexBuffer = CoreGraphics::CreateBuffer(vboInfo);
    stream.offset = 0;
    stream.index = 0;
    meshInfo.streams.Append(stream);

    CoreGraphics::PrimitiveGroup primGroup;
    primGroup.SetNumIndices(indices.Size());

    meshInfo.primitiveGroups.Append(primGroup);
    meshInfo.topology = CoreGraphics::PrimitiveTopology::TriangleFan;

    CoreGraphics::VertexLayoutCreateInfo vloInfo;
    vloInfo.comps.Append(CoreGraphics::VertexComponent{ 0, CoreGraphics::VertexComponent::Float3 });
    vloInfo.comps.Append(CoreGraphics::VertexComponent{ 0, CoreGraphics::VertexComponent::Float2 });
    meshInfo.vertexLayout = CoreGraphics::CreateVertexLayout(vloInfo);

    return CoreGraphics::CreateMesh(meshInfo);
}

} // namespace RenderUtil
