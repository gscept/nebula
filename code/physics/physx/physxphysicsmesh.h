#pragma once
//------------------------------------------------------------------------------
/**
	@class PhysX::PhysXPhysicsMesh

	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
#include "physics/resource/physicsmeshbase.h"
#include "physics/model/templates.h"
#include "coregraphics/primitivegroup.h"

namespace physx
{
	class PxConvexMesh;
	class PxTriangleMesh;
}

namespace PhysX
{

class PhysXPhysicsMesh : public Physics::PhysicsMeshBase
{
	__DeclareClass(PhysXPhysicsMesh);
public:
	/// constructor
	PhysXPhysicsMesh();
	/// destructor
	~PhysXPhysicsMesh();
	
	/// define a mesh component (primitivegroup)
	virtual void AddMeshComponent(int id, const CoreGraphics::PrimitiveGroup& group);
	/// create a convex collisionshape object from a primitivegroup
	physx::PxConvexMesh* GetConvexMesh(int primGroup);
	/// compute convex hull from input mesh
	physx::PxConvexMesh* GetConvexHullMesh(int primGroup);
	/// create generic collisionshape from the mesh object
	physx::PxTriangleMesh* GetMesh(int primGroup);
private:	
	Util::Dictionary<int, CoreGraphics::PrimitiveGroup> meshes;

};
}	