//------------------------------------------------------------------------------
//  rendershape.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/rendershape.h"

namespace CoreGraphics
{
using namespace Threading;
using namespace Util;
using namespace Math;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
RenderShape::RenderShape() :
    shapeType(InvalidShapeType),
    depthFlag(CheckDepth),
    topology(PrimitiveTopology::InvalidPrimitiveTopology),
    indexType(IndexType::None),
    numVertices(0),
    numIndices(0),
    color(1.0f, 1.0f, 1.0f, 1.0f),
    vertexDataOffset(0),
    vertexLayout(InvalidVertexLayoutId),
    groupIndex(InvalidIndex)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
RenderShape::RenderShape(Type shapeType_, RenderFlag depthFlag_, const mat4& modelTransform_, const vec4& color_) :
    shapeType(shapeType_),
    depthFlag(depthFlag_),
    modelTransform(modelTransform_),
    topology(PrimitiveTopology::InvalidPrimitiveTopology),
    indexType(IndexType::None),
    numVertices(0),
    numIndices(0),
    color(color_),
    lineThickness(1.0f),
    vertexDataOffset(0),
    vertexLayout(InvalidVertexLayoutId),
    groupIndex(InvalidIndex)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RenderShape::SetupSimpleShape(Type shapeType, RenderFlag depthFlag, const vec4& color, const mat4& modelTransform, float lineThickness)
{
    n_assert(!this->IsValid());
    n_assert(!((Primitives == shapeType) || (IndexedPrimitives == shapeType)));
    this->shapeType      = shapeType;
    this->depthFlag      = depthFlag;
    this->modelTransform = modelTransform;
    this->color          = color;
    this->lineThickness = lineThickness;
}

//------------------------------------------------------------------------------
/**
*/
void
RenderShape::SetupPrimitives(
    const RenderShape::RenderShapeVertex* vertices
    , SizeT numVertices
    , PrimitiveTopology::Code topology
    , RenderFlag depthFlag
    , const Math::mat4 transform
    , float lineThickness)
{
    n_assert(!this->IsValid());

    this->shapeType = Primitives;
    this->depthFlag = depthFlag;
    this->modelTransform = transform;
    this->topology = topology;
    this->numVertices = numVertices;
    this->vertexDataOffset = 0;
    this->lineThickness = lineThickness;

    // setup a memory stream and copy the vertex data
    SizeT bufferSize = numVertices * sizeof(RenderShape::RenderShapeVertex);
    this->dataStream = MemoryStream::Create();
    this->dataStream->SetSize(bufferSize);
    this->dataStream->SetAccessMode(IO::Stream::WriteAccess);
    this->dataStream->Open();
    this->dataStream->Write(vertices, bufferSize);
    this->dataStream->Close();
}

//------------------------------------------------------------------------------
/**
*/
void
RenderShape::SetupPrimitives(
    const Util::Array<RenderShape::RenderShapeVertex> vertices
    , PrimitiveTopology::Code topology
    , RenderFlag depthFlag
    , const Math::mat4 transform
    , float lineThickness)
{
    n_assert(!this->IsValid());

    this->shapeType = Primitives;
    this->depthFlag = depthFlag;
    this->modelTransform = transform;
    this->topology = topology;
    this->numVertices = vertices.Size();
    this->vertexDataOffset = 0;
    this->lineThickness = lineThickness;

    // setup a memory stream and copy the vertex data
    this->dataStream = MemoryStream::Create();
    this->dataStream->SetSize(vertices.ByteSize());
    this->dataStream->SetAccessMode(Stream::WriteAccess);
    this->dataStream->Open();
    this->dataStream->Write(vertices.Begin(), vertices.ByteSize());
    this->dataStream->Close();
}

//------------------------------------------------------------------------------
/**
*/
void
RenderShape::SetupIndexPrimitives(
    const RenderShape::RenderShapeVertex* vertices
    , SizeT numVertices
    , const void* indices
    , SizeT numIndices
    , IndexType::Code indexType
    , PrimitiveTopology::Code topology
    , RenderFlag depthFlag
    , const Math::mat4 transform
    , float lineThickness)
{
    n_assert(!this->IsValid());

    this->shapeType = IndexedPrimitives;
    this->depthFlag = depthFlag;
    this->modelTransform = transform;
    this->topology = topology;
    this->numIndices = numIndices;
    this->numVertices = numVertices;
    this->vertexDataOffset = 0;
    this->indexType = indexType;
    this->lineThickness = lineThickness;

    // setup a memory stream and copy the vertex data
    SizeT vertexBufferSize = numVertices * sizeof(RenderShape::RenderShapeVertex);
    SizeT indexBufferSize = numIndices * IndexType::SizeOf(indexType);
    this->vertexDataOffset = vertexBufferSize;
    this->dataStream = MemoryStream::Create();
    this->dataStream->SetSize(vertexBufferSize + indexBufferSize);
    this->dataStream->SetAccessMode(Stream::WriteAccess);
    this->dataStream->Open();
    this->dataStream->Write(vertices, vertexBufferSize);
    this->dataStream->Write(indices, indexBufferSize);
    this->dataStream->Close();
}

//------------------------------------------------------------------------------
/**
*/
void
RenderShape::SetupIndexPrimitives(
    const Util::Array<RenderShape::RenderShapeVertex> vertices
    , const Util::Array<uint16> indices
    , PrimitiveTopology::Code topology
    , RenderFlag depthFlag
    , const Math::mat4 transform
    , float lineThickness)
{
    n_assert(!this->IsValid());

    this->shapeType = IndexedPrimitives;
    this->depthFlag = depthFlag;
    this->modelTransform = transform;
    this->topology = topology;
    this->numIndices = vertices.Size();
    this->numVertices = indices.Size();
    this->indexType = IndexType::Code::Index16;
    this->lineThickness = lineThickness;

    // setup a memory stream and copy the vertex data
    this->vertexDataOffset = vertices.ByteSize();
    this->dataStream = MemoryStream::Create();
    this->dataStream->SetSize(vertices.ByteSize() + indices.ByteSize());
    this->dataStream->SetAccessMode(Stream::WriteAccess);
    this->dataStream->Open();
    this->dataStream->Write(vertices.Begin(), vertices.ByteSize());
    this->dataStream->Write(indices.Begin(), indices.ByteSize());
    this->dataStream->Close();
}

//------------------------------------------------------------------------------
/**
*/
void
RenderShape::SetupIndexPrimitives(
    const Util::Array<RenderShape::RenderShapeVertex> vertices
    , const Util::Array<uint32> indices
    , PrimitiveTopology::Code topology
    , RenderFlag depthFlag
    , const Math::mat4 transform
    , float lineThickness)
{
    n_assert(!this->IsValid());

    this->shapeType = IndexedPrimitives;
    this->depthFlag = depthFlag;
    this->modelTransform = transform;
    this->topology = topology;
    this->numIndices = vertices.Size();
    this->numVertices = indices.Size();
    this->indexType = IndexType::Code::Index32;
    this->lineThickness = lineThickness;

    // setup a memory stream and copy the vertex data
    this->vertexDataOffset = vertices.ByteSize();
    this->dataStream = MemoryStream::Create();
    this->dataStream->SetSize(vertices.ByteSize() + indices.ByteSize());
    this->dataStream->SetAccessMode(Stream::WriteAccess);
    this->dataStream->Open();
    this->dataStream->Write(vertices.Begin(), vertices.ByteSize());
    this->dataStream->Write(indices.Begin(), indices.ByteSize());
    this->dataStream->Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
RenderShape::SetupMesh(const MeshId mesh, const IndexT groupIndex, const Math::vec4& color, RenderFlag depthFlag, const Math::mat4& modelTransform, float lineThickness)
{
    n_assert(!this->IsValid());
    n_assert(mesh != InvalidMeshId);

    this->shapeType      = RenderMesh;
    this->depthFlag      = depthFlag;
    this->modelTransform = modelTransform;
    this->color          = color;
    this->mesh           = mesh;
    this->groupIndex     = groupIndex;
    this->lineThickness = lineThickness;
}

} // namespace CoreGraphics
