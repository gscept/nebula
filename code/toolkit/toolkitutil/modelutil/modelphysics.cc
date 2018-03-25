//------------------------------------------------------------------------------
//  modelphysics.cc
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "modelphysics.h"

#include "io/stream.h"
#include "io/ioserver.h"
#include "io/xmlwriter.h"
#include "io/xmlreader.h"

using namespace Util;
using namespace IO;


namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::ModelPhysics, 'MDPH', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
ModelPhysics::ModelPhysics() : 
	physicsMode(ToolkitUtil::UseBoundingBox),
	meshMode(Physics::MeshConvex)	
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ModelPhysics::~ModelPhysics()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelPhysics::SetMeshMode(Physics::MeshTopologyType flags)
{
	this->meshMode = flags;
}

//------------------------------------------------------------------------------
/**
*/
Physics::MeshTopologyType 
ModelPhysics::GetMeshMode()
{
	return this->meshMode;
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelPhysics::SetExportMode(PhysicsExportMode flags)
{
	this->physicsMode = flags;
}

//------------------------------------------------------------------------------
/**
*/
PhysicsExportMode 
ModelPhysics::GetExportMode()
{
	return this->physicsMode;
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelPhysics::Save(const Ptr<IO::Stream>& stream)
{
	n_assert(stream.isvalid());

	// set correct access and open stream
	stream->SetAccessMode(Stream::WriteAccess);
	if (stream->Open())
	{
		// create xml writer
		Ptr<XmlWriter> writer = XmlWriter::Create();
		writer->SetStream(stream);
		writer->Open();

		// first write enclosing tag
		writer->BeginNode("Nebula3");

		// set version
		writer->SetInt("version", ModelPhysics::Version);

		// start off with writing flags
		writer->BeginNode("Options");

		// write options		
		writer->SetInt("exportMode", this->physicsMode);
		writer->SetInt("meshMode", this->meshMode);	

		if(this->GetExportMode() == UsePhysics)
		{
			writer->SetString("physicsMesh",this->GetPhysicsMesh());
		}

		// end options node
		writer->EndNode();	

		// end Nebula 3 node
		writer->EndNode();

		// finish writing
		writer->Close();
		stream->Close();
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelPhysics::Load(const Ptr<IO::Stream>& stream)
{
	n_assert(stream.isvalid());

	// set correct access mode and open stream
	stream->SetAccessMode(Stream::ReadAccess);
	if (stream->Open())
	{
		// create xml reader
		Ptr<XmlReader> reader = XmlReader::Create();
		reader->SetStream(stream);
		reader->Open();

		// get version
		int version = reader->GetInt("version");

		// stop loading if version is wrong
		if (version != ModelPhysics::Version)
		{
			stream->Close();
			return;
		}

		// then make sure we have the options tag
		n_assert2(reader->SetToFirstChild("Options"), "CORRUPT .physics FILE!: First tag must be 'Options'!");

		// now load options
		this->physicsMode = (ToolkitUtil::PhysicsExportMode)reader->GetInt("exportMode");
		this->meshMode = (Physics::MeshTopologyType)reader->GetInt("meshMode");

		if(this->GetExportMode() == UsePhysics)
		{
			this->physicsMesh = reader->GetString("physicsMesh");
		}
	
		// finish writing
		reader->Close();
		stream->Close();
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelPhysics::Clear()
{
	// empty
}

} // namespace ToolkitUtil