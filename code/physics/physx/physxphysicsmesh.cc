//------------------------------------------------------------------------------
//  physxphysicsmesh.cc
//  (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/physx/physxphysicsmesh.h"
#include "cooking/PxConvexMeshDesc.h"
#include "extensions/PxDefaultStreams.h"
#include "cooking/PxCooking.h"
#include "physxphysicsserver.h"
#include "PxPhysics.h"

using namespace Physics;
using namespace physx;

namespace PhysX
{

__ImplementClass(PhysX::PhysXPhysicsMesh,'PXPM', Physics::PhysicsMeshBase);

//------------------------------------------------------------------------------
/**
*/
PhysXPhysicsMesh::PhysXPhysicsMesh()
{

}

//------------------------------------------------------------------------------
/**
*/
PhysXPhysicsMesh::~PhysXPhysicsMesh()
{
	
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXPhysicsMesh::AddMeshComponent(int id, const CoreGraphics::PrimitiveGroup& group)
{
	this->meshes.Add(id, group);
}

//------------------------------------------------------------------------------
/**
*/
physx::PxConvexMesh*
PhysXPhysicsMesh::GetConvexMesh(int primGroup)
{
	const CoreGraphics::PrimitiveGroup& group = this->meshes[primGroup];

	PxConvexMeshDesc convexDesc;
	convexDesc.points.count = this->numVertices;
    convexDesc.points.stride = this->vertexStride * sizeof(float);
	convexDesc.points.data = this->vertexData;
    convexDesc.triangles.count = group.GetNumPrimitives();
    convexDesc.triangles.data = (void*)&(this->indexData[group.GetBaseIndex()]);	
	convexDesc.triangles.stride = 3 * sizeof(unsigned int);
    convexDesc.flags = PxConvexFlag::eINFLATE_CONVEX;

	// physx does not support convex meshes with more than 256 triangles, let it build its own convex hull instead
	if(convexDesc.triangles.count >255)
	{ 
		n_warning("Mesh %s has %d triangles which exceeds the 256 limit, creating convex hull", this->GetResourceId().AsString().AsCharPtr(), convexDesc.triangles.count);
		convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX | PxConvexFlag::eINFLATE_CONVEX;
	}

	PxDefaultMemoryOutputStream buf;
	PxConvexMeshCookingResult::Enum result;
	if (!PhysXServer::Instance()->cooking->cookConvexMesh(convexDesc, buf, &result))
	{
		n_error("Failed to cook convex mesh");
	}		
	PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
	return PhysXServer::Instance()->physics->createConvexMesh(input);	
}

//------------------------------------------------------------------------------
/**
*/
physx::PxTriangleMesh*
PhysXPhysicsMesh::GetMesh(int primGroup)
{
	PxTolerancesScale scale;
	PxCookingParams params(scale);
	// disable mesh cleaning - perform mesh validation on development configurations
	params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
	// disable edge precompute, edges are set for each triangle, slows contact generation
	params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
	// lower hierarchy for internal mesh
	params.meshCookingHint = PxMeshCookingHint::eCOOKING_PERFORMANCE;

	PhysXServer::Instance()->cooking->setParams(params);

	const CoreGraphics::PrimitiveGroup& group = this->meshes[primGroup];

	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = this->numVertices;
	meshDesc.points.stride = this->vertexStride * sizeof(float);
	meshDesc.points.data = this->vertexData;

	meshDesc.triangles.count = group.GetNumPrimitives();
	meshDesc.triangles.stride = 3 * sizeof(unsigned int);
	meshDesc.triangles.data = (void*)&(this->indexData[group.GetBaseIndex()]);
	
#ifdef _DEBUG
	// mesh should be validated before cooked without the mesh cleaning
	bool res = PhysXServer::Instance()->cooking->validateTriangleMesh(meshDesc);
	//n_assert(res);
#endif

	PxTriangleMesh* aTriangleMesh = PhysXServer::Instance()->cooking->createTriangleMesh(meshDesc, PhysXServer::Instance()->physics->getPhysicsInsertionCallback());
	return aTriangleMesh;
}

//------------------------------------------------------------------------------
/**
*/
physx::PxConvexMesh*
PhysXPhysicsMesh::GetConvexHullMesh(int primGroup)
{
	const CoreGraphics::PrimitiveGroup& group = this->meshes[primGroup];

	PxConvexMeshDesc convexDesc;
	convexDesc.points.count = this->numVertices;
	convexDesc.points.stride = this->vertexStride * sizeof(float);
	convexDesc.points.data = this->vertexData;
    convexDesc.triangles.count = group.GetNumPrimitives();
	convexDesc.triangles.data = (void*)&(this->indexData[group.GetBaseIndex()]);
	convexDesc.triangles.stride = 3 * sizeof(unsigned int);
	convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX| PxConvexFlag::eINFLATE_CONVEX;    

	PxDefaultMemoryOutputStream buf;
	PxConvexMeshCookingResult::Enum result;
	if (!PhysXServer::Instance()->cooking->cookConvexMesh(convexDesc, buf, &result))
	{
		n_error("Failed to cook convex mesh");
	}
	PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
	return PhysXServer::Instance()->physics->createConvexMesh(input);
}

}