#pragma once
//------------------------------------------------------------------------------
/**
	@class Bullet::BulletPhysicsMesh

	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "physics/resource/physicsmeshbase.h"
#include "physics/model/templates.h"
#include "coregraphics/primitivegroup.h"

class btCollisionShape;
class btBvhTriangleMeshShape;
class btTriangleIndexVertexArray;
class btGImpactMeshShape;
struct btIndexedMesh;

namespace Bullet
{

class BulletPhysicsMesh : public Physics::PhysicsMeshBase
{
	__DeclareClass(BulletPhysicsMesh);
public:
	/// constructor
	BulletPhysicsMesh();
	/// destructor
	~BulletPhysicsMesh();
	
	/// define a mesh component (primitivegroup)
	virtual void AddMeshComponent(int id, const CoreGraphics::PrimitiveGroup& group);
	/// create a collisionshape object from a primitivegroup
	btCollisionShape* GetShape(int primGroup, Physics::MeshTopologyType meshtype);
private:	
	Util::Dictionary<int,btIndexedMesh> meshes;

};
}	