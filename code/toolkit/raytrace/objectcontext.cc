//------------------------------------------------------------------------------
// objectcontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "objectcontext.h"

namespace Toolkit
{
__ImplementClass(Toolkit::ObjectContext, 'OBCX', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
ObjectContext::ObjectContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ObjectContext::~ObjectContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
ObjectContext::Setup()
{
	// copy from temporarily mapped buffer 
	SizeT vertexBufSize = this->mesh.numVertices * this->mesh.vertexSize * sizeof(float);
	SizeT indexBufSize = this->mesh.numIndices * sizeof(int);
	float* vertexCopy = new float[vertexBufSize];
	ushort* indexCopy = new ushort[indexBufSize];
	memcpy(vertexCopy, this->mesh.vertices, vertexBufSize);
	memcpy(indexCopy, this->mesh.indices, indexBufSize);

	this->mesh.vertices = vertexCopy;
	this->mesh.indices = indexCopy;
}

} // namespace Toolkit