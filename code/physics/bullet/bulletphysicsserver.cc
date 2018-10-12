//------------------------------------------------------------------------------
//  bulletphysicsserver.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "physics/bullet/conversion.h"
#include "physics/scene.h"
#include "physics/collider.h"
#include "physics/physicsbody.h"
#include "physics/bullet/bulletphysicsserver.h"
#include "../staticobject.h"


namespace Bullet
{
	
__ImplementClass(Bullet::BulletPhysicsServer, 'PBPS', Physics::BasePhysicsServer);

using namespace Math;
using namespace Physics;

//------------------------------------------------------------------------------
/**
*/
void
BulletPhysicsServer::HandleCollisions()
{
	if(this->receivers.Size() == 0)
		return;

	btDynamicsWorld * world = this->GetScene()->GetWorld();
	int numManifolds = world->getDispatcher()->getNumManifolds();
	for (int i=0;i<numManifolds;i++)
	{
		btPersistentManifold* contactManifold =  world->getDispatcher()->getManifoldByIndexInternal(i);
		const btCollisionObject* obA = static_cast<const btCollisionObject*>(contactManifold->getBody0());
		const btCollisionObject* obB = static_cast<const btCollisionObject*>(contactManifold->getBody1());

		PhysicsObject*  va = (PhysicsObject*)obA->getUserPointer();
		PhysicsObject*  vb = (PhysicsObject*)obB->getUserPointer();

		if(!va || !vb)
			continue;
		if(contactManifold->getNumContacts()==0)
			continue;

		// shouldnt happen, but just in case
		if(va->IsA(StaticObject::RTTI) && vb->IsA(StaticObject::RTTI))
			continue;

		// check if neither of the objects wants a callback at all
		if(!va->GetUserData()->enableCollisionCallback && !vb->GetUserData()->enableCollisionCallback)
			continue;

		Ptr<Contact> cp = Contact::Create();

		const btManifoldPoint &contact = contactManifold->getContactPoint(0);										
		Util::Array<Math::vector> normals;
        Util::Array<Math::point> points;
		float depth = 0;
		for(int cc = 0; cc < contactManifold->getNumContacts();cc++)
		{
			points.Append(Bt2NebPoint(contactManifold->getContactPoint(cc).getPositionWorldOnA()));
			normals.Append(Bt2NebVector(contactManifold->getContactPoint(cc).m_normalWorldOnB));
			float newdepth = contactManifold->getContactPoint(cc).getDistance();
			depth = newdepth>depth?newdepth:depth;				
		}						
		cp->SetDepth(depth);
		cp->SetNormalVectors(normals);
		cp->SetPoints(points);

		if(va->GetUserData()->enableCollisionCallback)
		{
			cp->SetOwnerObject(va);
			cp->SetCollisionObject(vb);
			cp->SetMaterial(vb->GetMaterialType());
			for(Util::Array<CollisionReceiver*>::Iterator iter = this->receivers.Begin();iter != this->receivers.End();iter++)
			{
				(*iter)->OnCollision(va,vb,cp);
			}
		}

		if(vb->GetUserData()->enableCollisionCallback)
		{
			cp->SetOwnerObject(vb);
			cp->SetCollisionObject(va);
			cp->SetMaterial(va->GetMaterialType());
			for(Util::Array<CollisionReceiver*>::Iterator iter = this->receivers.Begin();iter != this->receivers.End();iter++)
			{
				(*iter)->OnCollision(vb,va,cp);
			}
		}
	}
}

}