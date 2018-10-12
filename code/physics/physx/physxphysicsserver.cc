//------------------------------------------------------------------------------
//  physxphysicsserver.cc
//  (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "stdneb.h"
#include "physics/physicsserver.h"
#include "PxPhysicsAPI.h"
#include "extensions/PxDefaultErrorCallback.h"
#include "extensions/PxDefaultAllocator.h"
#include "physxprofilesdk/PxProfileZoneManager.h"
#include "common/PxTolerancesScale.h"
#include "physics/materialtable.h"
#include "physics/physicsprobe.h"
#include "physics/contact.h"
#include "physxutils.h"
#include "physxbody.h"

#define PVD_HOST "127.0.0.1"

using namespace physx;
using namespace Physics;

static PhysX::NebulaAllocatorCallback gDefaultAllocatorCallback;

class NebulaErrorCallback : public PxErrorCallback
{
public:
	virtual void reportError(PxErrorCode::Enum code, const char* message, const char* file,
		int line)
	{
		n_error("PhysXError: type: %d, %s, \nfile: %s\nline: %d\n", code, message, file, line);
	}
};

static NebulaErrorCallback gDefaultErrorCallback;

namespace PhysX
{
__ImplementAbstractClass(PhysX::PhysXServer, 'PXSR', Physics::BasePhysicsServer);
__ImplementInterfaceSingleton(PhysX::PhysXServer);

using namespace Math;

//------------------------------------------------------------------------------
/**
*/
PhysXServer::PhysXServer()
{
    __ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
PhysXServer::~PhysXServer()
{
    __DestructInterfaceSingleton;
}


//------------------------------------------------------------------------------
/**
   
*/
void
PhysXServer::SetScene(Physics::Scene * level)
{
	BasePhysicsServer::SetScene(level);    
}

//------------------------------------------------------------------------------
/**
   
*/
bool
PhysXServer::Open()
{
	if (BasePhysicsServer::Open())
	{
		
		this->foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
		n_assert2(this->foundation, "PxCreateFoundation failed!");
		
		bool recordMemoryAllocations = true;
		this->profileZoneManager = &PxProfileZoneManager::createProfileZoneManager(this->foundation);
		n_assert2(this->profileZoneManager, "PxProfileZoneManager::createProfileZoneManager failed!");

		this->physics = PxCreatePhysics(PX_PHYSICS_VERSION, *this->foundation, PxTolerancesScale(), recordMemoryAllocations, this->profileZoneManager);
		n_assert2(this->physics, "PxCreatePhysics failed!");

		if (this->physics->getPvdConnectionManager())
		{
			this->physics->getVisualDebugger()->setVisualizeConstraints(true);
			this->physics->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_CONTACTS, true);
			this->physics->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_SCENEQUERIES, true);
			this->pvd = PxVisualDebuggerExt::createConnection(this->physics->getPvdConnectionManager(), PVD_HOST, 5425, 10, PxVisualDebuggerExt::getAllConnectionFlags());
		}

		this->cooking = PxCreateCooking(PX_PHYSICS_VERSION, *this->foundation, PxCookingParams(PxTolerancesScale()));
		n_assert2(this->cooking, "PxCreateCooking failed!");

		if (!PxInitExtensions(*this->physics))
		{
			n_error("PxInitExtensions failed!");
		}
			
        Util::Array<Physics::MaterialTable::Material> mats = Physics::MaterialTable::GetMaterialTable();
        this->materials.SetSize(mats.Size()+1);
        for (int i = 0; i < mats.Size(); i++)
        {
            PxMaterial * newMat = this->physics->createMaterial(mats[i].friction, mats[i].friction, mats[i].restitution);            
            this->materials[i + 1] = newMat;
        }
        PxMaterial * invalidMat = this->physics->createMaterial(0.5, 0.5, 0.5);
        this->materials[0] = invalidMat;
	}
    return false;
}

