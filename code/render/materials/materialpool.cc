//------------------------------------------------------------------------------
//  materialpool.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "materialpool.h"
#include "io/bxmlreader.h"
#include "materialserver.h"
namespace Materials
{

__ImplementClass(Materials::MaterialPool, 'MAPO', Resources::ResourceStreamPool);

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
MaterialPool::LoadFromStream(const Ids::Id24 id, const Util::StringAtom & tag, const Ptr<IO::Stream>& stream)
{
	Ptr<IO::BXmlReader> reader = IO::BXmlReader::Create();
	reader->SetStream(stream);
	if (reader->Open())
	{
		// make sure it's a valid frame shader file
		if (!reader->HasNode("/NebulaT/Surface"))
		{
			n_error("StreamSurfaceMaterialLoader: '%s' is not a valid surface!", stream->GetURI().AsString().AsCharPtr());
			return Failed;
		}

		// send to first node
		reader->SetToNode("/NebulaT/Surface");

		// load surface
		Resources::ResourceName materialType = reader->GetString("template");
		Materials::MaterialServer* server = Materials::MaterialServer::Instance();
		MaterialId id = server->AllocateMaterial(materialType);

		if (reader->SetToFirstChild("Param")) do
		{
			Util::StringAtom paramName = reader->GetString("name");
			if (!parameters.Contains(paramName))
			{
				n_warning(Util::String::Sprintf("No parameter matching name '%s' exists in the material template '%s'\n", paramName.AsString().AsCharPtr(), material->GetName().AsString().AsCharPtr()).AsCharPtr());
				continue;
			}
			const Material::MaterialParameter& param = parameters[paramName];

			// set variant value which we will use in the surface constants
			Variant var = param.defaultVal;
			switch (param.defaultVal.GetType())
			{
			case Variant::Float:
				var.SetFloat(reader->GetOptFloat("value", 0.0f));
				break;
			case Variant::Int:
				var.SetInt(reader->GetOptInt("value", 0));
				break;
			case Variant::Bool:
				var.SetBool(reader->GetOptBool("value", false));
				break;
			case Variant::Float4:
				var.SetFloat4(reader->GetOptFloat4("value", Math::float4(0)));
				break;
			case Variant::Float2:
				var.SetFloat2(reader->GetOptFloat2("value", Math::float2(0)));
				break;
			case Variant::Matrix44:
				var.SetMatrix44(reader->GetOptMatrix44("value", Math::matrix44()));
				break;
			case Variant::String:
			{
				var.SetString(reader->GetString("value"));
				break;
			}
			}

			// create binding object
			Surface::SurfaceValueBinding obj;
			obj.value = var;
			obj.system = param.system;

			// add to the static values in the surface
			surface->staticValues.Add(paramName, obj);
		} while (reader->SetToNextChild("Param"));
	}
	return LoadStatus();
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceUnknownId
MaterialPool::AllocObject()
{
	// actually, yes, return a crap ID, we do all the work in the LoadFromStream...
	return Resources::ResourceUnknownId();
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialPool::DeallocObject(const Resources::ResourceUnknownId id)
{
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialPool::Unload(const Ids::Id24 id)
{
}

} // namespace Materials
