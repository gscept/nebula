//------------------------------------------------------------------------------
//  bulletcollider.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "physics/bullet/conversion.h"
#include "physics/collider.h"
#include "math/bbox.h"
#include "resources/resourcemanager.h"


namespace Bullet
{
	__ImplementClass(Bullet::BulletCollider, 'PBCO', Physics::BaseCollider);

using namespace Math;
using namespace Physics;

//------------------------------------------------------------------------------
/**
*/
BulletCollider::BulletCollider()
{
	this->shape = (btCompoundShape *)n_new(btCompoundShape(true));
}

//------------------------------------------------------------------------------
/**
*/
BulletCollider::~BulletCollider()
{
	n_delete(shape);
	this->shape = 0;
	this->meshes.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCollider::AddPlane( const Math::plane &plane, const Math::matrix44 &localTransform)
{	
	btCollisionShape * childShape = (btCollisionShape *)n_new(btStaticPlaneShape(btVector3(plane.a(),plane.b(),plane.c()),plane.d()));
	this->shape->addChildShape(Neb2BtM44Transform(localTransform),childShape);
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCollider::AddBox( const Math::vector &halfWidth, const Math::matrix44 &localTransform )
{	
	btCollisionShape * childShape = (btCollisionShape *)n_new(btBoxShape(Neb2BtVector(halfWidth)));
	this->shape->addChildShape(Neb2BtM44Transform(localTransform),childShape);
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCollider::AddBox( const Math::bbox & box)
{
	Math::matrix44 trans;
	trans.translate(Math::vector(box.center()));
	AddBox(box.extents(),trans);
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCollider::AddSphere( float radius, const Math::matrix44 &localTransform )
{
	btCollisionShape * childShape = (btCollisionShape *)n_new(btSphereShape(radius));
	this->shape->addChildShape(Neb2BtM44Transform(localTransform),childShape);
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCollider::AddCapsule(float radius, float height, const Math::matrix44  &localTransform )
{
	btCollisionShape * childShape = (btCollisionShape *)n_new(btCapsuleShape(radius,height));
	this->shape->addChildShape(Neb2BtM44Transform(localTransform),childShape);
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCollider::AddCylinder(float radius, float height, const Math::matrix44  &localTransform )
{
	btVector3 cylinderSize = btVector3(radius, height, 1);
	btCollisionShape * childShape = (btCollisionShape *)n_new(btCylinderShape(cylinderSize));
	this->shape->addChildShape(Neb2BtM44Transform(localTransform),childShape);
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCollider::AddCapsule(const Math::vector& pointA, const Math::vector& pointB, float radius, const Math::matrix44 &localTransform)
{
	n_error("BulletCollider::AddCapsule: Not implemented");
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCollider::AddPhysicsMesh(Ptr<Physics::ManagedPhysicsMesh> colliderMesh, const Math::matrix44 & localTransform, Physics::MeshTopologyType meshType, int primGroup)
{
	meshes.Append(colliderMesh);
	this->shape->addChildShape(Neb2BtM44Transform(localTransform),colliderMesh->GetMesh().cast<BulletPhysicsMesh>()->GetShape(primGroup,meshType));
}

//------------------------------------------------------------------------------
/**
*/
Ptr<BulletCollider> 
BulletCollider::GetScaledCopy(const vector &scale)
{
	Ptr<BulletCollider> newColl = BulletCollider::Create();

	for(IndexT i = 0; i < this->descriptions.Size() ; i++)
	{
		newColl->AddFromDescription(descriptions[i]);
	}	
	newColl->name = this->name;
	newColl->shape->setLocalScaling(Neb2BtVector(scale));
	return newColl;
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletCollider::AddFromDescription(const Physics::ColliderDescription & description)
{	
	
	switch(description.type)
	{
		case ColliderSphere:
			{
				AddSphere(description.sphere.radius,description.transform);
			}
			break;
		case ColliderCube:
			{
				AddBox(description.box.halfWidth,description.transform);
			}
			break;
		case ColliderCylinder:
			{
				AddCylinder(description.cylinder.radius,description.cylinder.height,description.transform);
			}
			break;
		case ColliderCapsule:
			{
				AddCapsule(description.capsule.radius,description.capsule.height,description.transform);
			}
			break;
		case ColliderPlane:
			{
				AddPlane(description.plane.plane,description.transform);
			}
			break;
		case ColliderMesh:
			{
				Ptr<ManagedPhysicsMesh> mesh = Resources::ResourceManager::Instance()->CreateManagedResource(PhysicsMesh::RTTI,description.mesh.meshResource).cast<ManagedPhysicsMesh>();				
				AddPhysicsMesh(mesh,description.transform,description.mesh.meshType,description.mesh.primGroup);				
			}
			break;
	}
	BaseCollider::AddFromDescription(description);
}
//------------------------------------------------------------------------------
/**
*/
void
BulletCollider::RenderDebug(const matrix44 & trans)
{
	// empty
}
}