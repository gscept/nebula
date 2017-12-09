//------------------------------------------------------------------------------
//  rendershape.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
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
    threadId(InvalidThreadId),
    shapeType(InvalidShapeType),
	depthFlag(CheckDepth),
    topology(PrimitiveTopology::InvalidPrimitiveTopology),
    numPrimitives(0),
    vertexWidth(0),
    numVertices(0),
    indexType(IndexType::None),
    color(1.0f, 1.0f, 1.0f, 1.0f),
    vertexDataOffset(0),
    vertexLayout(NULL)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
RenderShape::RenderShape(ThreadId threadId_, Type shapeType_, RenderFlag depthFlag_, const matrix44& modelTransform_, const float4& color_) :
    threadId(threadId_),
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
    vertexLayout(NULL)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RenderShape::SetupSimpleShape(ThreadId threadId_, Type shapeType_, RenderFlag depthFlag_, const matrix44& modelTransform_, const float4& color_)
{
    n_assert(!this->IsValid());
    n_assert(!((Primitives == shapeType) || (IndexedPrimitives == shapeType)));
    this->threadId       = threadId_;
    this->shapeType      = shapeType_;
	this->depthFlag		 = depthFlag_;
    this->modelTransform = modelTransform_;
    this->color          = color_;
}

//------------------------------------------------------------------------------
/**
*/
void
RenderShape::SetupPrimitives(ThreadId threadId_, const Math::matrix44& modelTransform_, PrimitiveTopology::Code topology_, SizeT numPrimitives_, const RenderShape::RenderShapeVertex* vertices_, const Math::float4& color_, RenderFlag depthFlag_)
{
    n_assert(!this->IsValid());
    
    this->threadId         = threadId_;
    this->shapeType        = Primitives;
	this->depthFlag		   = depthFlag_;
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
RenderShape::SetupIndexedPrimitives(ThreadId threadId_, const Math::matrix44& modelTransform_, PrimitiveTopology::Code topology_, SizeT numPrimitives_, const RenderShape::RenderShapeVertex* vertices_, SizeT numVertices_, const void* indices_, IndexType::Code indexType_, const Math::float4& color_, RenderFlag depthFlag_)
{
    n_assert(!this->IsValid());

    this->threadId       = threadId_;
	this->shapeType      = IndexedPrimitives;
	this->depthFlag		 = depthFlag_;
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
RenderShape::SetupMesh(Threading::ThreadId threadId, const Math::matrix44& modelTransform, const MeshId mesh, const IndexT groupIndex, const Math::float4& color, RenderFlag depthFlag)
{
	n_assert(!this->IsValid());
	n_assert(mesh.isvalid());

	this->threadId       = threadId;
	this->shapeType      = RenderMesh;
	this->depthFlag		 = depthFlag;
	this->modelTransform = modelTransform;
	this->color          = color;
	this->mesh			 = mesh;
	this->groupIndex	 = groupIndex;
}

} // namespace CoreGraphics
