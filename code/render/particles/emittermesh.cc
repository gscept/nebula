//------------------------------------------------------------------------------
//  emittermesh.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
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
EmitterMesh::Setup(const CoreGraphics::MeshId mesh, IndexT primGroupIndex)
{
    n_assert(!this->IsValid());

    // we need to extract mesh vertices from the primitive group
    // without duplicate vertices
	const PrimitiveGroup& primGroup = MeshGetPrimitiveGroups(mesh)[primGroupIndex];// mesh->GetPrimitiveGroupAtIndex(primGroupIndex);
    const IndexBufferId indexBuffer = MeshGetIndexBuffer(mesh);    
    n_assert(IndexBufferGetType(indexBuffer) == IndexType::Index32);
    const VertexBufferId vertexBuffer = MeshGetVertexBuffer(mesh, 0);
    SizeT numVBufferVertices = VertexBufferGetNumVertices(vertexBuffer);

    IndexT baseIndex = primGroup.GetBaseIndex();
    SizeT numIndices = primGroup.GetNumIndices();

    int* indices = (int*) IndexBufferMap(indexBuffer, GpuBufferTypes::MapRead);
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
	IndexBufferUnmap(indexBuffer);

    // the emitterIndices array now contains the indices of all vertices
    // we need to copy
    this->numPoints = emitterIndices.Size();
    this->points = n_new_array(EmitterPoint, this->numPoints);

    // make sure the emitter mesh actually has the components we need
	const VertexLayoutId& vertexLayout = MeshGetPrimitiveGroups(mesh)[primGroupIndex].GetVertexLayout();

	const Util::Array<VertexComponent>& comps = VertexLayoutGetComponents(vertexLayout);
	bool posValid = false, normValid = false, tanValid = false;
	IndexT posByteOffset = 0, normByteOffset = 0, tanByteOffset = 0;
	for (i = 0; i < comps.Size(); i++)
	{
		const VertexComponent& comp = comps[i];
		const VertexComponent::SemanticName name = comp.GetSemanticName();
		const VertexComponent::Format fmt = comp.GetFormat();
		if (name == VertexComponent::Position && fmt == VertexComponent::Float3)
			posValid = true, posByteOffset = comp.GetByteOffset();
		else if (name == VertexComponent::Normal && fmt == VertexComponent::Byte4N)
			normValid = true, normByteOffset = comp.GetByteOffset();
		else if (name == VertexComponent::Tangent && fmt == VertexComponent::Byte4N)
			tanValid = true, tanByteOffset = comp.GetByteOffset();
	}
	n_assert(posValid && normValid && tanValid);


    // gain access to vertices and transfer vertex info
    const uchar* verts = (uchar*) VertexBufferMap(vertexBuffer, GpuBufferTypes::MapRead);
    const SizeT vertexByteSize = VertexLayoutGetSize(vertexLayout);
    for (i = 0; i < this->numPoints; i++)
    {
        const uchar* src = verts + vertexByteSize * emitterIndices[i];
        EmitterPoint &dst = this->points[i];
        dst.position.load_float3(src + posByteOffset, 1.0f);
        dst.normal.load_byte4n(src + normByteOffset, 0.0f);
        dst.normal = Math::normalize(dst.normal);
		dst.tangent.load_byte4n(src + tanByteOffset, 0.0f);
        dst.tangent = Math::normalize(dst.tangent);
    }
	VertexBufferUnmap(vertexBuffer);
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
