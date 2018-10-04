//------------------------------------------------------------------------------
//  physicsstreammodelloader.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/resource/streamphysicsmodelloader.h"
#include "physics/model/physicsmodel.h"
#include "io/xmlreader.h"
#include "resources/resourcemanager.h"
#include "physics/physicsbody.h"
#include "physics/staticobject.h"


namespace Physics
{
__ImplementClass(Physics::StreamPhysicsModelLoader, 'PSML', Resources::StreamResourceLoader);

using namespace IO;
using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
StreamPhysicsModelLoader::StreamPhysicsModelLoader()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Setup the mesh resource from legacy nvx2 file (Nebula2 binary mesh format).
*/
bool
StreamPhysicsModelLoader::SetupResourceFromStream(const Ptr<Stream>& stream)
{
	n_assert(stream.isvalid());
	n_assert(stream->CanBeMapped());
	
	const Ptr<PhysicsModel>& res = this->resource.downcast<PhysicsModel>();		

	// create a binary reader to parse the N3 file
	// @todo: map stream to memory for faster loading!
	Ptr<BinaryReader> reader = BinaryReader::Create();
	reader->SetStream(stream);
	if (reader->Open())
	{
		// make sure it really it's actually an n3 file and check the version
		// also, we assume that the file has host-native endianess (that's
		// ensured by the asset tools)
		FourCC magic = reader->ReadUInt();
		uint version = reader->ReadUInt();
		if (magic != FourCC('NEB3'))
		{
			n_error("PhysicsStreamModelLoader: '%s' is not a NP3 binary file!", stream->GetURI().AsString().AsCharPtr());
			return false;
		}
		if (version != 1)
		{
			n_error("PhysicsStreamModelLoader: '%s' has wrong version!", stream->GetURI().AsString().AsCharPtr());
			return false;
		}

		// start reading tags
		bool done = false;
		while ((!stream->Eof()) && (!done))
		{
			FourCC fourCC = reader->ReadUInt();
			if (fourCC == FourCC('>MDL'))
			{
				// start of Model
				FourCC classFourCC = reader->ReadUInt();
				String name = reader->ReadString();				
			}
			else if (fourCC == FourCC('<MDL'))
			{
				// end of Model, if we're reloading, we shouldn't load all resources again...
				done = true;
			}
			else if (fourCC == FourCC('>MND'))
			{
				n_error("Not a np3 file");
				// start of a ModelNode				
			}
			else if (fourCC == FourCC('<MND'))
			{
				n_error("Not a np3 file");
				// end of current ModelNode				
			}
			else if (fourCC == FourCC('>PHN'))
			{
				String name = reader->ReadString();
				this->ParseData(res,name, reader);
			}			
			else
			{				
				n_error("Not a np3 file");
			}
		}
		reader->Close();
	}
	else
	{
		n_error("PhysicsStreamModelLoader: can't open file '%s!", stream->GetURI().AsString().AsCharPtr());
	}
return true;
}


//------------------------------------------------------------------------------
/**
*/
void 
StreamPhysicsModelLoader::ParseData(const Ptr<Physics::PhysicsModel>& model,const Util::String & name, const Ptr<IO::BinaryReader> & reader)
{
	bool done = false;
	while ((!reader->Eof()) && (!done))
	{
		FourCC tag = reader->ReadUInt();
		if(tag == FourCC('>MND'))
			continue;					
		if (tag == FourCC('>CLR'))
			continue;
		if (tag == FourCC('<MND'))
			continue;
		if(tag == FourCC('CLGR'))
		{
			String name = reader->ReadString();
			this->ParseCollider(model,name,reader);					
		}
		else if(tag == FourCC('<CLR'))
		{
			done = true;
		}
	}

	done = false;

	while ((!reader->Eof()) && (!done))
	{
		FourCC tag = reader->ReadUInt();

		if(tag == FourCC('POBJ'))
		{
			this->ParsePhysicsObject(model,reader);
		}
		else if(tag == FourCC('PJNT'))
		{
			this->ParseJoint(model,reader);
		}
		else if(tag == FourCC('<PHN'))
		{
			done =  true;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
StreamPhysicsModelLoader::ParseCollider(const Ptr<Physics::PhysicsModel>& model,const Util::String& name, const Ptr<IO::BinaryReader> & reader)
{
	FourCC co = FourCC(reader->ReadUInt());
	Ptr<Collider> pcoll;
	if(!model->colliders.Contains(name))
	{
		pcoll = Collider::Create();
		model->colliders.Add(name,pcoll);
	}
	else
	{
		pcoll = model->colliders[name];
	}
	ColliderDescription coll;	
	coll.transform.setrow0(reader->ReadFloat4());
	coll.transform.setrow1(reader->ReadFloat4());
	coll.transform.setrow2(reader->ReadFloat4());
	coll.transform.setrow3(reader->ReadFloat4());

	coll.type = (ColliderType)reader->ReadInt();
	switch(coll.type)
	{
	case ColliderSphere:
		{
			coll.sphere.radius = reader->ReadFloat();
		}
		break;
	case ColliderCube:
		{
			coll.box.halfWidth = reader->ReadFloat4();
		}
		break;
	case ColliderCapsule:
		{
			coll.capsule.radius = reader->ReadFloat();
			coll.capsule.height = reader->ReadFloat();
		}
		break;
	case ColliderPlane:
		{
			float a = reader->ReadFloat();
			float b = reader->ReadFloat();
			float c = reader->ReadFloat();
			float d = reader->ReadFloat();
			coll.plane.plane.set(a,b,c,d);
		}
		break;
	case ColliderMesh:
		{
			coll.mesh.meshType = (MeshTopologyType)reader->ReadInt();
			coll.mesh.meshResource = reader->ReadString();
			coll.mesh.primGroup = reader->ReadInt();			
		}
		break;
	}

	pcoll->AddFromDescription(coll);


}

//------------------------------------------------------------------------------
/**
*/
void
StreamPhysicsModelLoader::ParsePhysicsObject(const Ptr<Physics::PhysicsModel>& model,const Ptr<IO::BinaryReader> & reader)
{
	PhysicsCommon newObj;
	newObj.type = FourCC(reader->ReadString());
	newObj.name = reader->ReadString();
	newObj.category = (CollideCategory)reader->ReadInt();
	newObj.collideFilterMask = reader->ReadInt();
	newObj.material = (MaterialType)reader->ReadInt();
	String collgroup = reader->ReadString();
	newObj.collider = model->colliders[collgroup];
	newObj.mass = reader->ReadFloat();
	newObj.bodyFlags = reader->ReadInt();

	model->objects.Append(newObj);

}

//------------------------------------------------------------------------------
/**
*/
void 
StreamPhysicsModelLoader::ParseJoint(const Ptr<Physics::PhysicsModel>& model, const Ptr<IO::BinaryReader> & reader)
{

}


} // namespace Physics
