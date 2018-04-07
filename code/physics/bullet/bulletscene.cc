//------------------------------------------------------------------------------
//  bulletscene.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "physics/bullet/conversion.h"
#include "physics/physicsbody.h"
#include "physics/staticobject.h"
#include "physics/bullet/bulletscene.h"
#include "physics/bullet/debugdrawer.h"
#include "timing/time.h"
#include "physics/physicsserver.h"
#include "framesync/framesynctimer.h"
#include "physics/filterset.h"
#include "physics/bullet/raycallbacks.h"
#include "coregraphics/primitivetopology.h"
#include "coregraphics/shaperenderer.h"
#include "debugrender/debugshaperenderer.h"


namespace Bullet
{
	
using namespace Physics;
using namespace Math;

__ImplementClass(Bullet::BulletScene, 'PBSC', Physics::BaseScene);

//------------------------------------------------------------------------------
/**
*/
BulletScene::BulletScene()
{

}

//------------------------------------------------------------------------------
/**
*/
BulletScene::~BulletScene()
{

}

//------------------------------------------------------------------------------
/**
*/
void
BulletScene::OnActivate()
{	
	this->physics.collisionConfiguration = n_new(btDefaultCollisionConfiguration());

    //const btVector3 worldAabbMin(-200,-10,-200);
    //const btVector3 worldAabbMax(200,200,200);

    //this->physics.broadphase = n_new(btAxisSweep3(worldAabbMin, worldAabbMax));

	// use AABB 
    this->physics.broadphase = n_new(btDbvtBroadphase());
    this->physics.constraintSolver = n_new(btSequentialImpulseConstraintSolver());
    this->physics.dispatcher = n_new(btCollisionDispatcher(this->physics.collisionConfiguration));
    this->physics.dynamicsWorld = n_new(btDiscreteDynamicsWorld(this->physics.dispatcher, this->physics.broadphase, this->physics.constraintSolver, this->physics.collisionConfiguration));

    this->debugDrawer = n_new(DebugDrawer);
    this->debugDrawer->setDebugMode(
        btIDebugDraw::DBG_DrawFeaturesText +
        btIDebugDraw::DBG_DrawText +
        btIDebugDraw::DBG_DrawWireframe +
        btIDebugDraw::DBG_DrawContactPoints +
        btIDebugDraw::DBG_DrawConstraints + btIDebugDraw::DBG_DrawConstraintLimits);
	this->debugDrawer->SetScene(this);
    this->physics.dynamicsWorld->setDebugDrawer(this->debugDrawer);
	this->physics.dynamicsWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
	this->lastUpdate = -1.0f;
}

//------------------------------------------------------------------------------
/**
*/
void
BulletScene::OnDeactivate()
{

	n_assert(0 != this->physics.dynamicsWorld);

	// clear the collision sound hash table
	//this->collisionSounds.Clear();

	// release all attached objects
	this->objects.Clear();

	n_delete(this->physics.dynamicsWorld);
	this->physics.dynamicsWorld = NULL;

	n_delete(this->physics.collisionConfiguration);
	this->physics.collisionConfiguration = NULL;

	n_delete(this->physics.broadphase);
	this->physics.broadphase = NULL;

	n_delete(this->physics.constraintSolver);
	this->physics.constraintSolver = NULL;

	n_delete(this->physics.dispatcher);
	this->physics.dispatcher = NULL;

	n_delete(this->debugDrawer);
	this->debugDrawer = NULL;
}

//------------------------------------------------------------------------------
/**
*/
void
BulletScene::Trigger()
{
	Timing::Time now = PhysicsServer::Instance()->GetTime();
	Timing::Time delta_t;
	if (this->lastUpdate<0.0f)
	{
		delta_t = 0.16f;
	}
	else
	{
		delta_t = now - this->lastUpdate;
	}
	
	if(delta_t > 0.0f)
	{
		this->physics.dynamicsWorld->stepSimulation((btScalar)delta_t,PhysicsServer::Instance()->GetSimulationStepLimit(),PhysicsServer::Instance()->GetSimulationFrameTime());	
		
		if(this->physics.dynamicsWorld->getDispatcher()->getNumManifolds() > 0)
			BulletPhysicsServer::Instance()->HandleCollisions();
		this->lastUpdate = now;
	}
}

//------------------------------------------------------------------------------
/**
*/
const Math::vector& 
BulletScene::GetGravity() const
{
	// MEH
	static Math::vector grav;
	grav = Bt2NebVector(this->physics.dynamicsWorld->getGravity());
	return grav;
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletScene::SetGravity(const Math::vector& v)
{
	this->physics.dynamicsWorld->setGravity(Neb2BtVector(v));
}

//------------------------------------------------------------------------------
/**
*/
struct ContactCallback : public btCollisionWorld::ContactResultCallback {
	
	//! Constructor, pass whatever context you want to have available when processing contacts
	/*! You may also want to set m_collisionFilterGroup and m_collisionFilterMask
	 *  (supplied by the superclass) for needsCollision() */
	ContactCallback(btCollisionObject& shape,Util::Array<Ptr<PhysicsObject> >& result, const FilterSet & filterSet)
		: btCollisionWorld::ContactResultCallback(), testShape(shape),results(result), filter(filterSet) { }
	
	btCollisionObject & testShape;
	Util::Array<Ptr<PhysicsObject> >& results;
	Util::Array<PhysicsObject::Id> resultIds;
	const FilterSet & filter;
		
	//! Called with each contact for your own processing (e.g. test if contacts fall in within sensor parameters)
	virtual btScalar addSingleResult(btManifoldPoint& cp,
		const btCollisionObjectWrapper* colObj0,int partId0,int index0,
		const btCollisionObjectWrapper* colObj1,int partId1,int index1)
	{
		
		const btCollisionObject * foo;
		if(colObj0->getCollisionObject()==&testShape) {
			foo = colObj1->getCollisionObject();
		} else {
			foo = colObj0->getCollisionObject();
		}
		PhysicsObject * body = (PhysicsObject*)foo->getUserPointer();
		if(body)
		{
			if (InvalidIndex == resultIds.BinarySearchIndex(body->GetUniqueId()))
			{
				Ptr<PhysicsObject> ent(body);
				results.Append(ent);
				resultIds.InsertSorted(body->GetUniqueId());
			}
			
		}
		// do stuff with the collision point
		return 0; // not actually sure if return value is used for anything...?
	}

	// override collision check and apply filterset
	virtual bool needsCollision(btBroadphaseProxy* proxy0) const
	{
		if(btCollisionWorld::ContactResultCallback::needsCollision(proxy0))
		{
			// broadphase figures we need a collision, lets check with filter too
			btCollisionObject * obj = static_cast<btCollisionObject*>(proxy0->m_clientObject);
			if(obj == NULL)
			{
				// we found something odd, best just ignore it since we wont be able to do anything with it anyway
				return false;
			}
			if(obj->getUserPointer())
			{
				PhysicsObject * body = static_cast<PhysicsObject*>(obj->getUserPointer());
				if(body)
				{
					return !filter.CheckObject(body);					
				}
			}
			return false;
		}
		else
		{
			return false;
		}			
	}
};

//------------------------------------------------------------------------------
/**
    This method returns all physics entities touching the given box shaped 
    area. The method creates a box shape and calls its collide
    method, so it's quite fast. Note that entities will be appended to the
    array, so usually you should make sure to pass an empty array. This method
    will also overwrite the  contactPoints array which can be 
    queried after the method has returned, but note that there will only
    be one contact per physics shape.

    @param  pos         center of box
    @param  scale       box scaling
    @param  excludeSet  what contacts should be ignored?
    @param  result      array which will be filled with entity pointers
    @return             number of entities touching the box
*/
int 
BulletScene::GetObjectsInBox(const Math::matrix44& transform, const Math::vector& halfWidth, const Physics::FilterSet& excludeSet, Util::Array<Ptr<Physics::PhysicsObject>>& result)
{
    // first remove scaling from transformation matrix	
    matrix44 pure = matrix44::identity();
    pure.set_position(transform.get_position());
    pure.set_xaxis(float4::normalize(transform.get_xaxis()));
    pure.set_yaxis(float4::normalize(transform.get_yaxis()));
    pure.set_zaxis(float4::normalize(transform.get_zaxis()));

	btPairCachingGhostObject * ghostObject = n_new(btPairCachingGhostObject);

	ghostObject->setWorldTransform(Neb2BtM44Transform(transform));
	btCollisionShape * collisionShape = n_new(btBoxShape(Neb2BtVector(halfWidth)));
	ghostObject->setCollisionShape(collisionShape);

	GetEntitiesInShape(ghostObject, excludeSet, result);

	n_delete(collisionShape);
	n_delete(ghostObject);
    return result.Size();
}

//------------------------------------------------------------------------------
/**
    This method returns all physics entities touching the given spherical 
    area. The method creates a sphere shape and calls its collide
    method, so it's quite fast. Note that entities will be appended to the
    array, so usually you should make sure to pass an empty array. This method
    will also overwrite the  contactPoints array which can be 
    queried after the method has returned, but note that there will only
    be one contact per physics shape.

    @param  pos         center of the sphere
    @param  radius      radius of the sphere
    @param  excludeSet  what contacts should be ignored?
    @param  result      array which will be filled with entity pointers
    @return             number of entities touching the sphere
*/
int 
BulletScene::GetObjectsInSphere(const Math::vector& pos, float radius, const Physics::FilterSet& excludeSet, Util::Array<Ptr<Physics::PhysicsObject> >& result)
{
	n_assert(radius >= 0.0f);
	
	// create a sphere shape and perform collision check
	Math::matrix44 m = Math::matrix44::identity();
	m.translate(pos);

	btCollisionShape * collisionShape = n_new(btSphereShape(radius));

	btPairCachingGhostObject * ghostObject = n_new(btPairCachingGhostObject);

	ghostObject->setWorldTransform(Neb2BtM44Transform(m));
	ghostObject->setCollisionShape(collisionShape);
	
	GetEntitiesInShape(ghostObject, excludeSet, result);

	n_delete(collisionShape);
	n_delete(ghostObject);

	return result.Size();
}

//------------------------------------------------------------------------------
/**
*/
int
BulletScene::GetEntitiesInShape(btPairCachingGhostObject * shape, const FilterSet& excludeSet, Util::Array<Ptr<PhysicsObject> >& result)
{	
	// setup filter
	short int filter = btBroadphaseProxy::AllFilter;
	short int excludes = excludeSet.GetCollideBits();
	filter &= ~excludes;
	
	shape->setCollisionFlags(filter);

	GetWorld()->addCollisionObject(shape);

	for (int i = 0; i < shape->getNumOverlappingObjects(); i++)
	{
		btCollisionObject* collisionObject = shape->getOverlappingObject(i);
		PhysicsObject* physicsObject = static_cast<PhysicsObject*>(collisionObject->getUserPointer());

		if (physicsObject)
			result.Append(physicsObject);
	}

	GetWorld()->removeCollisionObject(shape);

	return result.Size();
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Ptr<Physics::Contact> >
BulletScene::RayCheck(const Math::vector& pos, const Math::vector& dir, const Physics::FilterSet& excludeSet, RayTestType rayType)
{
	const static Math::matrix44 identity = Math::matrix44::identity();
	
	// setup filter
	short int filter = btBroadphaseProxy::AllFilter;
	short int excludes = excludeSet.GetCollideBits();
	filter &= ~excludes;

	btDiscreteDynamicsWorld *world = this->physics.dynamicsWorld;
	n_assert(world);
	Util::Array<Ptr<Contact> >points;
	btVector3 fromBt = Neb2BtVector(pos);
	btVector3 toBt = Neb2BtVector(pos+dir);
	switch(rayType)
	{
	case Test_All:
		{			
			BulletAllHitsRayResultCallback cb(fromBt,toBt,excludeSet);
			cb.m_collisionFilterMask = filter;
			cb.m_collisionFilterGroup = btBroadphaseProxy::AllFilter;
			world->rayTest(fromBt,toBt,cb);
			if(cb.hasHit())
			{
				for(int i=0 ; i < cb.m_hitPointWorld.size() ; i++)
				{
					Math::vector v = Bt2NebVector(cb.m_hitPointWorld[i]);
					Math::vector n = Bt2NebVector(cb.m_hitNormalWorld[i]);
					Ptr<Contact> p = Contact::Create();
					p->SetPoint(v);
					p->SetNormalVector(n);
					p->SetType(Contact::RayCheck);									
					PhysicsObject * obj = (PhysicsObject*)cb.m_collisionObject->getUserPointer();
					if(obj)
					{
						PhysicsUserData *user = obj->GetUserData();
						if(user)
							p->SetCollisionObject(user->physicsObject);
					}					
					points.Append(p);
				}
			}
		}
		break;
	case Test_Closest:
		{
			BulletClosestRayResultCallback cb(fromBt,toBt,excludeSet);	
			cb.m_collisionFilterMask = filter;
			cb.m_collisionFilterGroup = btBroadphaseProxy::AllFilter;
			world->rayTest(fromBt,toBt,cb);
			if(cb.hasHit())
			{
				Math::vector v = Bt2NebVector(cb.m_hitPointWorld);
				Math::vector n = Bt2NebVector(cb.m_hitNormalWorld);
				Ptr<Contact> p = Contact::Create();
				p->SetPoint(v);
				p->SetNormalVector(n);
				p->SetType(Contact::RayCheck);
				PhysicsObject * obj = (PhysicsObject*)cb.m_collisionObject->getUserPointer();
				if(obj)
				{
					PhysicsUserData *user = obj->GetUserData();
					if(user)
						p->SetCollisionObject(user->physicsObject);
				}
				points.Append(p);
			}
		}		
	}
	return points;
}

//------------------------------------------------------------------------------
/**
*/
void
BulletScene::RenderDebug()
{
	// reset primitives list, this will be filled whenever the debugDrawer performs 'drawLine'
	this->debugPrimitives.Clear();
    this->debugTriangles.Clear();
	this->debugPrimitives.Reserve(65535);
    this->debugTriangles.Reserve(65535);
	this->physics.dynamicsWorld->debugDrawWorld();
	BaseScene::RenderDebug();	
	if(!this->debugPrimitives.IsEmpty())
	{
		// draw buffered primitives
		Debug::DebugShapeRenderer::Instance()->DrawPrimitives(matrix44::identity(), CoreGraphics::PrimitiveTopology::LineList, this->debugPrimitives.Size() / 2, &this->debugPrimitives[0], float4(1));
	}
    if (!this->debugTriangles.IsEmpty())
    {
        // draw buffered primitives
        Debug::DebugShapeRenderer::Instance()->DrawPrimitives(matrix44::identity(), CoreGraphics::PrimitiveTopology::TriangleList, this->debugTriangles.Size() / 3, &this->debugTriangles[0], float4(1), CoreGraphics::RenderShape::Wireframe);
    }
}


} // namespace Bullet