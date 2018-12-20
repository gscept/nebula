//------------------------------------------------------------------------------
//  basescene.cc
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "stdneb.h"
#include "physics/base/basescene.h"
#include "physics/physicsbody.h"
#include "physics/collider.h"

namespace Physics
{
__ImplementAbstractClass(Physics::BaseScene, 'PBSB', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
BaseScene::BaseScene():
	simulationSpeed(1.0f),
	time(0)
{
	this->gravity.set(0, -9.81f, 0);
}

//------------------------------------------------------------------------------
/**
*/
BaseScene::~BaseScene()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseScene::Attach(const Ptr<PhysicsObject> & obj)
{
	obj->Attach(this);	
	this->objects.Append(obj);
}

//------------------------------------------------------------------------------
/**
*/
void
BaseScene::Detach(const Ptr<PhysicsObject> & obj)
{
	Util::Array<Ptr<PhysicsObject>>::Iterator idx = this->objects.Find(obj);
	if(idx)
	{
		this->objects.Erase(idx);
	}
	obj->Detach();	
}

//------------------------------------------------------------------------------
/**
*/
void
BaseScene::RenderDebug()
{
	Util::Array<Ptr<PhysicsObject> >::Iterator iter;
	for(iter = this->objects.Begin();iter != this->objects.End();iter++)
	{
		(*iter)->RenderDebug();
	}
}

//------------------------------------------------------------------------------
/**
*/
bool 
BaseScene::RayBundleCheck(const Math::vector& from, const Math::vector& dir, const Math::vector& upVec, const Math::vector& leftVec, float bundleRadius, const FilterSet& excludeSet, float& outContactDist)
{
	Math::vector offset;

	outContactDist = dir.length();
	bool contact = false;
	float distance = 0.0f;
	int i;
	for (i = 0; i < 4; i++)
	{
		switch (i)
		{
		case 0: offset = upVec * bundleRadius; break;
		case 1: offset = upVec * -bundleRadius; break;
		case 2: offset = leftVec * bundleRadius; break;
		case 3: offset = leftVec * -bundleRadius; break;
		default: break;
		}

		// do ray check
		Util::Array<Ptr<Physics::Contact> > contacts = RayCheck(from + offset, offset + dir, excludeSet,Test_Closest);

		// collided?
		
		IndexT j;
		for (j = 0; j < contacts.Size(); j++)
		{
			distance = Math::vector(contacts[j]->GetPoint() - from).length();

			// Stay away as far as possible
			if (distance < outContactDist)
			{
				outContactDist = distance;
				contact = true;
			}
		}
	}
	return contact;
}

//------------------------------------------------------------------------------
/**
    Shoots a 3d ray into the world and returns the closest contact.

    @param  pos         starting position of ray
    @param  dir         direction and length of ray
    @param  exludeSet   filter set defining objects to exclude
    @return             pointer to closest Contact, or 0 if no contact detected
*/
Ptr<Contact>
BaseScene::GetClosestContactAlongRay(const Math::vector& pos, const Math::vector& dir, const FilterSet& excludeSet)
{
    // do the actual ray check (returns all contacts)
    Util::Array<Ptr<Contact> > contacts = this->RayCheck(pos, dir, excludeSet,Test_Closest);

    int closestContactIndex = -1;
    float closestDistance = dir.length();
    int i;
    int numContacts = contacts.Size();
    Math::vector distVec;
    for (i = 0; i < numContacts; i++)
    {
        Ptr<Contact> curContact = contacts[i];
        distVec = curContact->GetPoint() - pos;
        float dist = distVec.length();
        if (dist < closestDistance)
        {
            closestContactIndex = i;
            closestDistance = dist;
        }
    }
    if (closestContactIndex != -1)
    {
        return contacts[closestContactIndex];
    }
    else
    {
        return 0;
    }
}

//------------------------------------------------------------------------------
/**
    Shoots a 3d ray through the current mouse position and returns the
    closest contact, or a null pointer if no contact.
    NOTE: This gets the current view matrix from the Nebula gfx server.
    This means the check could be one frame of, if the "previous" view matrix
    is used.

    @param  worldMouseRay   Mouse ray in world space
    @param  excludeSet      filter set defining which objects to exclude
    @return                 pointer to closest Contact or 0 if no contact detected
*/
Ptr<Contact>
BaseScene::GetClosestContactUnderMouse(const Math::line& worldMouseRay, const FilterSet& excludeSet)
{
    return this->GetClosestContactAlongRay(worldMouseRay.start(), worldMouseRay.vec(), excludeSet);
}

//------------------------------------------------------------------------------
/**
    Apply an impulse on the first rigid body which lies along the defined ray.
*/
bool
BaseScene::ApplyImpulseAlongRay(const Math::vector& pos, const Math::vector& dir, const FilterSet& excludeSet, float impulse)
{
    const Ptr<Contact> & Contact = this->GetClosestContactAlongRay(pos, dir, excludeSet);
    if (Contact)
    {

        Ptr<PhysicsObject> pobj = Contact->GetCollisionObject();
		if (pobj.isvalid() && pobj->IsA(PhysicsBody::RTTI))
        {
            Math::vector normDir = dir;
			normDir = Math::vector::normalize(normDir);
            pobj.cast<PhysicsBody>()->ApplyImpulseAtPos(Contact->GetPoint(), normDir * impulse);
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseScene::AddIgnoreCollisionPair(const Ptr<Physics::PhysicsBody>& bodyA, const Ptr<Physics::PhysicsBody>& bodyB)
{
	n_error("BaseScene::AddIgnoreCollisionPair: Do not use the base method, implement and use the method from a sub class from this");
}

}