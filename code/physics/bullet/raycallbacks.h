#pragma once
//------------------------------------------------------------------------------
/**
    Bullet Raycallback overloads supporting filtersets
	pretty much copy paste from btcollisionworld. not really pretty but it seems to be the only way
    
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/

#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "physics/filterset.h"


namespace Bullet
{

///RayResultCallback is used to report new raycast results
struct	BulletResultCallback : public btCollisionWorld::RayResultCallback
{
	
	const Physics::FilterSet & filter;

	BulletResultCallback(const Physics::FilterSet &filterSet)
		:RayResultCallback(), filter(filterSet)
	{
	}

	virtual bool needsCollision(btBroadphaseProxy* proxy0) const
	{		
		if(btCollisionWorld::RayResultCallback::needsCollision(proxy0))
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
				Physics::PhysicsObject * body = static_cast<Physics::PhysicsObject*>(obj->getUserPointer());
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

struct	BulletClosestRayResultCallback : public BulletResultCallback
{
	BulletClosestRayResultCallback(const btVector3&	rayFromWorld,const btVector3&	rayToWorld, const Physics::FilterSet & filterSet)
		:m_rayFromWorld(rayFromWorld),
		m_rayToWorld(rayToWorld),
		BulletResultCallback(filterSet)
	{
	}

	btVector3	m_rayFromWorld;//used to calculate hitPointWorld from hitFraction
	btVector3	m_rayToWorld;

	btVector3	m_hitNormalWorld;
	btVector3	m_hitPointWorld;

	virtual	btScalar	addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace)
	{
		//caller already does the filter on the m_closestHitFraction
		btAssert(rayResult.m_hitFraction <= m_closestHitFraction);

		m_closestHitFraction = rayResult.m_hitFraction;
		m_collisionObject = rayResult.m_collisionObject;
		if (normalInWorldSpace)
		{
			m_hitNormalWorld = rayResult.m_hitNormalLocal;
		} else
		{
			///need to transform normal into worldspace
			m_hitNormalWorld = m_collisionObject->getWorldTransform().getBasis()*rayResult.m_hitNormalLocal;
		}
		m_hitPointWorld.setInterpolate3(m_rayFromWorld,m_rayToWorld,rayResult.m_hitFraction);
		return rayResult.m_hitFraction;
	}
};

struct	BulletAllHitsRayResultCallback : public BulletResultCallback
{
	BulletAllHitsRayResultCallback(const btVector3&	rayFromWorld,const btVector3&	rayToWorld,  const Physics::FilterSet & filterSet)
		:m_rayFromWorld(rayFromWorld),
		m_rayToWorld(rayToWorld),
		BulletResultCallback(filterSet)
	{
	}

	btAlignedObjectArray<const btCollisionObject*>		m_collisionObjects;

	btVector3	m_rayFromWorld;//used to calculate hitPointWorld from hitFraction
	btVector3	m_rayToWorld;

	btAlignedObjectArray<btVector3>	m_hitNormalWorld;
	btAlignedObjectArray<btVector3>	m_hitPointWorld;
	btAlignedObjectArray<btScalar> m_hitFractions;

	virtual	btScalar	addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace)
	{
		m_collisionObject = rayResult.m_collisionObject;
		m_collisionObjects.push_back(rayResult.m_collisionObject);
		btVector3 hitNormalWorld;
		if (normalInWorldSpace)
		{
			hitNormalWorld = rayResult.m_hitNormalLocal;
		} else
		{
			///need to transform normal into worldspace
			hitNormalWorld = m_collisionObject->getWorldTransform().getBasis()*rayResult.m_hitNormalLocal;
		}
		m_hitNormalWorld.push_back(hitNormalWorld);
		btVector3 hitPointWorld;
		hitPointWorld.setInterpolate3(m_rayFromWorld,m_rayToWorld,rayResult.m_hitFraction);
		m_hitPointWorld.push_back(hitPointWorld);
		m_hitFractions.push_back(rayResult.m_hitFraction);
		return m_closestHitFraction;
	}
};

}