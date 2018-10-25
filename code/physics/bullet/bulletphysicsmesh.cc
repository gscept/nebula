//------------------------------------------------------------------------------
//  bulletphysicsmesh.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "physics/bullet/conversion.h"
#include "physics/bullet/bulletphysicsmesh.h"
#include "BulletCollision/Gimpact/btGImpactShape.h"

using namespace Physics;

namespace Bullet
{

__ImplementClass(Bullet::BulletPhysicsMesh,'PBPM', Physics::PhysicsMeshBase);

//------------------------------------------------------------------------------
/**
*/
BulletPhysicsMesh::BulletPhysicsMesh() 
{

}

//------------------------------------------------------------------------------
/**
*/
BulletPhysicsMesh::~BulletPhysicsMesh()
{
	this->meshes.Clear();
}

//------------------------------------------------------------------------------
/**
*/
btCollisionShape* 
BulletPhysicsMesh::GetShape(int primGroup, MeshTopologyType meshType)
{
	btCollisionShape * shape;
	btTriangleIndexVertexArray *mesh = n_new(btTriangleIndexVertexArray());	
	switch(meshType)
	{
	case MeshConcave:
		{
			mesh->addIndexedMesh(this->meshes[primGroup],PHY_INTEGER);
			btGImpactMeshShape * gshape = n_new(btGImpactMeshShape(mesh));			
			
			gshape->setLocalScaling(btVector3(1.f,1.f,1.f));
			gshape->setMargin(0.0f);
			gshape->updateBound();
			shape = (btCollisionShape *)gshape;
		}
		break;
	case MeshConvex:
		{
			
			// this is how its suppoed to be done (in theory, however as bullet documentation says
			// its faster to use the convex hull instead
#if 1
			mesh->addIndexedMesh(this->meshes[primGroup],PHY_INTEGER);
			btConvexTriangleMeshShape * cshape = n_new(btConvexTriangleMeshShape(mesh));
#else
			btConvexHullShape * cshape = n_new(btConvexHullShape((btScalar*)this->meshes[primGroup].m_vertexBase,this->meshes[primGroup].m_numVertices,this->meshes[primGroup].m_vertexStride));
#endif
			

			cshape->setLocalScaling((btVector3(1.f,1.f,1.f)));
			cshape->setMargin(0.f);
			shape = (btCollisionShape*) cshape;
		}
		break;
	case MeshConvexHull:
		{
			btConvexHullShape * cshape = n_new(btConvexHullShape((btScalar*)this->meshes[primGroup].m_vertexBase,this->meshes[primGroup].m_numVertices,this->meshes[primGroup].m_vertexStride));
			cshape->setLocalScaling(btVector3(1.f,1.f,1.f));
			cshape->setMargin(0.f);
			shape = (btCollisionShape *)cshape;
		}
		break;
	case MeshStatic:
		{
			mesh->addIndexedMesh(this->meshes[primGroup],PHY_INTEGER);
			btBvhTriangleMeshShape * bshape = n_new(btBvhTriangleMeshShape(mesh,true));
			shape = (btCollisionShape *)bshape;
		}
		break;
	default:
		n_error("Not implemented mesh topology type");

	}
	return shape;
}

//------------------------------------------------------------------------------
/**
*/
void
BulletPhysicsMesh::AddMeshComponent(int id, const CoreGraphics::PrimitiveGroup& group)
{
	
	btIndexedMesh meshData;
	meshData.m_indexType = PHY_INTEGER;
	
	meshData.m_numTriangles = group.GetNumPrimitives(CoreGraphics::PrimitiveTopology::TriangleList);
	meshData.m_numVertices = group.GetNumVertices();

	size_t indexbytes = meshData.m_numTriangles * sizeof(uint) * 3;
	meshData.m_triangleIndexBase = (const unsigned char*)&(this->indexData[group.GetBaseIndex()]);	
				
	size_t vertexbytes = meshData.m_numVertices * this->vertexStride * sizeof(float);
	meshData.m_vertexBase = (const unsigned char*)this->vertexData;
	
	meshData.m_triangleIndexStride = 3 * sizeof(uint);
	meshData.m_vertexStride = this->vertexStride * sizeof(float);
	meshData.m_vertexType = PHY_FLOAT;
			
	this->meshes.Add(id,meshData);	
}

}