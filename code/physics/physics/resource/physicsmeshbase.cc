//------------------------------------------------------------------------------
//  physicsmeshbase.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/resource/physicsmeshbase.h"

namespace Physics
{
__ImplementAbstractClass(Physics::PhysicsMeshBase, 'PPMB', Resources::Resource);


//------------------------------------------------------------------------------
/**
*/
PhysicsMeshBase::PhysicsMeshBase() :indexData(NULL), vertexData(NULL), vertexStride(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
PhysicsMeshBase::~PhysicsMeshBase()
{
	if (this->indexData)
	{
		Memory::Free(Memory::PhysicsHeap, (void*)this->indexData);
		this->indexData = NULL;
	}
	if (this->vertexData)
	{
		Memory::Free(Memory::PhysicsHeap, (void*)this->vertexData);
		this->vertexData = NULL;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsMeshBase::SetMeshData(float * vertexData, uint numVertices, uint verticeStride, uint * indexData, uint numIndices)
{
	size_t indexbytes = numIndices * sizeof(uint);
	this->indexData = (uint*)Memory::Alloc(Memory::PhysicsHeap, indexbytes);
	Memory::Copy(indexData, (void*)this->indexData, indexbytes);

	size_t vertexbytes = numVertices * verticeStride * sizeof(float);
	this->vertexData = (float*)Memory::Alloc(Memory::PhysicsHeap, vertexbytes);
	Memory::Copy(vertexData, (void*)this->vertexData, vertexbytes);
	this->vertexStride = verticeStride;
	this->numVertices = numVertices;
	this->numIndices = numIndices;
}

}
