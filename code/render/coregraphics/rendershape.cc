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
    numPrimitives(0),
    vertexWidth(0),
    numVertices(0),
    indexType(IndexType::None),
    color(1.0f, 1.0f, 1.0f, 1.0f),
    vertexDataOffset(0),
    vertexLayout(NULL),
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
    numPrimitives(0),
    vertexWidth(0),
    numVertices(0),
    indexType(IndexType::None),
    color(color_),
    vertexDataOffset(0),
    vertexLayout(NULL),
    groupIndex(InvalidIndex)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RenderShape::SetupSimpleShape(Type shapeType_, RenderFlag depthFlag_, const mat4& modelTransform_, const vec4& color_)
{
    n_assert(!this->IsValid());
    n_assert(!((Primitives == shapeType) || (IndexedPrimitives == shapeType)));
    this->shapeType      = shapeType_;
    this->depthFlag      = depthFlag_;
    this->modelTransform = modelTransform_;
    this->color          = color_;
}

//------------------------------------------------------------------------------
/**
*/
void
RenderShape::SetupPrimitives(const Math::mat4& modelTransform_, PrimitiveTopology::Code topology_, SizeT numPrimitives_, const RenderShape::RenderShapeVertex* vertices_, const Math::vec4& color_, RenderFlag depthFlag_)
{
    n_assert(!this->IsValid());
    
    this->shapeType        = Primitives;
    this->depthFlag        = depthFlag_;
    this->modelTransform   = modelTransform_;
    this->topology         = topology_;
    this->numPrimitives    = numPrimitives_;
    this->vertexWidth      = sizeof(RenderShape::RenderShapeVertex);
    this->color            = color_;
    this->vertexDataOffset = 0;

    // setup a memory stream and copy the vertex data
    SizeT numVertices = PrimitiveTopology::NumberOfVertices(this->topology, this->numPrimitives);
    SizeT bufferSize = numVertices * this->vertexWidth;
    this->dataStream = MemoryStream::Create();
    this->dataStream->SetSize(bufferSize);
    this->dataStream->SetAccessMode(Stream::WriteAccess);
    this->dataStream->Open();
    this->dataStream->Write(vertices_, bufferSize);
    this->dataStream->Close();
}

//------------------------------------------------------------------------------
/**
*/
void
RenderShape::SetupIndexedPrimitives(const Math::mat4& modelTransform_, PrimitiveTopology::Code topology_, SizeT numPrimitives_, const RenderShape::RenderShapeVertex* vertices_, SizeT numVertices_, const void* indices_, IndexType::Code indexType_, const Math::vec4& color_, RenderFlag depthFlag_)
{
    n_assert(!this->IsValid());

    this->shapeType      = IndexedPrimitives;
    this->depthFlag      = depthFlag_;
    this->modelTransform = modelTransform_;
    this->topology       = topology_;
    this->numPrimitives  = numPrimitives_;
    this->vertexWidth    = sizeof(RenderShape::RenderShapeVertex);
    this->numVertices    = numVertices_;
    this->indexType      = indexType_;
    this->color          = color_;

    // compute index buffer and vertex buffer sizes
    SizeT numIndices = PrimitiveTopology::NumberOfVertices(topology, numPrimitives);
    SizeT indexBufferSize = numIndices * IndexType::SizeOf(indexType);
    SizeT vertexBufferSize = this->numVertices * vertexWidth;
    SizeT bufferSize = indexBufferSize + vertexBufferSize;
    this->vertexDataOffset = indexBufferSize;

    // setup a memory stream and copy the vertex and index data there
    this->dataStream = MemoryStream::Create();
    this->dataStream->SetSize(bufferSize);
    this->dataStream->SetAccessMode(Stream::WriteAccess);
    this->dataStream->Open();
    this->dataStream->Write(indices_, indexBufferSize);
    this->dataStream->Write(vertices_, vertexBufferSize);
    this->dataStream->Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
RenderShape::SetupMesh(const Math::mat4& modelTransform, const MeshId mesh, const IndexT groupIndex, const Math::vec4& color, RenderFlag depthFlag)
{
    n_assert(!this->IsValid());
    n_assert(mesh != MeshId::Invalid());

    this->shapeType      = RenderMesh;
    this->depthFlag      = depthFlag;
    this->modelTransform = modelTransform;
    this->color          = color;
    this->mesh           = mesh;
    this->groupIndex     = groupIndex;
}

} // namespace CoreGraphics
