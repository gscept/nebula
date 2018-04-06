#pragma once
//------------------------------------------------------------------------------
/**
	@class Bullet::BulletCollider

	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "util/array.h"
#include "math/plane.h"
#include "physics/resource/managedphysicsmesh.h"
#include "physics/base/basecollider.h"

class btCompoundShape;
class btCollisionShape;

namespace Bullet
{

class BulletCollider : public Physics::BaseCollider
{
__DeclareClass(BulletCollider);
public:
	/// constructor
	BulletCollider();
	/// destructor
	~BulletCollider();

	/// render debug shapes of colliders
	void RenderDebug(const Math::matrix44& t);
	/// add plane collision object
	void AddPlane( const Math::plane &plane, const Math::matrix44 &localTransform );

	/// Add a box to the collision shape.
	void AddBox( const Math::vector &halfWidth, const Math::matrix44 &localTransform );
	/// Add a box using a bbox as input
	void AddBox( const Math::bbox & box );

	/// Add a sphere to the collision shape.
	void AddSphere( float radius, const Math::matrix44 &localTransform );

	/// Add a Y axis capsule to the collision shape.
	void AddCapsule(float radius, float height, const Math::matrix44  &localTransform );

	/// Add a cylinder to the collision shape.
	void AddCylinder(float radius, float height, const Math::matrix44  &localTransform );

	/// Add a capsule to the collision shape.
	void AddCapsule(const Math::vector& pointA, const Math::vector& pointB, float radius, const Math::matrix44 &localTransform);

	/// Add a triangle mesh to the collision shape.
	void AddPhysicsMesh(Ptr<Physics::ManagedPhysicsMesh> colliderMesh, const Math::matrix44 & localTransform, Physics::MeshTopologyType meshType, int primGroup);

	///
	virtual void AddFromDescription(const Physics::ColliderDescription & description);
	/// creates a new collider with a scaled collision shape
	Ptr<BulletCollider> GetScaledCopy(const Math::vector &scale);
	
private:
	friend class BulletBody;
	friend class BulletScene;
	friend class BulletStatic;
	friend class BulletProbe;

	btCollisionShape * GetBulletShape();

	btCompoundShape * shape;	

	Util::Array<Ptr<Physics::ManagedPhysicsMesh> > meshes;


};

inline btCollisionShape *
BulletCollider::GetBulletShape()
{
	return (btCollisionShape*)shape;
}

}