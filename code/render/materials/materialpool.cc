//------------------------------------------------------------------------------
//  materialpool.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "materialpool.h"
#include "io/bxmlreader.h"
#include "resources/resourcemanager.h"
#include "materialserver.h"

namespace Materials
{

__ImplementClass(Materials::MaterialPool, 'MAPO', Resources::ResourceStreamPool);

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
MaterialPool::LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream)
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
		MaterialId mid = server->AllocateMaterial(materialType);

		// add to internal table
		this->materialTable.Add(id, mid);

		const MaterialType* type = server->materialTypesByName[materialType];

		if (reader->SetToFirstChild("Param")) do
		{
			Util::Variant defaultVal;
			Util::StringAtom paramName = reader->GetString("name");
			IndexT index;
			if (type->constants.Contains(paramName, index))
			{
				defaultVal = type->constants.ValueAtIndex(index).default;
			}
			else if (type->textures.Contains(paramName, index))
			{
				defaultVal.SetUInt64(type->textures.ValueAtIndex(index).default.HashCode64());
			}
			else
			{
				const Resources::ResourceName& name = this->GetName(id);
				n_warning(Util::String::Sprintf("No parameter matching name '%s' exists in the material template '%s'\n", paramName.AsString().AsCharPtr(), name.Value()).AsCharPtr());
				continue;
			}

			// set variant value which we will use in the surface constants
			switch (defaultVal.GetType())
			{
			case Util::Variant::Float:
				defaultVal.SetFloat(reader->GetOptFloat("value", defaultVal.GetFloat()));
				MaterialSetConstant(mid, paramName, defaultVal);
				break;
			case Util::Variant::Int:
				defaultVal.SetInt(reader->GetOptInt("value", defaultVal.GetInt()));
				MaterialSetConstant(mid, paramName, defaultVal);
				break;
			case Util::Variant::Bool:
				defaultVal.SetBool(reader->GetOptBool("value", defaultVal.GetBool()));
				MaterialSetConstant(mid, paramName, defaultVal);
				break;
			case Util::Variant::Float4:
				defaultVal.SetFloat4(reader->GetOptFloat4("value", defaultVal.GetFloat4()));
				MaterialSetConstant(mid, paramName, defaultVal);
				break;
			case Util::Variant::Float2:
				defaultVal.SetFloat2(reader->GetOptFloat2("value", defaultVal.GetFloat2()));
				MaterialSetConstant(mid, paramName, defaultVal);
				break;
			case Util::Variant::Matrix44:
				defaultVal.SetMatrix44(reader->GetOptMatrix44("value", defaultVal.GetMatrix44()));
				MaterialSetConstant(mid, paramName, defaultVal);
				break;
			case Util::Variant::String: // texture
			{
				CoreGraphics::TextureId tex = Resources::CreateResource(reader->GetString("value"), tag, nullptr, nullptr);
				MaterialSetTexture(mid, paramName, tex);
				defaultVal.SetUInt64(tex.HashCode64());
				break;
			}
			}
		} while (reader->SetToNextChild("Param"));
	}
	return Success;
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
	// yeah, do nothing, it's fine!
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialPool::Unload(const Resources::ResourceId id)
{
	const MaterialId mid = this->materialTable[id];
	Materials::MaterialServer* server = Materials::MaterialServer::Instance();
	server->DeallocateMaterial(mid);
}

} // namespace Materials