//------------------------------------------------------------------------------
/**
    
*/
void
PhysXServer::Close()
{
	BasePhysicsServer::Close();
	PxCloseExtensions();
	this->cooking->release();	
	this->physics->release();
	this->profileZoneManager->release();
	this->foundation->release();
}

//------------------------------------------------------------------------------
/**
    
*/
void
PhysXServer::Trigger()
{
	BasePhysicsServer::Trigger();
}

//------------------------------------------------------------------------------
/**
    
*/
void
PhysXServer::RenderDebug()
{
	BasePhysicsServer::RenderDebug();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXServer::HandleCollisions()
{
	n_error("not implemented");
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXServer::onTrigger(PxTriggerPair* pairs, PxU32 count)
{
	this->critSect.Enter();
	for (PxU32 i = 0; i < count;i++)
	{
		if (!pairs[i].flags.isSet(PxTriggerPairFlag::eDELETED_SHAPE_OTHER) && !pairs[i].flags.isSet(PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER))
		{
			PhysX::PhysXProbe * probe = (PhysX::PhysXProbe*)pairs[i].triggerActor->userData;
			if (pairs[i].otherActor->userData)
			{
				probe->OnTriggerEvent(pairs[i].status, pairs[i].otherActor);
			}
		}
	}
	this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXServer::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{
	this->critSect.Enter();
	if (pairHeader.actors[0]->userData && pairHeader.actors[1]->userData)
	{
		Physics::PhysicsObject* objA = (Physics::PhysicsObject*)pairHeader.actors[0]->userData;
		Physics::PhysicsObject* objB = (Physics::PhysicsObject*)pairHeader.actors[1]->userData;

		// check if neither of the objects wants a callback at all
		if (!objA->GetUserData()->enableCollisionCallback && !objB->GetUserData()->enableCollisionCallback)
		{
			return;
		}

		Ptr<Contact> contact = Contact::Create();
		Util::Array<Math::point> points;
		Util::Array<Math::vector> normals;

		// limit amount of processed contact points
		PxContactPairPoint contactPoints[10];
		float penetration = FLT_MAX;
		for (PxU32 i = 0;i < nbPairs;i++)
		{
			PxU32 contactCount = Math::n_min(10, pairs[i].contactCount);
			if (contactCount)
			{				
				pairs[i].extractContacts(contactPoints, contactCount);

				for (PxU32 j = 0;j < contactCount;j++)
				{
					float seperation = contactPoints[j].separation;
					if (seperation < penetration)
					{
						penetration = seperation;
					}
					points.Append(Px2NebPoint(contactPoints[j].position));
					normals.Append(Px2NebVec(contactPoints[j].normal));					
				}
			}
		}
		contact->SetNormalVectors(normals);
		contact->SetPoints(points);
		contact->SetDepth(penetration);		
		
		if (objA->IsA(PhysX::PhysXBody::RTTI) && objB->IsA(PhysX::PhysXBody::RTTI))
		{
			contact->SetType(BaseContact::DynamicType);
		}
		else
		{
			contact->SetType(BaseContact::StaticType);
		}
		
		if (objA->GetUserData()->enableCollisionCallback)
		{
			contact->SetOwnerObject(objA);
			contact->SetCollisionObject(objB);
			contact->SetMaterial(objB->GetMaterialType());
			for (Util::Array<CollisionReceiver*>::Iterator iter = this->receivers.Begin();iter != this->receivers.End();iter++)
			{
				(*iter)->OnCollision(objA, objB, contact);
			}
		}

		if (objB->GetUserData()->enableCollisionCallback)
		{
			contact->SetOwnerObject(objB);
			contact->SetCollisionObject(objA);
			contact->SetMaterial(objA->GetMaterialType());
			for (Util::Array<CollisionReceiver*>::Iterator iter = this->receivers.Begin();iter != this->receivers.End();iter++)
			{
				(*iter)->OnCollision(objB, objA, contact);
			}
		}
	}
	this->critSect.Leave();
}

} // namespace PhysX