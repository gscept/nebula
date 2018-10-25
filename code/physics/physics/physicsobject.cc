//------------------------------------------------------------------------------
//  physicsobject.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/physicsobject.h"
#include "physics/physicsbody.h"
#include "physics/collider.h"
#include "debugrender/debugrender.h"
#include "io/xmlreader.h"
#include "staticobject.h"
#include "physics/staticobject.h"
#include "io/ioserver.h"
#include "resources/resourcemanager.h"

using namespace Util;
using namespace Math;

namespace Physics
{
__ImplementAbstractClass(Physics::PhysicsObject, 'PHOB', Core::RefCounted);
__ImplementClass(Physics::PhysicsUserData, 'PHUD', Core::RefCounted);

uint PhysicsObject::uniqueIdCounter = 1;

//------------------------------------------------------------------------------
/**
*/
PhysicsObject::PhysicsObject() : enabled(false),attached(false)
{
	this->uniqueId = uniqueIdCounter++;
	this->userData = PhysicsUserData::Create();
	userData->physicsObject = this;
}

//------------------------------------------------------------------------------
/**
*/
PhysicsObject::~PhysicsObject()
{
	userData->object = 0;
	userData = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsObject::RenderDebug()
{
	Util::String txt = "ID:";	
	txt.Append(Util::String::FromInt(this->GetUniqueId()));
	txt.Append(", ");
	txt.Append(this->GetName());
	txt.Append("Material: ");
	txt.Append(MaterialTable::MaterialTypeToString(this->common.material).AsString());
	Math::point pos = this->GetTransform().get_position();
	_debug_text3D(txt, pos, Math::float4(1,0,1,1));
}

//------------------------------------------------------------------------------
/**
    Loads resources from an xml file:
    <physics>
    <collidergroup name="foo">
    <collider type="0" transform="..." more attr depending on type />
    .. more colliders
    </collidergroup>
    <body mass="2" name="whatever" flags="0" collider="foo" />
    ...
    <static name="whatever2" collider="foo" />
    ...

    </physics>
*/
Util::Array<Ptr<PhysicsObject> >
PhysicsObject::CreateFromStream(const Util::String & filename, const Math::matrix44 & inTrans)
{

	n_assert2(IO::IoServer::Instance()->FileExists(filename), "failed to open file");

	Util::Array<Ptr<PhysicsObject> > ret;
#if 0
	Ptr<IO::XmlReader> xmlReader = IO::XmlReader::Create();
	xmlReader->SetStream(IO::IoServer::Instance()->CreateStream(filename));
	if (xmlReader->Open())
	{
		// make sure it's a physics file
		if (xmlReader->GetCurrentNodeName() != "physics")
		{
			n_error("PhysicsProperty::EnablePhysics(): not a valid Physics file!");
			return ret;
		}
		HashTable<String,Ptr<Collider> > colliders;
		if(xmlReader->SetToFirstChild("collidergroup")) do
		{
			String name = xmlReader->GetString("name");
			Ptr<Collider> collider = Collider::Create();
			colliders.Add(name,collider);
			if (xmlReader->SetToFirstChild("collider")) do
			{
				int colltype = xmlReader->GetInt("type");

				matrix44 localtrans = xmlReader->GetMatrix44("transform");
				switch(colltype)
				{
				case 0:
					{
						// plane
						float4 norm = xmlReader->GetFloat4("normal");
						float4 point = xmlReader->GetFloat4("point");									
						collider->AddPlane(plane(point,norm),localtrans);
					}
					break;
				case 1:
					{
						// box
						float4 halfwidth = xmlReader->GetFloat4("halfWidth");
						collider->AddBox(halfwidth,localtrans);
					}
					break;
				case 2:
					{
						// sphere
						float radius = xmlReader->GetFloat("radius");
						collider->AddSphere(radius,localtrans);
					}
					break;
				case 3:
					{
						// capsule
						float radius = xmlReader->GetFloat("radius");
						float height = xmlReader->GetFloat("height");
						collider->AddCapsule(radius,height,localtrans);
					}
					break;
				case 4:
					{
						// mesh
						Util::String resourcename = xmlReader->GetString("file");
						int shapetype = xmlReader->GetInt("shapetype");
						int primGroup = xmlReader->GetInt("primGroup");
						Ptr<ManagedPhysicsMesh> mesh = Resources::ResourceManager::Instance()->CreateManagedResource(PhysicsMesh::RTTI,resourcename).cast<ManagedPhysicsMesh>();
						collider->AddPhysicsMesh(mesh,localtrans,(Physics::MeshTopologyType)shapetype, primGroup);
					}
					break;
				}						
			} while (xmlReader->SetToNextChild("collider"));
		} while (xmlReader->SetToNextChild("collidergroup"));


		if (xmlReader->SetToFirstChild("body")) do
		{			
			Ptr<PhysicsBody> body = PhysicsBody::Create();
			PhysicsCommon c;
			c.mass = xmlReader->GetFloat("mass");					
			c.name = xmlReader->GetString("name");
			c.bodyFlags = xmlReader->GetInt("flags");
			c.type = PhysicsBody::RTTI.GetFourCC();
			String collname = xmlReader->GetString("collider");			
			n_assert2(colliders.Contains(collname),"undefined collider in body definition");
			c.collider = colliders[collname];
			body->SetupFromTemplate(c);			
			ret.Append(body.cast<PhysicsObject>());
		} while (xmlReader->SetToNextChild("body"));

		if (xmlReader->SetToFirstChild("static")) do
		{	
			Ptr<StaticObject> body = StaticObject::Create();		
			PhysicsCommon c;							
			c.name = xmlReader->GetString("name");			
			String collname = xmlReader->GetString("collider");			
			n_assert2(colliders.Contains(collname),"undefined collider in body definition");
			c.collider = colliders[collname];
			c.type = StaticObject::RTTI.GetFourCC();
			body->SetupFromTemplate(c);						
			ret.Append(body.cast<PhysicsObject>());		
		} while (xmlReader->SetToNextChild("static"));
		xmlReader->Close();				
	}else{
		n_error("Failed to open physics resource file");
	}
#endif
	return ret;

}

//------------------------------------------------------------------------------
/**
*/
void 
PhysicsObject::SetupFromTemplate(const PhysicsCommon & tmpl)
{
	this->common = tmpl;
}


//------------------------------------------------------------------------------
/**
*/
Ptr<PhysicsObject> 
PhysicsObject::CreateFromTemplate(const PhysicsCommon & tmpl)
{
	Ptr<Core::RefCounted> newobj = Core::Factory::Instance()->Create(tmpl.type);
	Ptr<PhysicsObject> ph = newobj.cast<PhysicsObject>();
	ph->SetupFromTemplate(tmpl);
	return ph;
}


}

