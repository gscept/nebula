//------------------------------------------------------------------------------
//  emittermesh.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "particles/emittermesh.h"

#include "debug/debugfloat.h"

namespace Particles
{
using namespace CoreGraphics;
using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
EmitterMesh::EmitterMesh() :
    numPoints(0),
    points(NULL)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
EmitterMesh::~EmitterMesh()
{
    if (this->IsValid())
    {
        this->Discard();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
EmitterMesh::Setup(const Ptr<Mesh>& mesh, IndexT primGroupIndex)
{
    n_assert(!this->IsValid());

    // we need to extract mesh vertices from the primitive group
    // without duplicate vertices
    const PrimitiveGroup& primGroup = mesh->GetPrimitiveGroupAtIndex(primGroupIndex);
    const Ptr<IndexBuffer>& indexBuffer = mesh->GetIndexBuffer();    
    n_assert(indexBuffer->GetIndexType() == IndexType::Index32);
    const Ptr<VertexBuffer>& vertexBuffer = mesh->GetVertexBuffer();
    SizeT numVBufferVertices = vertexBuffer->GetNumVertices();

    IndexT baseIndex = primGroup.GetBaseIndex();
    SizeT numIndices = primGroup.GetNumIndices();

    int* indices = (int*) indexBuffer->Map(IndexBuffer::MapRead);
	//int indices[] = { 0 };

    // allocate a "flag array" which holds a 0 at a
    // vertex index which hasn't been encountered yet, and
    // a 1 if the index has been encountered before
    SizeT size = numVBufferVertices * sizeof(uchar);
    uchar* flagArray = (uchar*) Memory::Alloc(Memory::ScratchHeap, size);
    Memory::Clear(flagArray, size);

    Array<int> emitterIndices;
    emitterIndices.Reserve(numVBufferVertices);
    IndexT i;
    for (i = baseIndex; i < (baseIndex + numIndices); i++)
    {
        int vertexIndex = indices[i];
        n_assert(vertexIndex < numVBufferVertices);
        if (flagArray[vertexIndex] == 0)
        {
            // this index hasn't been encountered yet
            emitterIndices.Append(vertexIndex);
            flagArray[vertexIndex] = 1;
        }
    }
    Memory::Free(Memory::ScratchHeap, flagArray);
    flagArray = 0;
    indexBuffer->Unmap();

    // the emitterIndices array now contains the indices of all vertices
    // we need to copy
    this->numPoints = emitterIndices.Size();
    this->points = n_new_array(EmitterPoint, this->numPoints);

    // make sure the emitter mesh actually has the components we need
    const Ptr<VertexLayout>& vertexLayout = vertexBuffer->GetVertexLayout();

    IndexT posCompIndex = vertexLayout->FindComponent(VertexComponent::Position, 0);
    n_assert(InvalidIndex != posCompIndex);
    n_assert(vertexLayout->GetComponentAt(posCompIndex).GetFormat() == VertexComponent::Float3);

    IndexT normCompIndex = vertexLayout->FindComponent(VertexComponent::Normal, 0);
    n_assert(InvalidIndex != normCompIndex);
    n_assert(vertexLayout->GetComponentAt(normCompIndex).GetFormat() == VertexComponent::Byte4N);

#ifndef __WII__
    IndexT tanCompIndex = vertexLayout->FindComponent(VertexComponent::Tangent, 0);
    n_assert(InvalidIndex != tanCompIndex);
	n_assert(vertexLayout->GetComponentAt(tanCompIndex).GetFormat() == VertexComponent::Byte4N);
#endif

    // get the byte offset from the start of the vertex
    IndexT posByteOffset = vertexLayout->GetComponentAt(posCompIndex).GetByteOffset();
    IndexT normByteOffset = vertexLayout->GetComponentAt(normCompIndex).GetByteOffset();
#ifndef __WII__
    IndexT tanByteOffset = vertexLayout->GetComponentAt(tanCompIndex).GetByteOffset();
#endif

    // gain access to vertices and transfer vertex info
    const uchar* verts = (uchar*) vertexBuffer->Map(VertexBuffer::MapRead);
    const SizeT vertexByteSize = vertexLayout->GetVertexByteSize();
    for (i = 0; i < this->numPoints; i++)
    {
        const uchar* src = verts + vertexByteSize * emitterIndices[i];
        EmitterPoint &dst = this->points[i];
        dst.position.load_float3(src + posByteOffset, 1.0f);
#ifdef __WII__

        s8 *nrm = (s8*)(src+normByteOffset);
        dst.normal = float4(nrm[0]/64.0f, nrm[1]/64.0f, nrm[2]/64.0f, 0.0f);
        
        // wii has no tangent vectors, make some up
        if (n_abs(dst.normal.x() < 0.1)) 
        {
            dst.tangent = float4::cross3(float4(1.0, 0.0, 0.0, 1.0), dst.normal);
        }
        else
        {
            dst.tangent = float4::cross3(float4(0.0, 1.0, 0.0, 1.0), dst.normal);
        }
#else
        dst.normal.load_byte4n(src + normByteOffset, 0.0f);
        dst.normal = Math::float4::normalize(dst.normal);
		dst.tangent.load_byte4n(src + tanByteOffset, 0.0f);
        dst.tangent = Math::float4::normalize(dst.tangent);
#endif
    }
    vertexBuffer->Unmap();
}

//------------------------------------------------------------------------------
/**
*/
void
EmitterMesh::Discard()
{
    n_assert(this->IsValid());
    n_delete_array(this->points);
    this->points = 0;
    this->numPoints = 0;
}

//------------------------------------------------------------------------------
/**
*/
const EmitterMesh::EmitterPoint& 
EmitterMesh::GetEmitterPoint(IndexT key) const
{
    n_assert(key >= 0);
    n_assert(this->numPoints > 0);
    IndexT pointIndex = key % this->numPoints;
    return this->points[pointIndex];
}

} // namespace Particles
