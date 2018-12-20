#pragma once
//------------------------------------------------------------------------------
/**
	@class Bullet::BulletScene

	(C) 2012-2018 Individual contributors, see AUTHORS file
*/
#include "physics/base/basescene.h"
#include "physics/physicsobject.h"
#include "coregraphics/rendershape.h"

class btDynamicsWorld;
class btDefaultCollisionConfiguration;
class btBroadphaseInterface;
class btConstraintSolver;
class btCollisionDispatcher;
class btDiscreteDynamicsWorld;
class btCollisionObject;
class btPairCachingGhostObject;


namespace Bullet
{

class DebugDrawer;
class BulletScene : public Physics::BaseScene
{

	__DeclareClass(BulletScene);

public:

	BulletScene();
	~BulletScene();


	/// called when the scene is attached to the physics server
	void OnActivate();
	/// called when the scene is detached from the physics server
	void OnDeactivate();
	/// perform one (or more) simulation steps depending on current time
	void Trigger();

	/// set gravity vector
	void SetGravity(const Math::vector& v);
	/// get gravity vector
	const Math::vector& GetGravity() const;

	/// perform debug rendering
	virtual void RenderDebug();
	
	const Ptr<Physics::PhysicsObject> & SimpleRayCheck(const Math::vector & start, const Math::vector & end, uint objectTypes = Physics::All);

	/// return all entities within a spherical area
	int GetObjectsInSphere(const Math::vector& pos, float radius, const Physics::FilterSet& excludeSet, Util::Array<Ptr<Physics::PhysicsObject>>& result);
	/// return all entities within a box 
    int GetObjectsInBox(const Math::matrix44& transform, const Math::vector& halfWidth, const Physics::FilterSet& excludeSet, Util::Array<Ptr<Physics::PhysicsObject>>& result);

	/// Do a ray check starting from position `pos' along ray `dir'.
	Util::Array<Ptr<Physics::Contact> >RayCheck(const Math::vector& pos, const Math::vector& dir, const Physics::FilterSet& excludeSet, RayTestType rayType);
			
private:

	friend class BulletCollider;
	friend class BulletBody;
	friend class BulletStatic;
	friend class BulletProbe;
	friend class BulletPhysicsServer;
	friend class BulletJoint;
	friend class BulletCharacter;
	friend class DebugDrawer;

	struct _physics
	{
		_physics() : collisionConfiguration(NULL), broadphase(NULL),
			constraintSolver(NULL), 
			dispatcher(NULL), dynamicsWorld(NULL)            
		{}
		btDefaultCollisionConfiguration *collisionConfiguration;
		btBroadphaseInterface *broadphase;
		btConstraintSolver *constraintSolver;
		btCollisionDispatcher* dispatcher;
		btDiscreteDynamicsWorld *dynamicsWorld;
	} physics;

	Timing::Time lastUpdate;
	DebugDrawer* debugDrawer;
	Util::Array<CoreGraphics::RenderShape::RenderShapeVertex> debugPrimitives;
    Util::Array<CoreGraphics::RenderShape::RenderShapeVertex> debugTriangles;


	btDynamicsWorld * GetWorld();
	int GetEntitiesInShape(btPairCachingGhostObject * shape, const Physics::FilterSet& excludeSet, Util::Array<Ptr<Physics::PhysicsObject> >& result);
	
};

//------------------------------------------------------------------------------
/**
*/
inline btDynamicsWorld * 
BulletScene::GetWorld()
{
	return (btDynamicsWorld*)this->physics.dynamicsWorld;
}


} // namespace Bullet
