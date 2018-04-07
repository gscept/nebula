#pragma once
//------------------------------------------------------------------------------
/**
	@class Physics::PhysicsMeshBase

	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "resources/resource.h"
#include "coregraphics/primitivegroup.h"


namespace Physics
{

class PhysicsMeshBase : public Resources::Resource
{
	__DeclareClass(PhysicsMeshBase);
public:
	/// constructor
	PhysicsMeshBase();
	/// destructor
	virtual ~PhysicsMeshBase();
	
	/// copy vertex and index buffers to mesh object
	virtual void SetMeshData(float * vertexData, uint numVertices, uint verticeStride, uint * indexData, uint numIndices);
	/// declare a mesh component
	virtual void AddMeshComponent(int id, const CoreGraphics::PrimitiveGroup& group) = 0;
protected:
	uint * indexData;
	float * vertexData;
	uint vertexStride;
	uint numVertices;
	uint numIndices;
};

}
